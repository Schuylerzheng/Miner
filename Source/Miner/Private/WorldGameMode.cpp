// Copyright Schuyler Zheng. All Rights Reserved.

#include "WorldGameMode.h"
#include "PlayerCharacter.h"
#include "WorldLandscape.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

AWorldGameMode::AWorldGameMode()
{

}

void AWorldGameMode::BeginPlay()
{
	Super::BeginPlay();

	check(GetWorld());

	
}

AActor* AWorldGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	//check(IsValid(PlayerStart));    // Player joins too fast, so it is not vaild yet for local join methods like listen server and standalone.

	if (!IsValid(PlayerStart)) { SpawnPlayerStarts(); }

	return PlayerStart;
}

void AWorldGameMode::SpawnPlayerStarts()
{
	//check(IsValid(WorldLandscape));    // Player joins too fast, so it is not vaild yet for local join methods like listen server and standalone.

	if (!IsValid(WorldLandscape)) { GetWorldLandscapes(); }

	if (WorldLandscape->DynamicMesh->IsEmpty()) { unimplemented(); }

	PlayerStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FindPlayerSpawnLocation(), FRotator::ZeroRotator);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Player spawn location: %s"), *PlayerStart->GetActorLocation().ToString()));

}

void AWorldGameMode::GetWorldLandscapes()
{
	for (TActorIterator<AWorldLandscape> It(GetWorld()); It; ++It)
	{
		AWorldLandscape* Landscape = *It;

		check(IsValid(Landscape));
		checkf(!IsValid(WorldLandscape), TEXT("There is more than 1 landscape."))

		if (IsValid(Landscape) && !IsValid(WorldLandscape))	{
			WorldLandscape = Landscape;
			Landscape->ApplyTerrainDataDelegate.AddUObject(this, &AWorldGameMode::SpawnPlayerStarts);
		}
		else {
			unimplemented();    // For now only 1 landscape.
		}
	}
}

FVector AWorldGameMode::FindPlayerSpawnLocation() const
{
	// Settings for the line trace
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;
	bool Finished = false;
	
	// Could use multilinetrace, but this is easier
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(0, 0, SpawnCheckRaycastDistance), FVector(0, 0, -SpawnCheckRaycastDistance), LandscapeChannel, Params)) {
		check(Hit.bBlockingHit);

		// This means that the spawn location is above 
		return FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z + APlayerCharacter::StaticClass()->GetDefaultObject<APlayerCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}

	ensure(!"Couldn't find a landscape to put the player on, so giving world origin.");
	return FVector::ZeroVector;
}
