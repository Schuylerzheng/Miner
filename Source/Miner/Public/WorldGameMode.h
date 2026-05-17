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
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void SpawnPlayerStarts();
	void GetWorldLandscapes();
	/** For now just find an acceptable height at 0, 0, 0 */
	FVector FindPlayerSpawnLocation() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawning|Player")
	double SpawnCheckRaycastDistance = 10000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawning|Player")
	TEnumAsByte<ECollisionChannel> LandscapeChannel = ECC_WorldStatic;

	TObjectPtr<AWorldLandscape> WorldLandscape = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerStart> PlayerStart = nullptr;
};
