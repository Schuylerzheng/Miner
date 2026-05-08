// Copyright Schuyler Zheng. All rights reserved.

#include "WorldLandscape.h"
#include "Engine/CollisionProfile.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "ThirdPartyLibraries/FastNoiseLite.h"
#include "WorldGameMode.h"
#include "WorldGenerationRunnable.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY(LogLandscape);

AWorldLandscape::AWorldLandscape()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	CurrentMachineType = HasAuthority() ? MachineType::Server : MachineType::Client;

	DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
	DynamicMeshComponent->SetMobility(EComponentMobility::Movable);
	DynamicMeshComponent->SetGenerateOverlapEvents(false);
	DynamicMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	DynamicMeshComponent->CollisionType = ECollisionTraceFlag::CTF_UseDefault;
	DynamicMeshComponent->SetComplexAsSimpleCollisionEnabled(true);
	
	DynamicMeshComponent->SetEnableGravity(false);

	SetRootComponent(DynamicMeshComponent);

	ApplyTerrainDataDelegate.AddUObject(this, &AWorldLandscape::GenerateTerrain);
}

void AWorldLandscape::BeginPlay()
{
	Super::BeginPlay();

	checkf(ChunkDistance != 0, TEXT("Chunk distance cannot be 0"));

	DynamicMesh = AllocateComputeMesh();
	DynamicMeshComponent->SetDynamicMesh(DynamicMesh);
	DynamicMeshComponent->SetMaterial(0, GrassMaterial);

	// Create the thread that generates the vertex locations
	WorldGenerationRunnable = new FWorldGenerationRunnable(this, GetWorld());

	// Pre-allocate vertex locations array
	const int NumPointsPerLine = FMath::FloorToInt((RenderDistance * 2.0f) / Resolution) + 1;
	int64 ReserveCount = (int64)NumPointsPerLine * (int64)NumPointsPerLine;
	if (ReserveCount > TNumericLimits<int32>::Max()) ReserveCount = TNumericLimits<int32>::Max();
	GeneratedVertexLocations.Reserve((int32)ReserveCount);

	SetupNoise();

	if (CurrentMachineType == MachineType::Server) { LastPlayerLocation = FVector::ZeroVector; }    // TMP, Server just needs to find the spawn chunks for now.
	else {
		check(IsValid(GetWorld())); 
		check(IsValid(LocalClientPawn = GetWorld()->GetFirstPlayerController()->GetPawn()));
		checkf(LocalClientPawn->IsLocallyControlled(), TEXT("LocalClientPawn is not locally controlled"));

		LastPlayerLocation = LocalClientPawn->GetActorLocation();
	}

	// Generate the mesh (last step)
	WorldGenerationRunnable->bGenerate = true; 
	while (WorldGenerationRunnable->bGenerate) { FPlatformProcess::Sleep(0.1f); } // Wait for it to finish
}

void AWorldLandscape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentMachineType == MachineType::Server) {
		// TMP
		return;
	}
	
	checkf(IsValid(LocalClientPawn), TEXT("Client Pawn bad"));
	FVector LocalClientPawnLocation = LocalClientPawn->GetActorLocation();

	// If the player has moved out of bounds, make the mesh follow them. (No Z check for now) (make sure that this runs last because if it is generating it will return)
	if (FMath::RoundToInt(LastPlayerLocation.X / ChunkDistance) * ChunkDistance != FMath::RoundToInt(LocalClientPawnLocation.X / ChunkDistance) * ChunkDistance || FMath::RoundToInt(LastPlayerLocation.Y / ChunkDistance) * ChunkDistance != FMath::RoundToInt(LocalClientPawnLocation.Y / ChunkDistance) * ChunkDistance /* || FMath::RoundToInt(LastPlayerLocation.Z / ChunkDistance) * ChunkDistance != LocalClientPawnLocation.Z */) {
		if (WorldGenerationRunnable->bGenerate) { return; }
		
		LastPlayerLocation = LocalClientPawnLocation;

		// Request mesh data to be generated (apply the mesh later)
		WorldGenerationRunnable->bGenerate = true;
	}
}

void AWorldLandscape::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CurrentMachineType == MachineType::Server) {
		// TMP
		return;
	}

	FreeAllComputeMeshes();
	CleanUp();
}

void AWorldLandscape::SetupNoise()
{
	// Set noise parameters
	SetNoiseParameters(BasicLandNoise, BasicLandNoiseSettings);
	SetNoiseParameters(PlateTectonicsNoise, PlateTectonicsNoiseSettings);
}

void AWorldLandscape::GenerateTerrain()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenerateTerrain);

	check(IsValid(DynamicMeshComponent));
	check(IsValid(DynamicMesh));

	DynamicMesh->InitializeMesh();
	
	// EditMesh > just using notifymesh because more safe
	DynamicMesh->EditMesh([&](UE::Geometry::FDynamicMesh3& Mesh) {
		Mesh = *WorldGenerationRunnable->DynamicMesh->GetMeshPtr();

		// Final validity checks
		ensure(Mesh.CheckValidity(ValidityOptions, ValidityCheckFailMode));
		ensure(Mesh.IsCompact());
	});
}

void AWorldLandscape::SetNoiseParameters(FastNoiseLite*& Noise, const FNoiseSettings& NoiseSettings)
{
	Noise = new FastNoiseLite(Seed);

	Noise->SetFrequency(NoiseSettings.Frequency);
	Noise->SetNoiseType(static_cast<FastNoiseLite::NoiseType>(NoiseSettings.NoiseType.GetValue()));
	Noise->SetRotationType3D(static_cast<FastNoiseLite::RotationType3D>(NoiseSettings.RotationType3D.GetValue()));
	Noise->SetFractalType(static_cast<FastNoiseLite::FractalType>(NoiseSettings.FractalType.GetValue()));
	Noise->SetFractalOctaves(NoiseSettings.FractalOctaves);
	Noise->SetFractalLacunarity(NoiseSettings.FractalLacunarity);
	Noise->SetFractalGain(NoiseSettings.FractalGain);
	Noise->SetFractalWeightedStrength(NoiseSettings.FractalWeightedStrength);
	Noise->SetFractalPingPongStrength(NoiseSettings.FractalPingPongStrength);
	Noise->SetCellularDistanceFunction(static_cast<FastNoiseLite::CellularDistanceFunction>(NoiseSettings.CellularDistanceFunction.GetValue()));
	Noise->SetCellularReturnType(static_cast<FastNoiseLite::CellularReturnType>(NoiseSettings.CellularReturnType.GetValue()));
	Noise->SetCellularJitter(NoiseSettings.CellularJitter);
	Noise->SetDomainWarpType(static_cast<FastNoiseLite::DomainWarpType>(NoiseSettings.DomainWarpType.GetValue()));
	Noise->SetDomainWarpAmp(NoiseSettings.DomainWarpAmp);
}

void AWorldLandscape::CleanUp()
{
	CleanUpPointer(BasicLandNoise);
	CleanUpPointer(PlateTectonicsNoise);

	if (WorldGenerationRunnable)
	{
		WorldGenerationRunnable->Stop();
		WorldGenerationRunnable = nullptr;
	}
}

UDynamicMeshPool* AWorldLandscape::GetComputeMeshPool()
{
	if (DynamicMeshPool == nullptr && bEnableComputeMeshPool)
	{
		DynamicMeshPool = NewObject<UDynamicMeshPool>();
	}
	return DynamicMeshPool;
}


UDynamicMesh* AWorldLandscape::AllocateComputeMesh()
{
	if (bEnableComputeMeshPool)
	{
		UDynamicMeshPool* UsePool = GetComputeMeshPool();
		if (UsePool)
		{
			return UsePool->RequestMesh();
		}
	}

	// if we could not return a pool mesh, allocate a new mesh that isn't owned by the pool
	return NewObject<UDynamicMesh>(this);
}


bool AWorldLandscape::ReleaseComputeMesh(UDynamicMesh* Mesh)
{
	if (bEnableComputeMeshPool && Mesh)
	{
		UDynamicMeshPool* UsePool = GetComputeMeshPool();
		if (UsePool != nullptr)
		{
			UsePool->ReturnMesh(Mesh);
			return true;
		}
	}
	return false;
}


void AWorldLandscape::ReleaseAllComputeMeshes()
{
	UDynamicMeshPool* UsePool = GetComputeMeshPool();
	if (UsePool)
	{
		UsePool->ReturnAllMeshes();
	}
}

void AWorldLandscape::FreeAllComputeMeshes()
{
	UDynamicMeshPool* UsePool = GetComputeMeshPool();
	if (UsePool)
	{
		UsePool->FreeAllMeshes();
	}
}
