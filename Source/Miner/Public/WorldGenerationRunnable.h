// Copyright Schuyler Zheng. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "UDynamicMesh.h"
#include "LandscapeGenerationHelpers.h"

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

	UPROPERTY(Transient)
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
	ECollisionType ArePlatesColliding(FVector2D MasterVertexLocation1, FVector2D MasterVertexLocation2);
	FPlateVertexLocations FindBothPlateVertexLocations(FVector2D Vertex);
	bool IsNextToBlack(FVector2D VertexLocation);

	FRunnableThread* Thread;
	bool bRunThread = true;

	TObjectPtr<AWorldLandscape> OwnerLandscape = nullptr;
	UWorld* CurrentWorld = nullptr;

	TArray<int32> Verticies;
	/* Sort of like a height map, but not a map. Maybe make it a map? */
	TArray<double> VertexHeights;

	double LastRenderDistance = -1;
	FVector LocalClientPawnLocation = FVector::ZeroVector;
	int64 NumPointsPerLine = -1;

	TMap<FVector2D, FVector2D> MasterVertexCache;
};
