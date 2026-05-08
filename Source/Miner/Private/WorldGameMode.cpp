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

	for (TActorIterator<AWorldLandscape> It(GetWorld()); It; ++It)
	{
		AWorldLandscape* Landscape = *It;

		check(IsValid(Landscape));
		if (IsValid(Landscape))
		{
			Landscape->ApplyTerrainDataDelegate.AddUObject(this, &AWorldGameMode::SpawnPlayerStarts);
		}
	}
}

AActor* AWorldGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	check(IsValid(PlayerStart));

	return PlayerStart;
}

void AWorldGameMode::SpawnPlayerStarts()
{
	PlayerStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FindPlayerSpawnLocation(), FRotator::ZeroRotator);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Player spawn location: %s"), *PlayerStart->GetActorLocation().ToString()));

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
