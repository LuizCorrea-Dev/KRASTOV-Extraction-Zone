#include "Inventory/KrastovItemDefinition.h"

bool UKrastovItemDefinition::IsWeapon() const {
  return Category == EItemCategory::WeaponLong ||
         Category == EItemCategory::WeaponPistol;
}

bool UKrastovItemDefinition::IsArtifact() const {
  return Category == EItemCategory::Artifact &&
         Element != EArtifactElement::None;
}

FPrimaryAssetId UKrastovItemDefinition::GetPrimaryAssetId() const {
  return FPrimaryAssetId(FPrimaryAssetType(TEXT("KrastovItem")), GetFName());
}
