#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zone/ZoneTypes.h"
#include "ZoneGenerator.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class KRASTOVEXTRACTION_API AZoneGenerator : public AActor
{
	GENERATED_BODY()

public:
	AZoneGenerator();

	/** Gera a Zona com a Seed atual (também aparece como botão no Details) */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Krastov|Zone")
	void Generate();

	UFUNCTION(CallInEditor, Category = "Krastov|Zone")
	void GenerateWithRandomSeed();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Krastov|Zone")
	void ClearZone();

	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") FVector GetSpawnLocation() const;
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") FVector GetExtractionLocation() const;
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") FVector CellToWorld(FIntPoint Coord) const;
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") TArray<FZoneCell> GetCells() const { return Cells; }
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") TArray<FVector> GetRoomCellLocations() const;

	/** Use antes de spawnar inimigos: true = área segura do Spawn */
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone")
	bool IsLocationInSafeArea(FVector WorldLocation) const;

	/** Bitmask de aberturas → tipo de peça modular + rotação (graus) */
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone")
	static void GetTileForMask(uint8 Mask, EZoneTileType& OutType, float& OutYaw);

protected:
	virtual void BeginPlay() override;

	// ---------- Seed ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Krastov|Zone|Seed")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Seed")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Seed")
	bool bRandomSeedOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Seed")
	bool bTeleportPlayerToSpawn = true;

	// ---------- Layout ----------
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2", ClampMax = "64"))
	int32 GridWidth = 12;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2", ClampMax = "64"))
	int32 GridHeight = 12;

	/** Tamanho de cada célula em unidades (800 = 8 m) */
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "200"))
	float CellSize = 800.f;

	/** Chance de abrir paredes extras (rotas alternativas) */
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LoopChance = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "0"))
	int32 RoomCount = 4;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2"))
	int32 RoomMinSize = 2;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2"))
	int32 RoomMaxSize = 3;

	/** Células (em passos) ao redor do Spawn sem inimigos */
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "0"))
	int32 SafeRadius = 1;

	// ---------- Geometria (greybox) ----------
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Geometry") float WallHeight = 400.f;
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Geometry") float WallThickness = 40.f;
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Geometry") float FloorThickness = 20.f;

	// ---------- Componentes ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> FloorInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WallInstances;

	// ---------- Gancho para o Blueprint ----------
	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov|Zone")
	void OnZoneGenerated(FVector SpawnLocation, FVector ExtractionLocation);

private:
	int32 Index(int32 X, int32 Y) const { return Y * GridWidth + X; }
	bool InBounds(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < GridWidth && Y < GridHeight; }
	FVector CellLocal(FIntPoint Coord) const;
	FVector MeshSize(const UInstancedStaticMeshComponent* ISM) const;

	void Open(int32 X, int32 Y, int32 Dir);
	void CarveMaze(FRandomStream& Stream);
	void AddLoops(FRandomStream& Stream);
	void CarveRooms(FRandomStream& Stream);
	void ComputeDistances();
	void BuildGeometry();
	void TeleportPlayer();

	UPROPERTY(VisibleInstanceOnly, Category = "Krastov|Zone|Debug") FIntPoint SpawnCell = FIntPoint::ZeroValue;
	UPROPERTY(VisibleInstanceOnly, Category = "Krastov|Zone|Debug") FIntPoint ExtractionCell = FIntPoint::ZeroValue;
	UPROPERTY(VisibleInstanceOnly, Category = "Krastov|Zone|Debug") int32 ReachableCells = 0;
	UPROPERTY(VisibleInstanceOnly, AdvancedDisplay, Category = "Krastov|Zone|Debug") TArray<FZoneCell> Cells;
};
