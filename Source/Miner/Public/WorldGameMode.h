// Copyright Schuyler Zheng. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MinerGameMode.h"
#include "WorldLandscape.h"
#include "WorldGameMode.generated.h"

class APlayerCharacter;

/**
 * 
 */
UCLASS()
class MINER_API AWorldGameMode : public AMinerGameMode
{
	GENERATED_BODY()
	
public:
	AWorldGameMode();

protected:
	virtual void BeginPlay();

	void SetPlayerSpawns();
	/** For now just find an acceptable height at 0, 0, 0 */
	FVector FindPlayerSpawnLocation() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawning|Player")
	double SpawnCheckRaycastDistance = 10000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawning|Player")
	TEnumAsByte<ECollisionChannel> LandscapeChannel = ECC_WorldStatic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<AWorldLandscape> ServerWorldLandscape;

	FTerrainDataGeneratedSignature LandscapeGeneratedDelegate;
};
