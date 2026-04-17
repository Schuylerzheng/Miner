// Copyright Schuyler Zheng. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UDynamicMesh.h"
#include "Components/DynamicMeshComponent.h"
#include "ThridPartyHelpers/FastNoiseLiteTypes.h"
#include "WorldLandscape.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLandscape, Log, All);

DECLARE_MULTICAST_DELEGATE(FTerrainDataGeneratedSignature);

/**
 * AWorldLandscape is an Actor that generates a dynamic landscape mesh based on a seed
 */

class FastNoiseLite;
class FWorldGenerationRunnable;

UCLASS(ConversionRoot, ComponentWrapperClass, ClassGroup = DynamicMesh, meta = (ChildCanTick), MinimalAPI)
class AWorldLandscape : public AActor
{
	GENERATED_BODY()

public:
	AWorldLandscape();

	UFUNCTION(BlueprintCallable, Category = DynamicMeshActor)
	UDynamicMeshComponent* GetDynamicMeshComponent() const { return DynamicMeshComponent; }

	//
	// Mesh Pool support. Meshes can be locally allocated from the Mesh Pool
	// in Blueprints, and then released back to the Pool and re-used. This
	// avoids generating temporary UDynamicMesh instances that need to be
	// garbage-collected. See UDynamicMeshPool for more details.
	//

	/** Control whether the DynamicMeshPool will be created when requested via GetComputeMeshPool() */
	UPROPERTY(Category = "DynamicMeshActor|Advanced", EditAnywhere, BlueprintReadWrite)
	bool bEnableComputeMeshPool = true;

	/** Access the compute mesh pool */
	UFUNCTION(BlueprintCallable, Category = DynamicMeshActor)
	UDynamicMeshPool* GetComputeMeshPool();

	/** Request a compute mesh from the Pool, which will return a previously-allocated mesh or add and return a new one. If the Pool is disabled, a new UDynamicMesh will be allocated and returned. */
	UFUNCTION(BlueprintCallable, Category = DynamicMeshActor)
	UDynamicMesh* AllocateComputeMesh();

	/** Release a compute mesh back to the Pool */
	UFUNCTION(BlueprintCallable, Category = DynamicMeshActor)
	bool ReleaseComputeMesh(UDynamicMesh* Mesh);

	/** Release all compute meshes that the Pool has allocated */
	UFUNCTION(BlueprintCallable, Category = DynamicMeshActor)
	void ReleaseAllComputeMeshes();

	/** Release all compute meshes that the Pool has allocated, and then release them from the Pool, so that they will be garbage-collected */
	UFUNCTION(BlueprintCallable, Category = DynamicMeshActor)
	void FreeAllComputeMeshes();

	UPROPERTY(Transient)
	TObjectPtr<UDynamicMesh> DynamicMesh;

	friend class FWorldGenerationRunnable;

	FTerrainDataGeneratedSignature ApplyTerrainDataDelegate;

protected:
	virtual void BeginPlay() override; 
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetupNoise();
	void GenerateTerrain();

	void SetNoiseParameters(FastNoiseLite *& Noise, const FNoiseSettings& NoiseSettings);

	void CleanUp();

	template <class T>
	void CleanUpPointer(T Pointer)
	{
		if (Pointer)
		{
			delete Pointer;
			Pointer = nullptr;
		}
	}

	UPROPERTY(Category = DynamicMeshActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|StaticMesh", AllowPrivateAccess = "true"))
	TObjectPtr<class UDynamicMeshComponent> DynamicMeshComponent;

	/** The internal Mesh Pool, for use in DynamicMeshActor BPs. Use GetComputeMeshPool() to access this, as it will only be created on-demand if bEnableComputeMeshPool = true */
	UPROPERTY(Transient)
	TObjectPtr<UDynamicMeshPool> DynamicMeshPool;

	FastNoiseLite* BasicLandNoise = nullptr;

	FastNoiseLite* PlateTectonicsNoise = nullptr;

	//===============================================================================================================
	// Landscape Constant Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (ToolTip = "The number that controls all randomness"))
	int Seed = 1337;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "How much distance to go until checking the noise again."))
	double Resolution = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "Chunk spacing/Distance to go until make chunk follow"))
	double ChunkDistance = 1000.00f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (ToolTip = "How far the chunk should go"))
	double RenderDistance = 100.00f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "Height Scale of the Landscape"))
	double HeightScale = 10.00f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "The amount to multiply the value of plate tectonics by."))
	double PlateTectonicsHeightScale = 50;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "How big the value of the cellular noise has to be inorder to count as a plate edge.", ClampMin = 0, ClampMax = 1))
	double PlateBoarderThreshhold = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "Max attempts to check for the master vertex. If less than 1, it just keeps going."))
	int MasterVertexCheckAttempts = 1000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "Min value for plate displacement."))
	double MinPlateSpeed = 500;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "Max value for plate displacement."))
	double MaxPlateSpeed = 1000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "Max value for plate displacement."))
	int MaxMasterVertexCacheSize = 10000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape", meta = (ToolTip = "Max value for plate displacement."))
	int MaxCheckBothPlateLocationsAttempts = 100;
	//===============================================================================================================

	//===============================================================================================================
	// Materials for the landscape
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Landscape|Materials", meta = (ToolTip = "Grass Material"))
	TObjectPtr<UMaterialInterface> GrassMaterial = nullptr;
	//===============================================================================================================

	UPROPERTY(BlueprintReadOnly, meta = (Tooltip = "The local pawn for this client. Does not need to be changed by blueprints because it is automatically set at beginplay in c++."))
	APawn* LocalClientPawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Noise")
	FNoiseSettings BasicLandNoiseSettings {
		0.1f,											// Frequency
		NoiseType_Perlin, 								// Noise Type
		RotationType3D_None,							// Rotation Type 3D
		FractalType_FBm,								// Fractal Type
		3,												// Fractal Octaves
		2.0f,											// Fractal Lacunarity
		0.5f,											// Fractal Gain
		0.0f,											// Fractal Weighted Strength
		2.0f,											// Fractal Ping Pong Strength
		CellularDistanceFunction_EuclideanSq,			// Cellular Distance Function
		CellularReturnType_Distance,					// Cellular Return Type
		1.0f,											// Cellular Jitter
		DomainWarpType_OpenSimplex2,					// Domain Warp Type
		-4.5f											// Domain Warp Amp
	};

	UPROPERTY(EditAnywhere, Category = "Noise")
	FNoiseSettings PlateTectonicsNoiseSettings{
		0.01f,											// Frequency
		NoiseType_Cellular, 								// Noise Type
		RotationType3D_None,							// Rotation Type 3D
		FractalType_FBm,								// Fractal Type
		3,												// Fractal Octaves
		2.0f,											// Fractal Lacunarity
		0.5f,											// Fractal Gain
		0.0f,											// Fractal Weighted Strength
		2.0f,											// Fractal Ping Pong Strength
		CellularDistanceFunction_EuclideanSq,			// Cellular Distance Function
		CellularReturnType_Distance,					// Cellular Return Type
		1.0f,											// Cellular Jitter
		DomainWarpType_OpenSimplex2,					// Domain Warp Type
		-4.5f											// Domain Warp Amp
	};

	UE::Geometry::EValidityCheckFailMode ValidityCheckFailMode = UE::Geometry::EValidityCheckFailMode::Ensure;

	FDynamicMesh3::FValidityOptions ValidityOptions = { false, false };

	FWorldGenerationRunnable* WorldGenerationRunnable = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Landscape Generation")
	TArray<FVector> GeneratedVertexLocations;

	UPROPERTY(BlueprintReadWrite)
	FVector LastPlayerLocation = FVector::ZeroVector;
};
