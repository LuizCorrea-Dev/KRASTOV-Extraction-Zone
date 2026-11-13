#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "KrastovItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;

UENUM(BlueprintType)
enum class EItemCategory : uint8 {
  Generic UMETA(DisplayName = "Generico"),
  WeaponLong UMETA(DisplayName = "Arma Longa"),
  WeaponPistol UMETA(DisplayName = "Pistola"),
  Artifact UMETA(DisplayName = "Artefato"),
  Medical UMETA(DisplayName = "Medico"),
  GasMask UMETA(DisplayName = "Mascara de Gas"),
  Filter UMETA(DisplayName = "Filtro"),
  Suit UMETA(DisplayName = "Traje"),
  KeyItem UMETA(DisplayName = "Item-Chave"),
  Ammo UMETA(DisplayName = "Municao")
};

UENUM(BlueprintType)
enum class EArtifactElement : uint8 {
  None,
  RadGreen UMETA(DisplayName = "RAD_GREEN"),
  ElecPurple UMETA(DisplayName = "ELEC_PURPLE"),
  CryoBlue UMETA(DisplayName = "CRYO_BLUE")
};

/** Definição de um tipo de item (um Data Asset por item: DA_AK74M,
 * DA_Isotope_Zaria...) */
UCLASS(BlueprintType)
class KRASTOVEXTRACTION_API UKrastovItemDefinition : public UPrimaryDataAsset {
  GENERATED_BODY()

public:
  /** Identificador único, usado nos comandos de teste (ex.: AK74M) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
  FName ItemId;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
  FText DisplayName;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",
            meta = (MultiLine = true))
  FText Description;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
  EItemCategory Category = EItemCategory::Generic;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",
            meta = (EditCondition = "Category == EItemCategory::Artifact",
                    EditConditionHides))
  EArtifactElement Element = EArtifactElement::None;

  /** Peso em kg (soma no CarryWeight) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",
            meta = (ClampMin = "0"))
  float Weight = 0.5f;

  /** Tamanho na grade: X = colunas, Y = linhas (AK = 3x1) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid",
            meta = (ClampMin = "1", ClampMax = "8"))
  FIntPoint Size = FIntPoint(1, 1);

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid")
  bool bCanRotate = true;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
  TObjectPtr<UTexture2D> Icon;

  /** Mesh usado quando o item está no chão da Zona (Etapa 4.4) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
  TObjectPtr<UStaticMesh> WorldMesh;

  UFUNCTION(BlueprintPure, Category = "Krastov|Item")
  bool IsWeapon() const;

  UFUNCTION(BlueprintPure, Category = "Krastov|Item")
  bool IsArtifact() const;

  virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

/** Um item concreto dentro de uma grade */
USTRUCT(BlueprintType)
struct KRASTOVEXTRACTION_API FInventoryItem {
  GENERATED_BODY()

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
  FGuid InstanceId;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
  TObjectPtr<UKrastovItemDefinition> Definition = nullptr;

  /** Canto superior esquerdo na grade */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
  FIntPoint Position = FIntPoint::ZeroValue;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
  bool bRotated = false;

  /** Tamanho ocupado considerando a rotação */
  FIntPoint GetFootprint() const {
    if (!Definition) {
      return FIntPoint(1, 1);
    }
    return bRotated ? FIntPoint(Definition->Size.Y, Definition->Size.X)
                    : Definition->Size;
  }

  bool ContainsCell(const FIntPoint &Cell) const {
    const FIntPoint F = GetFootprint();
    return Cell.X >= Position.X && Cell.X < Position.X + F.X &&
           Cell.Y >= Position.Y && Cell.Y < Position.Y + F.Y;
  }
};
