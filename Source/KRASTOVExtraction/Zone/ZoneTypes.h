#pragma once

#include "CoreMinimal.h"
#include "ZoneTypes.generated.h"

/** Direções da grade. +X = "cima" na tela (câmera top-down com Yaw 0). */
namespace KrastovZone
{
	constexpr int32 DirCount = 4;   // 0:+X  1:+Y  2:-X  3:-Y
	inline const FIntPoint DirOffset[DirCount] = { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) };

	inline uint8 DirBit(int32 Dir) { return static_cast<uint8>(1 << Dir); }
	inline int32 Opposite(int32 Dir) { return (Dir + 2) % DirCount; }
}

UENUM(BlueprintType)
enum class EZoneTileType : uint8
{
	Solid,      // sem aberturas
	DeadEnd,    // 1 abertura
	Straight,   // 2 opostas
	Corner,     // 2 adjacentes
	TJunction,  // 3
	Cross       // 4
};

USTRUCT(BlueprintType)
struct FZoneCell
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FIntPoint Coord = FIntPoint::ZeroValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 OpenMask = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsRoom = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsSafe = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 DistanceFromSpawn = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EZoneTileType TileType = EZoneTileType::Solid;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float TileYaw = 0.f;
};
