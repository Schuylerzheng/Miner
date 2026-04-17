#pragma once

#include "CoreMinimal.h"
#include "LandscapeGenerationHelpers.generated.h"

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
	Push,
	Pull,
	Slide,    // Not implemented yet, and needs direction
	None
};

USTRUCT(BlueprintType)
struct FPlateVertexLocations
{
	GENERATED_BODY()

public:
	/* Plate 1 is the closest plate vertex to the point */
	FVector2D Plate1VertexLocation;
	/* Plate 2 is the farther plate vertex to the point */
	FVector2D Plate2VertexLocation;
};
