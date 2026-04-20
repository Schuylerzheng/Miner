// Copyright Schuyler Zheng. All Rights Reserved.

#include "WorldGameMode.h"
#include "PlayerCharacter.h"
#include "WorldLandscape.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"

AWorldGameMode::AWorldGameMode()
{
	ServerWorldLandscape = AWorldLandscape::StaticClass();
}

void AWorldGameMode::BeginPlay()
{
	Super::BeginPlay();

	check(GetWorld());

	GetWorld()->SpawnActor(ServerWorldLandscape);

	LandscapeGeneratedDelegate.AddUObject(this, &AWorldGameMode::SetPlayerSpawns);
}

void AWorldGameMode::SetPlayerSpawns()
{
	unimplemented();
}

FVector AWorldGameMode::FindPlayerSpawnLocation() const
{
	FVector FinalSpawnLocation = FVector(0, 0, SpawnCheckRaycastDistance);

	// Settings for the line trace
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;
	bool Finished = false;
	
	// Could use multilinetrace, but this is easier
	if (GetWorld()->LineTraceSingleByChannel(Hit, FinalSpawnLocation, FVector(0, 0, -SpawnCheckRaycastDistance), LandscapeChannel, Params)) {
		// This means that the spawn location is above 
		return FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);
	}

	ensure(!"Couldn't find a landscape to put the player on, so giving world origin.");
	return FVector::ZeroVector;
}
