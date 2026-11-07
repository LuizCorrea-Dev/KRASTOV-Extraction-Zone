#include "Zone/ZoneGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AZoneGenerator::AZoneGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	FloorInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorInstances"));
	FloorInstances->SetupAttachment(Root);
	FloorInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	WallInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallInstances"));
	WallInstances->SetupAttachment(Root);
	WallInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// Greybox: cubo da engine (100x100x100, pivô no centro)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FloorInstances->SetStaticMesh(CubeMesh.Object);
		WallInstances->SetStaticMesh(CubeMesh.Object);
	}
}

void AZoneGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		if (bRandomSeedOnBeginPlay)
		{
			Seed = FMath::Rand();
		}
		Generate();
	}

	if (bTeleportPlayerToSpawn && Cells.Num() > 0)
	{
		// Próximo tick: garante que o pawn do jogador já foi spawnado
		GetWorldTimerManager().SetTimerForNextTick(this, &AZoneGenerator::TeleportPlayer);
	}
}

// ============================================================ Geração

void AZoneGenerator::Generate()
{
	Modify();

	RoomMaxSize = FMath::Max(RoomMaxSize, RoomMinSize);

	Cells.Init(FZoneCell(), GridWidth * GridHeight);
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			Cells[Index(X, Y)].Coord = FIntPoint(X, Y);
		}
	}

	FRandomStream Stream(Seed);

	CarveMaze(Stream);
	AddLoops(Stream);
	CarveRooms(Stream);

	// Spawn na borda -X (parte "de baixo" da tela)
	SpawnCell = FIntPoint(0, Stream.RandRange(0, GridHeight - 1));
	ComputeDistances();

	// Extração = célula mais distante (em passos) do Spawn
	int32 BestDistance = -1;
	for (FZoneCell& Cell : Cells)
	{
		if (Cell.DistanceFromSpawn > BestDistance)
		{
			BestDistance = Cell.DistanceFromSpawn;
			ExtractionCell = Cell.Coord;
		}

		Cell.bIsSafe = Cell.DistanceFromSpawn != INDEX_NONE && Cell.DistanceFromSpawn <= SafeRadius;
		GetTileForMask(Cell.OpenMask, Cell.TileType, Cell.TileYaw);
	}

	BuildGeometry();

	if (ReachableCells != Cells.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[Zone] Seed %d: apenas %d/%d celulas alcancaveis!"), Seed, ReachableCells, Cells.Num());
	}
	UE_LOG(LogTemp, Log, TEXT("[Zone] Seed=%d  Grid=%dx%d  Alcancaveis=%d/%d  Spawn=(%d,%d)  Extracao=(%d,%d) a %d passos"),
		Seed, GridWidth, GridHeight, ReachableCells, Cells.Num(),
		SpawnCell.X, SpawnCell.Y, ExtractionCell.X, ExtractionCell.Y, BestDistance);

	OnZoneGenerated(GetSpawnLocation(), GetExtractionLocation());
}

void AZoneGenerator::GenerateWithRandomSeed()
{
	Seed = FMath::Rand();
	Generate();
}

void AZoneGenerator::ClearZone()
{
	Modify();
	Cells.Reset();
	FloorInstances->ClearInstances();
	WallInstances->ClearInstances();
}

void AZoneGenerator::Open(int32 X, int32 Y, int32 Dir)
{
	const FIntPoint N = FIntPoint(X, Y) + KrastovZone::DirOffset[Dir];
	if (!InBounds(X, Y) || !InBounds(N.X, N.Y))
	{
		return;
	}
	Cells[Index(X, Y)].OpenMask |= KrastovZone::DirBit(Dir);
	Cells[Index(N.X, N.Y)].OpenMask |= KrastovZone::DirBit(KrastovZone::Opposite(Dir));
}

void AZoneGenerator::CarveMaze(FRandomStream& Stream)
{
	// Recursive backtracker iterativo: gera um labirinto "perfeito" (tudo conectado)
	TArray<bool> Visited;
	Visited.Init(false, Cells.Num());

	TArray<FIntPoint> Stack;
	const FIntPoint Start(Stream.RandRange(0, GridWidth - 1), Stream.RandRange(0, GridHeight - 1));
	Stack.Push(Start);
	Visited[Index(Start.X, Start.Y)] = true;

	while (Stack.Num() > 0)
	{
		const FIntPoint Current = Stack.Last();

		int32 Candidates[KrastovZone::DirCount];
		int32 NumCandidates = 0;
		for (int32 Dir = 0; Dir < KrastovZone::DirCount; ++Dir)
		{
			const FIntPoint N = Current + KrastovZone::DirOffset[Dir];
			if (InBounds(N.X, N.Y) && !Visited[Index(N.X, N.Y)])
			{
				Candidates[NumCandidates++] = Dir;
			}
		}

		if (NumCandidates == 0)
		{
			Stack.Pop();
			continue;
		}

		const int32 Dir = Candidates[Stream.RandRange(0, NumCandidates - 1)];
		const FIntPoint Next = Current + KrastovZone::DirOffset[Dir];
		Open(Current.X, Current.Y, Dir);
		Visited[Index(Next.X, Next.Y)] = true;
		Stack.Push(Next);
	}
}

void AZoneGenerator::AddLoops(FRandomStream& Stream)
{
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			// Só +X e +Y: cada parede interna é avaliada uma única vez
			for (int32 Dir = 0; Dir <= 1; ++Dir)
			{
				const FIntPoint N = FIntPoint(X, Y) + KrastovZone::DirOffset[Dir];
				const bool bClosed = (Cells[Index(X, Y)].OpenMask & KrastovZone::DirBit(Dir)) == 0;
				if (bClosed && InBounds(N.X, N.Y) && Stream.FRand() < LoopChance)
				{
					Open(X, Y, Dir);
				}
			}
		}
	}
}

void AZoneGenerator::CarveRooms(FRandomStream& Stream)
{
	for (int32 R = 0; R < RoomCount; ++R)
	{
		const int32 W = Stream.RandRange(RoomMinSize, RoomMaxSize);
		const int32 H = Stream.RandRange(RoomMinSize, RoomMaxSize);
		if (W > GridWidth || H > GridHeight)
		{
			continue;
		}

		const int32 X0 = Stream.RandRange(0, GridWidth - W);
		const int32 Y0 = Stream.RandRange(0, GridHeight - H);

		for (int32 Y = Y0; Y < Y0 + H; ++Y)
		{
			for (int32 X = X0; X < X0 + W; ++X)
			{
				Cells[Index(X, Y)].bIsRoom = true;
				if (X + 1 < X0 + W) Open(X, Y, 0); // +X
				if (Y + 1 < Y0 + H) Open(X, Y, 1); // +Y
			}
		}
	}
}

void AZoneGenerator::ComputeDistances()
{
	for (FZoneCell& Cell : Cells)
	{
		Cell.DistanceFromSpawn = INDEX_NONE;
	}

	TArray<FIntPoint> Queue;
	Queue.Reserve(Cells.Num());
	Queue.Add(SpawnCell);
	Cells[Index(SpawnCell.X, SpawnCell.Y)].DistanceFromSpawn = 0;

	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const FIntPoint Current = Queue[Head];
		const uint8 Mask = Cells[Index(Current.X, Current.Y)].OpenMask;
		const int32 CurrentDist = Cells[Index(Current.X, Current.Y)].DistanceFromSpawn;

		for (int32 Dir = 0; Dir < KrastovZone::DirCount; ++Dir)
		{
			if ((Mask & KrastovZone::DirBit(Dir)) == 0)
			{
				continue;
			}
			const FIntPoint N = Current + KrastovZone::DirOffset[Dir];
			FZoneCell& Neighbor = Cells[Index(N.X, N.Y)];
			if (Neighbor.DistanceFromSpawn == INDEX_NONE)
			{
				Neighbor.DistanceFromSpawn = CurrentDist + 1;
				Queue.Add(N);
			}
		}
	}

	ReachableCells = Queue.Num();
}

// ============================================================ Bitmask → peça

void AZoneGenerator::GetTileForMask(uint8 Mask, EZoneTileType& OutType, float& OutYaw)
{
	Mask &= 0xF;
	OutType = EZoneTileType::Solid;
	OutYaw = 0.f;
	if (Mask == 0)
	{
		return;
	}

	// Girar 90° no Unreal leva +X para +Y: bit i → bit i+1
	auto Rotate = [](uint8 M, int32 Steps) -> uint8
		{
			return static_cast<uint8>(((M << Steps) | (M >> (4 - Steps))) & 0xF);
		};

	struct FBaseTile { EZoneTileType Type; uint8 Mask; };
	static const FBaseTile Bases[] =
	{
		{ EZoneTileType::DeadEnd,   0b0001 }, // abre para +X
		{ EZoneTileType::Straight,  0b0101 }, // +X e -X
		{ EZoneTileType::Corner,    0b0011 }, // +X e +Y
		{ EZoneTileType::TJunction, 0b0111 }, // fechado em -Y
		{ EZoneTileType::Cross,     0b1111 },
	};

	for (const FBaseTile& Base : Bases)
	{
		for (int32 Steps = 0; Steps < 4; ++Steps)
		{
			if (Rotate(Base.Mask, Steps) == Mask)
			{
				OutType = Base.Type;
				OutYaw = 90.f * Steps;
				return;
			}
		}
	}
}

// ============================================================ Geometria

FVector AZoneGenerator::MeshSize(const UInstancedStaticMeshComponent* ISM) const
{
	if (ISM && ISM->GetStaticMesh())
	{
		return (ISM->GetStaticMesh()->GetBounds().BoxExtent * 2.f).ComponentMax(FVector(1.f));
	}
	return FVector(100.f);
}

FVector AZoneGenerator::CellLocal(FIntPoint Coord) const
{
	return FVector((Coord.X + 0.5f) * CellSize, (Coord.Y + 0.5f) * CellSize, 0.f);
}

void AZoneGenerator::BuildGeometry()
{
	FloorInstances->ClearInstances();
	WallInstances->ClearInstances();

	const FVector FloorMesh = MeshSize(FloorInstances);
	const FVector WallMesh = MeshSize(WallInstances);
	const float HalfCell = CellSize * 0.5f;
	const float WallZ = WallHeight * 0.5f;

	// Paredes paralelas a Y (bordas em X) e paralelas a X (bordas em Y)
	const FVector WallAlongY(WallThickness, CellSize + WallThickness, WallHeight);
	const FVector WallAlongX(CellSize + WallThickness, WallThickness, WallHeight);

	TArray<FTransform> Floors;
	TArray<FTransform> Walls;
	Floors.Reserve(Cells.Num());

	auto AddWall = [&](const FVector& Location, const FVector& Size)
		{
			Walls.Add(FTransform(FRotator::ZeroRotator, Location, Size / WallMesh));
		};

	for (const FZoneCell& Cell : Cells)
	{
		const FVector Center = CellLocal(Cell.Coord);

		Floors.Add(FTransform(FRotator::ZeroRotator,
			Center - FVector(0.f, 0.f, FloorThickness * 0.5f),
			FVector(CellSize, CellSize, FloorThickness) / FloorMesh));

		if ((Cell.OpenMask & KrastovZone::DirBit(0)) == 0) AddWall(Center + FVector(HalfCell, 0.f, WallZ), WallAlongY); // +X
		if ((Cell.OpenMask & KrastovZone::DirBit(1)) == 0) AddWall(Center + FVector(0.f, HalfCell, WallZ), WallAlongX); // +Y
		if (Cell.Coord.X == 0)                              AddWall(Center + FVector(-HalfCell, 0.f, WallZ), WallAlongY); // borda -X
		if (Cell.Coord.Y == 0)                              AddWall(Center + FVector(0.f, -HalfCell, WallZ), WallAlongX); // borda -Y
	}

	FloorInstances->AddInstances(Floors, false);
	WallInstances->AddInstances(Walls, false);
}

// ============================================================ Consultas

FVector AZoneGenerator::CellToWorld(FIntPoint Coord) const
{
	return GetActorTransform().TransformPosition(CellLocal(Coord));
}

FVector AZoneGenerator::GetSpawnLocation() const { return CellToWorld(SpawnCell); }
FVector AZoneGenerator::GetExtractionLocation() const { return CellToWorld(ExtractionCell); }

TArray<FVector> AZoneGenerator::GetRoomCellLocations() const
{
	TArray<FVector> Result;
	for (const FZoneCell& Cell : Cells)
	{
		if (Cell.bIsRoom)
		{
			Result.Add(CellToWorld(Cell.Coord));
		}
	}
	return Result;
}

bool AZoneGenerator::IsLocationInSafeArea(FVector WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	const int32 X = FMath::FloorToInt(Local.X / CellSize);
	const int32 Y = FMath::FloorToInt(Local.Y / CellSize);
	return InBounds(X, Y) && Cells.IsValidIndex(Index(X, Y)) && Cells[Index(X, Y)].bIsSafe;
}

void AZoneGenerator::TeleportPlayer()
{
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Pawn->TeleportTo(GetSpawnLocation() + FVector(0.f, 0.f, 120.f), Pawn->GetActorRotation());
	}
}
