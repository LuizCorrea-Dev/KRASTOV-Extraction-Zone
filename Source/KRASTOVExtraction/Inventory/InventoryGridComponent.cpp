#include "Inventory/InventoryGridComponent.h"

UInventoryGridComponent::UInventoryGridComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

FIntPoint
UInventoryGridComponent::GetFootprint(const UKrastovItemDefinition *Definition,
                                      bool bRotated) {
  if (!Definition) {
    return FIntPoint(1, 1);
  }
  return bRotated ? FIntPoint(Definition->Size.Y, Definition->Size.X)
                  : Definition->Size;
}

bool UInventoryGridComponent::IsInsideGrid(FIntPoint Position,
                                           FIntPoint Footprint) const {
  return Position.X >= 0 && Position.Y >= 0 &&
         Position.X + Footprint.X <= GridWidth &&
         Position.Y + Footprint.Y <= GridHeight;
}

int32 UInventoryGridComponent::FindIndex(const FGuid &InstanceId) const {
  return Items.IndexOfByPredicate([&InstanceId](const FInventoryItem &Item) {
    return Item.InstanceId == InstanceId;
  });
}

void UInventoryGridComponent::NotifyChanged() {
  RecomputeSynergies();
  OnInventoryChanged.Broadcast(this);
}

bool UInventoryGridComponent::CanPlace(const UKrastovItemDefinition *Definition,
                                       FIntPoint Position, bool bRotated,
                                       FGuid IgnoreItem) const {
  if (!Definition || (bRotated && !Definition->bCanRotate)) {
    return false;
  }

  const FIntPoint Footprint = GetFootprint(Definition, bRotated);
  if (!IsInsideGrid(Position, Footprint)) {
    return false;
  }

  for (const FInventoryItem &Other : Items) {
    if (Other.InstanceId == IgnoreItem) {
      continue;
    }
    const FIntPoint OtherFootprint = Other.GetFootprint();
    const bool bOverlapX = Position.X < Other.Position.X + OtherFootprint.X &&
                           Other.Position.X < Position.X + Footprint.X;
    const bool bOverlapY = Position.Y < Other.Position.Y + OtherFootprint.Y &&
                           Other.Position.Y < Position.Y + Footprint.Y;
    if (bOverlapX && bOverlapY) {
      return false;
    }
  }
  return true;
}

bool UInventoryGridComponent::FindFreeSpot(
    const UKrastovItemDefinition *Definition, FIntPoint &OutPosition,
    bool &bOutRotated) const {
  if (!Definition) {
    return false;
  }

  for (int32 Pass = 0; Pass < 2; ++Pass) {
    const bool bRotated = (Pass == 1);
    if (bRotated &&
        (!Definition->bCanRotate || Definition->Size.X == Definition->Size.Y)) {
      break; // girar não muda nada ou não é permitido
    }

    for (int32 Y = 0; Y < GridHeight; ++Y) {
      for (int32 X = 0; X < GridWidth; ++X) {
        if (CanPlace(Definition, FIntPoint(X, Y), bRotated, FGuid())) {
          OutPosition = FIntPoint(X, Y);
          bOutRotated = bRotated;
          return true;
        }
      }
    }
  }
  return false;
}

bool UInventoryGridComponent::TryAddItem(UKrastovItemDefinition *Definition,
                                         FGuid &OutInstanceId) {
  FIntPoint Position;
  bool bRotated = false;
  if (!FindFreeSpot(Definition, Position, bRotated)) {
    return false;
  }
  return TryAddItemAt(Definition, Position, bRotated, OutInstanceId);
}

bool UInventoryGridComponent::TryAddItemAt(UKrastovItemDefinition *Definition,
                                           FIntPoint Position, bool bRotated,
                                           FGuid &OutInstanceId) {
  if (Definition && Definition->Size.X == Definition->Size.Y) {
    bRotated = false; // item quadrado: girar não muda nada
  }

  if (!CanPlace(Definition, Position, bRotated, FGuid())) {
    return false;
  }

  FInventoryItem NewItem;
  NewItem.InstanceId = FGuid::NewGuid();
  NewItem.Definition = Definition;
  NewItem.Position = Position;
  NewItem.bRotated = bRotated;
  Items.Add(NewItem);

  OutInstanceId = NewItem.InstanceId;
  NotifyChanged();
  return true;
}

bool UInventoryGridComponent::MoveItem(FGuid InstanceId, FIntPoint NewPosition,
                                       bool bRotated) {
  const int32 Index = FindIndex(InstanceId);
  if (Index == INDEX_NONE) {
    return false;
  }

  const UKrastovItemDefinition *Def = Items[Index].Definition;
  if (Def && Def->Size.X == Def->Size.Y) {
    bRotated = false;
  }

  if (!CanPlace(Def, NewPosition, bRotated, InstanceId)) {
    return false;
  }

  Items[Index].Position = NewPosition;
  Items[Index].bRotated = bRotated;
  NotifyChanged();
  return true;
}

bool UInventoryGridComponent::RemoveItem(FGuid InstanceId,
                                         FInventoryItem &OutRemoved) {
  const int32 Index = FindIndex(InstanceId);
  if (Index == INDEX_NONE) {
    return false;
  }

  OutRemoved = Items[Index];
  Items.RemoveAt(Index);
  NotifyChanged();
  return true;
}

void UInventoryGridComponent::ClearAll() {
  if (Items.Num() > 0) {
    Items.Reset();
    NotifyChanged();
  }
}

float UInventoryGridComponent::GetTotalWeight() const {
  float Total = 0.f;
  for (const FInventoryItem &Item : Items) {
    if (Item.Definition) {
      Total += Item.Definition->Weight;
    }
  }
  return Total;
}

bool UInventoryGridComponent::GetItemAtCell(FIntPoint Cell,
                                            FInventoryItem &OutItem) const {
  for (const FInventoryItem &Item : Items) {
    if (Item.ContainsCell(Cell)) {
      OutItem = Item;
      return true;
    }
  }
  return false;
}

FString UInventoryGridComponent::BuildDebugString() const {
  FString Out = FString::Printf(TEXT("== %s (%dx%d)  peso %.2f kg ==\n"),
                                *GridName.ToString(), GridWidth, GridHeight,
                                GetTotalWeight());

  TArray<TCHAR> Cells;
  Cells.Init(TEXT('.'), GridWidth * GridHeight);

  auto LetterFor = [](int32 Index) -> TCHAR {
    return Index < 26 ? TCHAR(TEXT('A') + Index)
                      : TCHAR(TEXT('a') + (Index - 26) % 26);
  };

  for (int32 i = 0; i < Items.Num(); ++i) {
    const FIntPoint F = Items[i].GetFootprint();
    for (int32 Y = Items[i].Position.Y; Y < Items[i].Position.Y + F.Y; ++Y) {
      for (int32 X = Items[i].Position.X; X < Items[i].Position.X + F.X; ++X) {
        Cells[Y * GridWidth + X] = LetterFor(i);
      }
    }
  }

  Out += TEXT("   ");
  for (int32 X = 0; X < GridWidth; ++X) {
    Out += FString::Printf(TEXT("%d"), X % 10);
  }
  Out += TEXT("\n");

  for (int32 Y = 0; Y < GridHeight; ++Y) {
    Out += FString::Printf(TEXT("%2d "), Y);
    for (int32 X = 0; X < GridWidth; ++X) {
      Out.AppendChar(Cells[Y * GridWidth + X]);
    }
    Out += TEXT("\n");
  }

  for (int32 i = 0; i < Items.Num(); ++i) {
    const FInventoryItem &Item = Items[i];
    const FIntPoint F = Item.GetFootprint();
    FString Name = TEXT("???");
    if (Item.Definition) {
      Name = Item.Definition->DisplayName.IsEmpty()
                 ? Item.Definition->ItemId.ToString()
                 : Item.Definition->DisplayName.ToString();
    }
    Out += FString::Printf(TEXT("  %c [%d] %s  %dx%d em (%d,%d)%s  %.2f kg\n"),
                           LetterFor(i), i, *Name, F.X, F.Y, Item.Position.X,
                           Item.Position.Y,
                           Item.bRotated ? TEXT(" girado") : TEXT(""),
                           Item.Definition ? Item.Definition->Weight : 0.f);
  }
  return Out;
}

const TCHAR *
UInventoryGridComponent::ElementToString(EArtifactElement Element) {
  switch (Element) {
  case EArtifactElement::RadGreen:
    return TEXT("RAD_GREEN");
  case EArtifactElement::ElecPurple:
    return TEXT("ELEC_PURPLE");
  case EArtifactElement::CryoBlue:
    return TEXT("CRYO_BLUE");
  default:
    return TEXT("-");
  }
}

FString UInventoryGridComponent::GetItemDisplayName(int32 Index) const {
  if (!Items.IsValidIndex(Index) || !Items[Index].Definition) {
    return TEXT("???");
  }
  const UKrastovItemDefinition *Def = Items[Index].Definition;
  return Def->DisplayName.IsEmpty() ? Def->ItemId.ToString()
                                    : Def->DisplayName.ToString();
}

bool UInventoryGridComponent::GetWeaponSynergy(
    FGuid WeaponId, FWeaponSynergy &OutSynergy) const {
  for (const FWeaponSynergy &WS : CachedSynergy.Weapons) {
    if (WS.WeaponId == WeaponId) {
      OutSynergy = WS;
      return true;
    }
  }
  return false;
}

void UInventoryGridComponent::RecomputeSynergies() {
  CachedSynergy = FInventorySynergyReport();

  const int32 NumItems = Items.Num();
  if (!bTransmitsSynergy || NumItems < 2) {
    return;
  }

  // 1) Mapa de ocupação: qual item está em cada célula
  TArray<int32> Occupancy;
  Occupancy.Init(INDEX_NONE, GridWidth * GridHeight);
  for (int32 i = 0; i < NumItems; ++i) {
    const FIntPoint F = Items[i].GetFootprint();
    for (int32 Y = Items[i].Position.Y; Y < Items[i].Position.Y + F.Y; ++Y) {
      for (int32 X = Items[i].Position.X; X < Items[i].Position.X + F.X; ++X) {
        Occupancy[Y * GridWidth + X] = i;
      }
    }
  }

  // 2) Contatos ortogonais entre itens diferentes (um por par)
  struct FContact {
    int32 A;
    int32 B;
    FIntPoint CellA;
    FIntPoint CellB;
  };
  TArray<FContact> Contacts;
  TSet<int64> SeenPairs;

  auto TryContact = [&](int32 X1, int32 Y1, int32 X2, int32 Y2) {
    const int32 IA = Occupancy[Y1 * GridWidth + X1];
    const int32 IB = Occupancy[Y2 * GridWidth + X2];
    if (IA == INDEX_NONE || IB == INDEX_NONE || IA == IB) {
      return;
    }
    const int64 Key = int64(FMath::Min(IA, IB)) * 100000 + FMath::Max(IA, IB);
    if (SeenPairs.Contains(Key)) {
      return;
    }
    SeenPairs.Add(Key);
    Contacts.Add(FContact{IA, IB, FIntPoint(X1, Y1), FIntPoint(X2, Y2)});
  };

  for (int32 Y = 0; Y < GridHeight; ++Y) {
    for (int32 X = 0; X < GridWidth; ++X) {
      if (X + 1 < GridWidth) {
        TryContact(X, Y, X + 1, Y);
      }
      if (Y + 1 < GridHeight) {
        TryContact(X, Y, X, Y + 1);
      }
    }
  }

  // 3) Cadeias de artefatos (union-find)
  TArray<int32> Parent;
  Parent.SetNum(NumItems);
  for (int32 i = 0; i < NumItems; ++i) {
    Parent[i] = i;
  }
  auto FindRoot = [&Parent](int32 i) {
    while (Parent[i] != i) {
      Parent[i] = Parent[Parent[i]];
      i = Parent[i];
    }
    return i;
  };

  // 4) Classifica os contatos
  TArray<TPair<int32, int32>> ArtifactToWeapon; // (artefato, arma)
  for (const FContact &C : Contacts) {
    const UKrastovItemDefinition *DA = Items[C.A].Definition;
    const UKrastovItemDefinition *DB = Items[C.B].Definition;
    if (!DA || !DB) {
      continue;
    }

    FSynergyLink Link;
    Link.ItemA = Items[C.A].InstanceId;
    Link.ItemB = Items[C.B].InstanceId;
    Link.CellA = C.CellA;
    Link.CellB = C.CellB;
    Link.ElementA = DA->Element;
    Link.ElementB = DB->Element;

    const bool bArtA = DA->IsArtifact();
    const bool bArtB = DB->IsArtifact();

    if (bArtA && bArtB) {
      Link.Type = ESynergyLinkType::ArtifactToArtifact;
      Parent[FindRoot(C.A)] = FindRoot(C.B);
    } else if (bArtA && DB->IsWeapon()) {
      Link.Type = ESynergyLinkType::ArtifactToWeapon;
      ArtifactToWeapon.Add(TPair<int32, int32>(C.A, C.B));
    } else if (bArtB && DA->IsWeapon()) {
      Link.Type = ESynergyLinkType::ArtifactToWeapon;
      ArtifactToWeapon.Add(TPair<int32, int32>(C.B, C.A));
    } else if (bArtA && DB->Category == EItemCategory::Medical) {
      Link.Type = ESynergyLinkType::ArtifactToMedical;
      CachedSynergy.BoostedMedicalItems.AddUnique(Items[C.B].InstanceId);
    } else if (bArtB && DA->Category == EItemCategory::Medical) {
      Link.Type = ESynergyLinkType::ArtifactToMedical;
      CachedSynergy.BoostedMedicalItems.AddUnique(Items[C.A].InstanceId);
    } else {
      continue; // par sem sinergia (ex.: arma com máscara)
    }

    CachedSynergy.Links.Add(Link);
  }

  // 5) Resultado por arma
  for (int32 WeaponIdx = 0; WeaponIdx < NumItems; ++WeaponIdx) {
    const UKrastovItemDefinition *WeaponDef = Items[WeaponIdx].Definition;
    if (!WeaponDef || !WeaponDef->IsWeapon()) {
      continue;
    }

    TSet<int32> ChainRoots;
    for (const TPair<int32, int32> &Pair : ArtifactToWeapon) {
      if (Pair.Value == WeaponIdx) {
        ChainRoots.Add(FindRoot(Pair.Key));
      }
    }
    if (ChainRoots.Num() == 0) {
      continue;
    }

    FWeaponSynergy WS;
    WS.WeaponId = Items[WeaponIdx].InstanceId;

    for (int32 i = 0; i < NumItems; ++i) {
      const UKrastovItemDefinition *Def = Items[i].Definition;
      if (!Def || !Def->IsArtifact() || !ChainRoots.Contains(FindRoot(i))) {
        continue;
      }
      ++WS.ArtifactCount;
      switch (Def->Element) {
      case EArtifactElement::RadGreen:
        WS.bRadioactive = true;
        break;
      case EArtifactElement::ElecPurple:
        WS.bElectric = true;
        break;
      case EArtifactElement::CryoBlue:
        WS.bCryo = true;
        break;
      default:
        break;
      }
    }

    // x1.15 por artefato além do primeiro de cada cadeia
    const int32 ExtraArtifacts = WS.ArtifactCount - ChainRoots.Num();
    WS.NodeMultiplier = FMath::Pow(NodeMultiplierPerExtraArtifact,
                                   float(FMath::Max(0, ExtraArtifacts)));

    float DamageBonus = 1.f;
    if (WS.bRadioactive) {
      DamageBonus += RadDamageBonus;
    }
    if (WS.bCryo) {
      DamageBonus += CryoDamageBonus;
    }

    WS.DamageMultiplier = DamageBonus * WS.NodeMultiplier;
    WS.FireRateMultiplier = 1.f + (WS.bElectric ? ElecFireRateBonus : 0.f);

    CachedSynergy.Weapons.Add(WS);
  }
}

FString UInventoryGridComponent::BuildSynergyDebugString() const {
  FString Out = TEXT("-- Sinergias --\n");
  if (!bTransmitsSynergy) {
    return Out + TEXT("  (blindado: nao transmite sinergias)\n");
  }

  auto Label = [this](const FGuid &Id) {
    const int32 Index = FindIndex(Id);
    return FString::Printf(TEXT("[%d] %s"), Index, *GetItemDisplayName(Index));
  };

  for (const FSynergyLink &Link : CachedSynergy.Links) {
    const TCHAR *TypeText = TEXT("ART->ARMA  ");
    if (Link.Type == ESynergyLinkType::ArtifactToArtifact) {
      TypeText = TEXT("ART<->ART  ");
    }
    if (Link.Type == ESynergyLinkType::ArtifactToMedical) {
      TypeText = TEXT("ART->MEDICO");
    }

    const EArtifactElement Color = (Link.ElementA != EArtifactElement::None)
                                       ? Link.ElementA
                                       : Link.ElementB;
    Out += FString::Printf(TEXT("  %s %s <-> %s  neon %s  (%d,%d)-(%d,%d)\n"),
                           TypeText, *Label(Link.ItemA), *Label(Link.ItemB),
                           ElementToString(Color), Link.CellA.X, Link.CellA.Y,
                           Link.CellB.X, Link.CellB.Y);
  }

  for (const FWeaponSynergy &WS : CachedSynergy.Weapons) {
    FString Elements;
    if (WS.bRadioactive) {
      Elements += TEXT("RAD ");
    }
    if (WS.bElectric) {
      Elements += TEXT("ELEC ");
    }
    if (WS.bCryo) {
      Elements += TEXT("CRYO ");
    }
    Elements.TrimEndInline();

    Out += FString::Printf(TEXT("  ARMA %s: %d artefato(s) [%s]  dano x%.2f  "
                                "cadencia x%.2f  no x%.2f\n"),
                           *Label(WS.WeaponId), WS.ArtifactCount, *Elements,
                           WS.DamageMultiplier, WS.FireRateMultiplier,
                           WS.NodeMultiplier);
  }

  for (const FGuid &MedId : CachedSynergy.BoostedMedicalItems) {
    Out += FString::Printf(TEXT("  MEDICO %s: cura x%.2f\n"), *Label(MedId),
                           MedicalHealingMultiplier);
  }

  if (CachedSynergy.Links.Num() == 0) {
    Out += TEXT("  (nenhuma conexao)\n");
  }
  return Out;
}
