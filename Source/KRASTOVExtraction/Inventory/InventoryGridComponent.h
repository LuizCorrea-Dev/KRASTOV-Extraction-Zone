#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/KrastovItemDefinition.h"

#include "InventoryGridComponent.generated.h"

class UInventoryGridComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged,
                                            UInventoryGridComponent *, Grid);

UENUM(BlueprintType)
enum class ESynergyLinkType : uint8 {
  ArtifactToWeapon UMETA(DisplayName = "Artefato -> Arma"),
  ArtifactToArtifact UMETA(DisplayName = "Artefato <-> Artefato"),
  ArtifactToMedical UMETA(DisplayName = "Artefato -> Medico")
};

/** Uma conexão neon entre dois itens vizinhos (a UI desenha a linha entre CellA
 * e CellB) */
USTRUCT(BlueprintType)
struct KRASTOVEXTRACTION_API FSynergyLink {
  GENERATED_BODY()

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  FGuid ItemA;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  FGuid ItemB;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  FIntPoint CellA = FIntPoint::ZeroValue;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  FIntPoint CellB = FIntPoint::ZeroValue;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  ESynergyLinkType Type = ESynergyLinkType::ArtifactToWeapon;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  EArtifactElement ElementA = EArtifactElement::None;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  EArtifactElement ElementB = EArtifactElement::None;
};

/** Resultado das sinergias para uma arma (usado pela balística na Etapa 5) */
USTRUCT(BlueprintType)
struct KRASTOVEXTRACTION_API FWeaponSynergy {
  GENERATED_BODY()

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  FGuid WeaponId;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  int32 ArtifactCount = 0;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  bool bRadioactive = false;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  bool bElectric = false;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  bool bCryo = false;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  float NodeMultiplier = 1.f;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  float DamageMultiplier = 1.f;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  float FireRateMultiplier = 1.f;
};

USTRUCT(BlueprintType)
struct KRASTOVEXTRACTION_API FInventorySynergyReport {
  GENERATED_BODY()

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  TArray<FSynergyLink> Links;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  TArray<FWeaponSynergy> Weapons;

  /** Itens médicos encostados em artefatos (cura multiplicada) */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Synergy")
  TArray<FGuid> BoostedMedicalItems;
};

/** Uma grade de inventário (Maleta 8x5 ou Safe Pocket 4x3) */
UCLASS(ClassGroup = (Krastov), meta = (BlueprintSpawnableComponent))
class KRASTOVEXTRACTION_API UInventoryGridComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UInventoryGridComponent();

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Inventory")
  FText GridName;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Inventory",
            meta = (ClampMin = "1", ClampMax = "16"))
  int32 GridWidth = 8;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Inventory",
            meta = (ClampMin = "1", ClampMax = "16"))
  int32 GridHeight = 5;

  /** Itens desta grade geram sinergias? (Safe Pocket = não, blindagem de
   * chumbo) */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Inventory")
  bool bTransmitsSynergy = true;

  /** Itens desta grade sobrevivem à morte do Operador? */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Inventory")
  bool bKeptOnDeath = false;

  // ---------- Ajuste das sinergias ----------
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Synergy")
  float RadDamageBonus = 0.35f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Synergy")
  float ElecFireRateBonus = 0.25f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Synergy")
  float CryoDamageBonus = 0.10f;

  /** Multiplicador por artefato extra encadeado (2 = x1.15, 3 = x1.32...) */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Synergy")
  float NodeMultiplierPerExtraArtifact = 1.15f;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Krastov|Synergy")
  float MedicalHealingMultiplier = 1.35f;

  UPROPERTY(BlueprintAssignable, Category = "Krastov|Inventory")
  FOnInventoryChanged OnInventoryChanged;

  // ---------- Grade ----------
  /** O item cabe nesta posição? IgnoreItem = item que está sendo movido (pode
   * ser vazio) */
  UFUNCTION(BlueprintPure, Category = "Krastov|Inventory")
  bool CanPlace(const UKrastovItemDefinition *Definition, FIntPoint Position,
                bool bRotated, FGuid IgnoreItem) const;

  /** Procura o primeiro espaço livre (tenta normal e depois girado) */
  UFUNCTION(BlueprintPure, Category = "Krastov|Inventory")
  bool FindFreeSpot(const UKrastovItemDefinition *Definition,
                    FIntPoint &OutPosition, bool &bOutRotated) const;

  UFUNCTION(BlueprintCallable, Category = "Krastov|Inventory")
  bool TryAddItem(UKrastovItemDefinition *Definition, FGuid &OutInstanceId);

  UFUNCTION(BlueprintCallable, Category = "Krastov|Inventory")
  bool TryAddItemAt(UKrastovItemDefinition *Definition, FIntPoint Position,
                    bool bRotated, FGuid &OutInstanceId);

  UFUNCTION(BlueprintCallable, Category = "Krastov|Inventory")
  bool MoveItem(FGuid InstanceId, FIntPoint NewPosition, bool bRotated);

  UFUNCTION(BlueprintCallable, Category = "Krastov|Inventory")
  bool RemoveItem(FGuid InstanceId, FInventoryItem &OutRemoved);

  UFUNCTION(BlueprintCallable, Category = "Krastov|Inventory")
  void ClearAll();

  UFUNCTION(BlueprintPure, Category = "Krastov|Inventory")
  float GetTotalWeight() const;

  UFUNCTION(BlueprintPure, Category = "Krastov|Inventory")
  TArray<FInventoryItem> GetItems() const { return Items; }

  UFUNCTION(BlueprintPure, Category = "Krastov|Inventory")
  bool GetItemAtCell(FIntPoint Cell, FInventoryItem &OutItem) const;

  const TArray<FInventoryItem> &GetItemsRef() const { return Items; }

  // ---------- Sinergias ----------
  UFUNCTION(BlueprintPure, Category = "Krastov|Synergy")
  FInventorySynergyReport GetSynergyReport() const { return CachedSynergy; }

  UFUNCTION(BlueprintPure, Category = "Krastov|Synergy")
  bool GetWeaponSynergy(FGuid WeaponId, FWeaponSynergy &OutSynergy) const;

  UFUNCTION(BlueprintPure, Category = "Krastov|Synergy")
  bool IsMedicalBoosted(FGuid ItemId) const {
    return CachedSynergy.BoostedMedicalItems.Contains(ItemId);
  }

  // ---------- Debug ----------
  /** Desenho em texto da grade, para o Output Log */
  FString BuildDebugString() const;

  /** Lista de conexões e multiplicadores, para o Output Log */
  FString BuildSynergyDebugString() const;

private:
  static FIntPoint GetFootprint(const UKrastovItemDefinition *Definition,
                                bool bRotated);
  static const TCHAR *ElementToString(EArtifactElement Element);
  bool IsInsideGrid(FIntPoint Position, FIntPoint Footprint) const;
  int32 FindIndex(const FGuid &InstanceId) const;
  FString GetItemDisplayName(int32 Index) const;
  void RecomputeSynergies();
  void NotifyChanged();

  UPROPERTY(VisibleInstanceOnly, Category = "Krastov|Inventory")
  TArray<FInventoryItem> Items;

  UPROPERTY(VisibleInstanceOnly, Transient, Category = "Krastov|Synergy")
  FInventorySynergyReport CachedSynergy;
};
