// Copyright Schuyler Zheng. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "UDynamicMesh.h"

UENUM(BlueprintType)
enum class EPlateDirection : uint8
{
	North,
	East,
	South,
	West
};

UENUM(BlueprintType)
enum class ECollisionType : uint8 
{
	None, 
	Push,
	Pull
};

class FSingleThreadRunnable;
class AWorldLandscape;

DECLARE_LOG_CATEGORY_EXTERN(LogWorldGenerationThread, Log, All);

/**
 * The thread that handles world generation tasks
 * This will free up the game thread and allow for smoother gameplay
 */
class MINER_API FWorldGenerationRunnable : public FRunnable
{
public:
	FWorldGenerationRunnable(AWorldLandscape* WorldLandscape, UWorld* World);
	~FWorldGenerationRunnable();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	virtual FSingleThreadRunnable* GetSingleThreadInterface() override;

	bool bGenerate = false;

	// Just remove this I guess
	//UPROPERTY(Transient)
	TObjectPtr<UDynamicMesh> DynamicMesh;

protected:
	void GenerateDynamicMesh();

	void GenerateBasicHeights();
	void ApplyPlateTectonics();
	void FinalizeLandMesh();

	void ModifyHeightArray(TFunctionRef<double(FVector)> ModifyFunc);
	/**
	* The master veretex is the most top left vertex, then if there are multiple, the highest
	* It also has to connect to a black part of the main plate in order to count.
	* This is nessesary so that a world generates the same everytime.
	*/
	FVector2D FindMasterVertexOfPlate(FVector2D BoarderLocation);
	/**
	*
	*/
	ECollisionType ArePlatesColliding(EPlateDirection Plate1Direction, EPlateDirection Plate2Direction);

	FRunnableThread* Thread;
	bool bRunThread = true;

	TObjectPtr<AWorldLandscape> OwnerLandscape;
	UWorld* CurrentWorld;

	TArray<int32> Verticies;
	/* Sort of like a height map, but not a map. Maybe make it a map? */
	TArray<double> VertexHeights;

	double LastRenderDistance = -1;
	FVector3d LocalClientPawnLocation;
	int64 NumPointsPerLine;
};
