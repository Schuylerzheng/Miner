// Copyright Schuyler Zheng. All Rights Reserved.

#include "WorldGenerationRunnable.h"
#include "WorldLandscape.h"
#include "ThirdPartyLibraries/FastNoiseLite.h"

DEFINE_LOG_CATEGORY(LogWorldGenerationThread);

#pragma region Main Thread Code
FWorldGenerationRunnable::FWorldGenerationRunnable(AWorldLandscape* WorldLandscape, UWorld* World)
{
	OwnerLandscape = WorldLandscape;
	CurrentWorld = World;
	DynamicMesh = OwnerLandscape->AllocateComputeMesh();

	Thread = FRunnableThread::Create(this, TEXT("World Generation Thread"));
}

FWorldGenerationRunnable::~FWorldGenerationRunnable()
{
	if (Thread) {
		Thread->Kill();
		delete Thread;
	}
}
#pragma endregion

bool FWorldGenerationRunnable::Init()
{
	UE_LOG(LogWorldGenerationThread, Log, TEXT("World Generation Thread Initialized."));

	return true;
}

uint32 FWorldGenerationRunnable::Run()
{
	check(IsValid(CurrentWorld));
	if (!CurrentWorld->IsGameWorld()) { return 1; }

	while (bRunThread) {
		if (bGenerate) { GenerateDynamicMesh(); }
	}

	return 0;
}

void FWorldGenerationRunnable::Stop()
{
	bRunThread = false;
}

void FWorldGenerationRunnable::Exit()
{

}

FSingleThreadRunnable* FWorldGenerationRunnable::GetSingleThreadInterface()
{
	return nullptr;
}

void FWorldGenerationRunnable::GenerateDynamicMesh()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenerateDynamicMesh);

	check(IsValid(OwnerLandscape));

	DynamicMesh->InitializeMesh();

	// Empty arrays 
	Verticies.Empty();
	VertexHeights.Empty();

	if (MasterVertexCache.Num() > OwnerLandscape->MaxMasterVertexCacheSize) MasterVertexCache.Empty();

	// Local Client Pawn Location always changes
	LocalClientPawnLocation = (CurrentWorld->IsGameWorld()) ? OwnerLandscape->LocalClientPawn->GetActorLocation() : FVector3d::ZeroVector;

	// These things only change if render distance changes
	if (OwnerLandscape->RenderDistance != LastRenderDistance) {
		NumPointsPerLine = FMath::FloorToInt((OwnerLandscape->RenderDistance * 2.0f) / OwnerLandscape->Resolution) + 1;
		int64 ReserveCount = NumPointsPerLine * NumPointsPerLine;    // You apparently can't use ^ for power, it is bitwise, and the Unreal power function requires floats
		if (NumPointsPerLine > TNumericLimits<int32>::Max()) ReserveCount = TNumericLimits<int32>::Max();
		Verticies.Reserve((int32)ReserveCount);
		VertexHeights.Reserve((int32)ReserveCount);
		LastRenderDistance = OwnerLandscape->RenderDistance;
	}

	check(LastRenderDistance > 0);

	// Run the terrain generation steps
	GenerateBasicHeights();
	ApplyPlateTectonics();
	FinalizeLandMesh();

	// signal the worker finished + make sure it doesn run again until requested
	bGenerate = false;
	// Move the completed local mesh to the UDynamicMesh on the game thread and then broadcast the delegate.
	TWeakObjectPtr<AWorldLandscape> WeakWorldLandscapeReference(OwnerLandscape);
	AsyncTask(ENamedThreads::GameThread, [WeakWorldLandscapeReference]() {
		if (WeakWorldLandscapeReference.IsValid() && IsValid(WeakWorldLandscapeReference->DynamicMesh))
		{
			// Broadcast that terrain data is ready (must be done on game thread?)
			WeakWorldLandscapeReference->ApplyTerrainDataDelegate.Broadcast();
		}
	});
}

void FWorldGenerationRunnable::GenerateBasicHeights()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenerateBasicHeights);

	ModifyHeightArray([&](FVector LocalVertexLocation) -> double {
		float Height = OwnerLandscape->BasicLandNoise->GetNoise(LocalVertexLocation.X + LocalClientPawnLocation.X / 50, LocalVertexLocation.Y + LocalClientPawnLocation.Y / 50) * OwnerLandscape->HeightScale;

		return Height;
		});
}

void FWorldGenerationRunnable::ApplyPlateTectonics()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ApplyPlateTectonics);

	ModifyHeightArray([&](FVector LocalVertexLocation) -> double {
		double FinalHeight = LocalVertexLocation.Z;
		FVector2D WorldVertexLocation = FVector2D(LocalVertexLocation.X + LocalClientPawnLocation.X / 50, LocalVertexLocation.Y + LocalClientPawnLocation.Y / 50);
		
		double CurrentNoiseValue = FMath::Abs(OwnerLandscape->PlateTectonicsNoise->GetNoise(WorldVertexLocation.X, WorldVertexLocation.Y));    // For some reason the noise can be negative, so make it absolute value
		bool KeepCheckingForNextPlateOver = true;
		FPlateVertexLocations PlateVertexLocations = FindBothPlateVertexLocations(WorldVertexLocation);
		FVector2d MasterVertexLocation1 = FindMasterVertexOfPlate(PlateVertexLocations.Plate1VertexLocation);
		FVector2d MasterVertexLocation2 = FindMasterVertexOfPlate(PlateVertexLocations.Plate2VertexLocation);
		FRandomStream Plate1RandomStream(MasterVertexLocation1.X * MasterVertexLocation1.Y);    // Not the best way of making a random seed for the random generator, but that is for later. 
		FRandomStream Plate2RandomStream(MasterVertexLocation2.X * MasterVertexLocation2.Y);
		double PlateSpeed1 = Plate1RandomStream.RandRange(OwnerLandscape->MinPlateSpeed, OwnerLandscape->MaxPlateSpeed);
		double PlateSpeed2 = Plate2RandomStream.RandRange(OwnerLandscape->MinPlateSpeed, OwnerLandscape->MaxPlateSpeed);

		switch (ArePlatesColliding(MasterVertexLocation1, MasterVertexLocation2))
		{
			case ECollisionType::Push:
				FinalHeight += CurrentNoiseValue * OwnerLandscape->PlateTectonicsHeightScale * FMath::Fmod(PlateSpeed1 + PlateSpeed2, OwnerLandscape->MaxPlateSpeed);
				break;
			case ECollisionType::Pull:
				FinalHeight -= CurrentNoiseValue * OwnerLandscape->PlateTectonicsHeightScale * FMath::Fmod(PlateSpeed1 + PlateSpeed2, OwnerLandscape->MaxPlateSpeed);
				break;
			case ECollisionType::Slide:
				// Tmp copilot thing
				FinalHeight += (CurrentNoiseValue * OwnerLandscape->PlateTectonicsHeightScale * FMath::Fmod(FMath::Abs(PlateSpeed1 - PlateSpeed2), OwnerLandscape->MaxPlateSpeed)) / 2.0f;
				break;
			default:
				// None collision type
				break;
		}

		return FinalHeight;
	});
}

void FWorldGenerationRunnable::FinalizeLandMesh()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FinalizeLandMesh);

	int Index = 0;

	// Make the verticies
	for (int IndexY = 0; IndexY < NumPointsPerLine; ++IndexY) {
		float VertexY = -OwnerLandscape->RenderDistance + IndexY * OwnerLandscape->Resolution;

		for (int IndexX = 0; IndexX < NumPointsPerLine; ++IndexX) {
			float VertexX = -OwnerLandscape->RenderDistance + IndexX * OwnerLandscape->Resolution;

			// create vertex and remember its index (use local mesh)
			int32 NewVertex = (int32)DynamicMesh->GetMeshPtr()->AppendVertex(FVector(VertexX + LocalClientPawnLocation.X / 50, VertexY + LocalClientPawnLocation.Y / 50, VertexHeights[Index]));
			check(DynamicMesh->GetMeshPtr()->IsVertex(NewVertex));
			Verticies.Add(NewVertex);
			Index++;
		}
	}

	// Make the triangles
	for (int IndexY = 0; IndexY < NumPointsPerLine - 1; ++IndexY) {
		for (int IndexX = 0; IndexX < NumPointsPerLine - 1; ++IndexX) {
			// Apparently making them into individual variables instead of an array is better
			const int TopLeftVertex = Verticies[IndexX + IndexY * NumPointsPerLine];             // top left
			const int TopRightVertex = Verticies[(IndexX + 1) + IndexY * NumPointsPerLine];       // top right
			const int BottomLeftVertex = Verticies[IndexX + (IndexY + 1) * NumPointsPerLine];       // bottom left
			const int BottomRightVertex = Verticies[(IndexX + 1) + (IndexY + 1) * NumPointsPerLine]; // bottom right

			// First triangle (top-left, bottom-left, bottom-right)
			DynamicMesh->GetMeshPtr()->AppendTriangle(TopLeftVertex, BottomLeftVertex, BottomRightVertex);

			// Second triangle (top-left, bottom-right, top-right)
			DynamicMesh->GetMeshPtr()->AppendTriangle(TopLeftVertex, BottomRightVertex, TopRightVertex);
		}
	}
}

void FWorldGenerationRunnable::ModifyHeightArray(TFunctionRef<double(FVector)> ModifyFunc)
{
	int Index = 0;

	int64 ReserveCount = NumPointsPerLine * NumPointsPerLine;
	if (NumPointsPerLine > TNumericLimits<int32>::Max()) ReserveCount = TNumericLimits<int32>::Max();
	TArray<double> NewVertexHeights;
	NewVertexHeights.Reserve((int32)ReserveCount);

	for (int IndexY = 0; IndexY < NumPointsPerLine; ++IndexY) {
		float VertexY = -OwnerLandscape->RenderDistance + IndexY * OwnerLandscape->Resolution;

		for (int IndexX = 0; IndexX < NumPointsPerLine; ++IndexX) {
			float VertexX = -OwnerLandscape->RenderDistance + IndexX * OwnerLandscape->Resolution;

			const double CurrentVertexHeight = !VertexHeights.IsEmpty() ? VertexHeights[Index] : 0;
			const FVector CurrentVertexLocation = FVector(VertexX, VertexY, CurrentVertexHeight);    // Do all the calculations in local space, then transfer to world space later when finalizing the mesh

			double NewVertexHeight = ModifyFunc(CurrentVertexLocation);
			NewVertexHeights.Add(NewVertexHeight);
			Index++;
		}
	}

	VertexHeights = MoveTemp(NewVertexHeights);
}

FVector2D FWorldGenerationRunnable::FindMasterVertexOfPlate(FVector2D BoarderVertexLocation)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FindMasterVertexOfPlate);

	if (MasterVertexCache.Contains(BoarderVertexLocation)) {
		return MasterVertexCache[BoarderVertexLocation];
	}

	TSet<FVector2D> AttemptedPoints;
	FVector2D MostTopLeftPoint = BoarderVertexLocation;
	double StepDistance = OwnerLandscape->Resolution;
	int IterationsDone = 0;

	while (IterationsDone < OwnerLandscape->MasterVertexCheckAttempts || OwnerLandscape->MasterVertexCheckAttempts <= 0) {
		IterationsDone++;
		AttemptedPoints.Add(MostTopLeftPoint);

		// Check up
		FVector2D CurrentLocation = FVector2D(MostTopLeftPoint.X, MostTopLeftPoint.Y + StepDistance);
		if (double NoiseSample = OwnerLandscape->PlateTectonicsNoise->GetNoise(CurrentLocation.X, CurrentLocation.Y); NoiseSample >= OwnerLandscape->PlateBoarderThreshhold && IsNextToBlack(CurrentLocation)) {
			MostTopLeftPoint = FVector2D(MostTopLeftPoint.X, MostTopLeftPoint.Y + StepDistance);
			IterationsDone = 0;
			continue;
		}

		// Check left
		CurrentLocation = FVector2D(MostTopLeftPoint.X - StepDistance, MostTopLeftPoint.Y);
		if (double NoiseSample = OwnerLandscape->PlateTectonicsNoise->GetNoise(CurrentLocation.X, CurrentLocation.Y); NoiseSample >= OwnerLandscape->PlateBoarderThreshhold && IsNextToBlack(CurrentLocation)) {
			MostTopLeftPoint = FVector2D(MostTopLeftPoint.X - StepDistance, MostTopLeftPoint.Y);
			IterationsDone = 0;
			continue;
		}

		// Could not find, so stop
		break;
	}

	for (const FVector2D& AttemptedPoint : AttemptedPoints) {
		MasterVertexCache.Add(AttemptedPoint, MostTopLeftPoint);
	}

	return MostTopLeftPoint;
}

ECollisionType FWorldGenerationRunnable::ArePlatesColliding(FVector2D MasterVertexLocation1, FVector2D MasterVertexLocation2)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ArePlatesColliding);

	// Create deterministic directions based on master locations
	FRandomStream Plate1RandomStream(MasterVertexLocation1.X * MasterVertexLocation1.Y);
	FRandomStream Plate2RandomStream(MasterVertexLocation2.X * MasterVertexLocation2.Y);

	// Ensure we produce valid enum values (0..3)
	EPlateDirection Plate1Direction = static_cast<EPlateDirection>(Plate1RandomStream.RandRange(0, 3));
	EPlateDirection Plate2Direction = static_cast<EPlateDirection>(Plate2RandomStream.RandRange(0, 3));

	// Vector from plate1 to plate2
	const FVector2D Delta = MasterVertexLocation2 - MasterVertexLocation1;

	// If points coincide, fallback to Pull (consistent with previous default)
	if (FMath::IsNearlyZero(Delta.X) && FMath::IsNearlyZero(Delta.Y)) {
		return ECollisionType::Pull;
	}

	// Choose primary axis of separation
	const bool bHorizontalSeparation = FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y);

	// Helper lambdas for direction checks (ok copilot?)
	auto IsTowardEachOther_EW = [&](bool bMaster2IsEast) -> bool {
		if (bMaster2IsEast) {
			return (Plate1Direction == EPlateDirection::East && Plate2Direction == EPlateDirection::West);
		} else {
			return (Plate1Direction == EPlateDirection::West && Plate2Direction == EPlateDirection::East);
		}
	};
	auto IsAwayFromEachOther_EW = [&](bool bMaster2IsEast) -> bool {
		if (bMaster2IsEast) {
			return (Plate1Direction == EPlateDirection::West && Plate2Direction == EPlateDirection::East);
		} else {
			return (Plate1Direction == EPlateDirection::East && Plate2Direction == EPlateDirection::West);
		}
	};
	auto IsTowardEachOther_NS = [&](bool bMaster2IsNorth) -> bool {
		if (bMaster2IsNorth) {
			return (Plate1Direction == EPlateDirection::North && Plate2Direction == EPlateDirection::South);
		} else {
			return (Plate1Direction == EPlateDirection::South && Plate2Direction == EPlateDirection::North);
		}
	};
	auto IsAwayFromEachOther_NS = [&](bool bMaster2IsNorth) -> bool {
		if (bMaster2IsNorth) {
			return (Plate1Direction == EPlateDirection::South && Plate2Direction == EPlateDirection::North);
		} else {
			return (Plate1Direction == EPlateDirection::North && Plate2Direction == EPlateDirection::South);
		}
	};

	if (bHorizontalSeparation) {
		const bool bMaster2IsEast = Delta.X > 0.0;
		if (IsTowardEachOther_EW(bMaster2IsEast)) {
			return ECollisionType::Push;
		}
		if (IsAwayFromEachOther_EW(bMaster2IsEast)) {
			return ECollisionType::Pull;
		}
	} else {
		const bool bMaster2IsNorth = Delta.Y > 0.0;
		if (IsTowardEachOther_NS(bMaster2IsNorth)) {
			return ECollisionType::Push;
		}
		if (IsAwayFromEachOther_NS(bMaster2IsNorth)) {
			return ECollisionType::Pull;
		}
	}

	// Parallel same-direction movement or perpendicular movement -> slide (or no strong push/pull)
	if (Plate1Direction == Plate2Direction) {
		return ECollisionType::Slide;
	}

	// Unimplemented for now
	return ECollisionType::Slide;
}

FPlateVertexLocations FWorldGenerationRunnable::FindBothPlateVertexLocations(FVector2D Vertex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FindBothPlateVertexLocations);

	FPlateVertexLocations PlateVertexLocations;

	if (!IsValid(OwnerLandscape)) {
		PlateVertexLocations.Plate1VertexLocation = Vertex;
		PlateVertexLocations.Plate2VertexLocation = Vertex;
		return PlateVertexLocations;
	}

	// Directions to probe (cardinal + diagonals)
	const TArray<FVector2D> ProbeDirs = {
		FVector2D(1, 0), FVector2D(-1, 0),
		FVector2D(0, 1), FVector2D(0, -1),
		FVector2D(1, 1), FVector2D(-1, 1),
		FVector2D(1, -1), FVector2D(-1, -1)
	};

	TArray<FVector2D> Candidates;
	const double StepDistance = OwnerLandscape->Resolution;
	// If MasterVertexCheckAttempts <= 0 the code elsewhere treats that as "no limit".
	// Cap search to a reasonable maximum to avoid runaway loops here.
	const int32 MaxSteps = (OwnerLandscape->MasterVertexCheckAttempts > 0) ? OwnerLandscape->MasterVertexCheckAttempts : 1000;

	for (const FVector2D& Dir : ProbeDirs)
	{
		for (int32 Step = 1; Step <= MaxSteps; ++Step)
		{
			const FVector2D Current = Vertex + Dir * (StepDistance * (double)Step);
			const double NoiseSample = OwnerLandscape->PlateTectonicsNoise->GetNoise(Current.X, Current.Y);

			// We're looking for border vertices (noise >= threshold) that are adjacent to "black" (non-border).
			if (NoiseSample >= OwnerLandscape->PlateBoarderThreshhold && IsNextToBlack(Current))
			{
				// Add unique candidates only (with a small epsilon to avoid duplicates)
				bool bAlreadyHave = false;
				const double Epsilon = StepDistance * 0.5;
				for (const FVector2D& Existing : Candidates)
				{
					if (FMath::Square(FVector2D::Distance(Existing, Current)) <= (Epsilon * Epsilon))
					{
						bAlreadyHave = true;
						break;
					}
				}
				if (!bAlreadyHave)
				{
					Candidates.Add(Current);
				}

				// Found the nearest border point along this probe direction -> stop probing further in this direction
				break;
			}
		}
	}

	// Fallback: if no candidates found, use the provided vertex for both
	if (Candidates.Num() == 0)
	{
		PlateVertexLocations.Plate1VertexLocation = Vertex;
		PlateVertexLocations.Plate2VertexLocation = Vertex;
		return PlateVertexLocations;
	}

	// Sort candidates by distance to the original vertex (closest first)
	Candidates.Sort([&](const FVector2D& A, const FVector2D& B) {
		return FMath::Square(FVector2D::Distance(A, Vertex)) < FMath::Square(FVector2D::Distance(B, Vertex));
	});

	PlateVertexLocations.Plate1VertexLocation = Candidates[0];
	PlateVertexLocations.Plate2VertexLocation = (Candidates.Num() > 1) ? Candidates[1] : Candidates[0];

	return PlateVertexLocations;
}

bool FWorldGenerationRunnable::IsNextToBlack(FVector2D VertexLocation)
{
	if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X - OwnerLandscape->Resolution, VertexLocation.Y) < OwnerLandscape->PlateBoarderThreshhold) {
		// Left
		return true;
	}
	else if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X + OwnerLandscape->Resolution, VertexLocation.Y) < OwnerLandscape->PlateBoarderThreshhold) {
		// Right
		return true;
	}
	else if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X, VertexLocation.Y + OwnerLandscape->Resolution) < OwnerLandscape->PlateBoarderThreshhold) {
		// Up
		return true;
	}
	else if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X, VertexLocation.Y - OwnerLandscape->Resolution) < OwnerLandscape->PlateBoarderThreshhold) {
		// Down
		return true;
	}
	else if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X - OwnerLandscape->Resolution, VertexLocation.Y + OwnerLandscape->Resolution) < OwnerLandscape->PlateBoarderThreshhold) {
		// Up Left
		return true;
	}
	else if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X + OwnerLandscape->Resolution, VertexLocation.Y + OwnerLandscape->Resolution) < OwnerLandscape->PlateBoarderThreshhold) {
		// Up Right
		return true;
	}
	else if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X - OwnerLandscape->Resolution, VertexLocation.Y - OwnerLandscape->Resolution) < OwnerLandscape->PlateBoarderThreshhold) {
		// Down Left
		return true;
	}
	else if (OwnerLandscape->PlateTectonicsNoise->GetNoise(VertexLocation.X + OwnerLandscape->Resolution, VertexLocation.Y - OwnerLandscape->Resolution) < OwnerLandscape->PlateBoarderThreshhold) {
		// Down Right
		return true;
	}

	return false;
}
