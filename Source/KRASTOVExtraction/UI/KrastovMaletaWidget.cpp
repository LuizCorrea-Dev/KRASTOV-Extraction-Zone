#include "UI/KrastovMaletaWidget.h"

#include "Abilities/KrastovAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Player/OperatorCharacter.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	FPaintGeometry MakeGeo(const FGeometry& Geo, const FVector2D& Position, const FVector2D& Size)
	{
		return Geo.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position)));
	}

	const FSlateBrush* WhiteBrush()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}
}

// ============================================================ Setup

void UKrastovMaletaWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible); // recebe cliques do mouse
}

void UKrastovMaletaWidget::SetOperator(AOperatorCharacter* InOperator)
{
	Operator   = InOperator;
	MaletaGrid = InOperator ? InOperator->GetMaleta() : nullptr;
	SafeGrid   = InOperator ? InOperator->GetSafePocket() : nullptr;
	CancelDrag();
}

void UKrastovMaletaWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	CacheIconBrushes();
}

void UKrastovMaletaWidget::CacheIconBrushes()
{
	auto CacheGrid = [this](const UInventoryGridComponent* Grid)
	{
		if (!Grid)
		{
			return;
		}
		for (const FInventoryItem& Item : Grid->GetItemsRef())
		{
			UTexture2D* Tex = Item.Definition ? Item.Definition->Icon.Get() : nullptr;
			if (Tex && !IconBrushes.Contains(Tex))
			{
				FSlateBrush Brush;
				Brush.SetResourceObject(Tex);
				Brush.ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
				Brush.DrawAs = ESlateBrushDrawType::Image;
				IconBrushes.Add(Tex, Brush);
			}
		}
	};
	CacheGrid(MaletaGrid.Get());
	CacheGrid(SafeGrid.Get());
}

const FSlateBrush* UKrastovMaletaWidget::FindIconBrush(const UKrastovItemDefinition* Def) const
{
	if (!Def || !Def->Icon)
	{
		return nullptr;
	}
	return IconBrushes.Find(Def->Icon);
}

// ============================================================ Layout e cores

UKrastovMaletaWidget::FGridLayout UKrastovMaletaWidget::ComputeLayout(const FVector2D& LocalSize) const
{
	const int32 MW = MaletaGrid.IsValid() ? MaletaGrid->GridWidth  : 8;
	const int32 MH = MaletaGrid.IsValid() ? MaletaGrid->GridHeight : 5;
	const int32 SW = SafeGrid.IsValid()   ? SafeGrid->GridWidth    : 4;
	const int32 SH = SafeGrid.IsValid()   ? SafeGrid->GridHeight   : 3;

	const FVector2D MaletaSize(MW * CellSize, MH * CellSize);
	const FVector2D SafeSize(SW * CellSize, SH * CellSize);
	const float PanelGap = 48.f;
	const float TotalW = MaletaSize.X + PanelGap + PanelWidth;
	const float TotalH = MaletaSize.Y + GridGap + SafeSize.Y;

	FGridLayout L;
	L.MaletaOrigin = FVector2D((LocalSize.X - TotalW) * 0.5f, (LocalSize.Y - TotalH) * 0.5f);
	L.SafeOrigin   = FVector2D(L.MaletaOrigin.X + (MaletaSize.X - SafeSize.X) * 0.5f, L.MaletaOrigin.Y + MaletaSize.Y + GridGap);
	L.PanelOrigin  = FVector2D(L.MaletaOrigin.X + MaletaSize.X + PanelGap, L.MaletaOrigin.Y);
	L.PanelHeight  = TotalH;
	return L;
}

FVector2D UKrastovMaletaWidget::GetGridOrigin(const UInventoryGridComponent* Grid, const FGridLayout& Layout) const
{
	return (Grid == MaletaGrid.Get()) ? Layout.MaletaOrigin : Layout.SafeOrigin;
}

bool UKrastovMaletaWidget::PickCell(const FVector2D& LocalPos, const FGridLayout& Layout, UInventoryGridComponent*& OutGrid, FIntPoint& OutCell) const
{
	for (UInventoryGridComponent* Grid : { MaletaGrid.Get(), SafeGrid.Get() })
	{
		if (!Grid)
		{
			continue;
		}
		const FVector2D InGrid = (LocalPos - GetGridOrigin(Grid, Layout)) / CellSize;
		const FIntPoint Cell(FMath::FloorToInt(InGrid.X), FMath::FloorToInt(InGrid.Y));
		if (Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Grid->GridWidth && Cell.Y < Grid->GridHeight)
		{
			OutGrid = Grid;
			OutCell = Cell;
			return true;
		}
	}
	return false;
}

FLinearColor UKrastovMaletaWidget::GetElementColor(EArtifactElement Element)
{
	switch (Element)
	{
	case EArtifactElement::RadGreen:   return FLinearColor(0.25f, 1.00f, 0.35f, 1.f);
	case EArtifactElement::ElecPurple: return FLinearColor(0.75f, 0.35f, 1.00f, 1.f);
	case EArtifactElement::CryoBlue:   return FLinearColor(0.35f, 0.75f, 1.00f, 1.f);
	default:                           return FLinearColor(0.80f, 0.80f, 0.80f, 1.f);
	}
}

FLinearColor UKrastovMaletaWidget::GetItemColor(const UKrastovItemDefinition* Def) const
{
	if (!Def)
	{
		return FLinearColor::Gray;
	}
	if (Def->IsArtifact())
	{
		return GetElementColor(Def->Element);
	}
	switch (Def->Category)
	{
	case EItemCategory::WeaponLong:
	case EItemCategory::WeaponPistol: return FLinearColor(0.55f, 0.55f, 0.55f, 1.f);
	case EItemCategory::Medical:      return FLinearColor(0.95f, 0.95f, 0.95f, 1.f);
	case EItemCategory::GasMask:
	case EItemCategory::Filter:
	case EItemCategory::Suit:         return FLinearColor(0.55f, 0.60f, 0.35f, 1.f);
	case EItemCategory::KeyItem:      return FLinearColor(1.00f, 0.80f, 0.20f, 1.f);
	default:                          return FLinearColor(0.60f, 0.50f, 0.40f, 1.f);
	}
}

FString UKrastovMaletaWidget::GetItemName(const UKrastovItemDefinition* Def)
{
	if (!Def)
	{
		return TEXT("???");
	}
	return Def->DisplayName.IsEmpty() ? Def->ItemId.ToString() : Def->DisplayName.ToString();
}

FString UKrastovMaletaWidget::FindItemName(const UInventoryGridComponent* Grid, const FGuid& Id)
{
	if (Grid)
	{
		for (const FInventoryItem& Item : Grid->GetItemsRef())
		{
			if (Item.InstanceId == Id)
			{
				return GetItemName(Item.Definition);
			}
		}
	}
	return TEXT("???");
}

// ============================================================ Mouse / arraste

FReply UKrastovMaletaWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	MouseLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	const FGridLayout L = ComputeLayout(InGeometry.GetLocalSize());
	UInventoryGridComponent* Grid = nullptr;
	FIntPoint Cell;
	FInventoryItem Item;
	if (PickCell(MouseLocal, L, Grid, Cell) && Grid->GetItemAtCell(Cell, Item) && Item.Definition)
	{
		bDragging      = true;
		HeldId         = Item.InstanceId;
		HeldSourceGrid = Grid;
		HeldDef        = Item.Definition;
		bHeldRotated   = Item.bRotated;
		GrabOffset     = Cell - Item.Position;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return FReply::Handled();
}

FReply UKrastovMaletaWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	MouseLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

FReply UKrastovMaletaWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	MouseLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	if (!bDragging || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	TryDrop(InGeometry.GetLocalSize());
	CancelDrag();
	return FReply::Handled().ReleaseMouseCapture();
}

bool UKrastovMaletaWidget::GetDropTarget(const FVector2D& LocalSize, UInventoryGridComponent*& OutGrid, FIntPoint& OutPosition) const
{
	const FGridLayout L = ComputeLayout(LocalSize);
	FIntPoint Cell;
	if (!PickCell(MouseLocal, L, OutGrid, Cell))
	{
		return false;
	}
	OutPosition = Cell - GrabOffset;
	return true;
}

void UKrastovMaletaWidget::TryDrop(const FVector2D& LocalSize)
{
	UInventoryGridComponent* Source = HeldSourceGrid.Get();
	UInventoryGridComponent* Target = nullptr;
	FIntPoint TargetPos;
	if (!Source || !HeldDef || !GetDropTarget(LocalSize, Target, TargetPos))
	{
		return; // soltou fora das grades: cancela (4.4 = jogar no chÃ£o)
	}

	if (Target == Source)
	{
		Source->MoveItem(HeldId, TargetPos, bHeldRotated);
		return;
	}

	if (!Target->CanPlace(HeldDef, TargetPos, bHeldRotated, FGuid()))
	{
		return;
	}

	FInventoryItem Removed;
	if (Source->RemoveItem(HeldId, Removed))
	{
		FGuid NewId;
		if (!Target->TryAddItemAt(HeldDef, TargetPos, bHeldRotated, NewId))
		{
			FGuid BackId; // seguranÃ§a: devolve ao lugar original
			Source->TryAddItemAt(Removed.Definition, Removed.Position, Removed.bRotated, BackId);
		}
	}
}

void UKrastovMaletaWidget::RotateHeldItem()
{
	if (!bDragging || !HeldDef || !HeldDef->bCanRotate || HeldDef->Size.X == HeldDef->Size.Y)
	{
		return;
	}
	bHeldRotated = !bHeldRotated;
	GrabOffset = FIntPoint(GrabOffset.Y, GrabOffset.X);
}

void UKrastovMaletaWidget::CancelDrag()
{
	bDragging = false;
	HeldId.Invalidate();
	HeldSourceGrid = nullptr;
	HeldDef = nullptr;
}

// ============================================================ Desenho

int32 UKrastovMaletaWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FGridLayout L = ComputeLayout(AllottedGeometry.GetLocalSize());

	// Fundo escurecido (o jogo continua visÃ­vel atrÃ¡s)
	++Layer;
	FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), WhiteBrush(), ESlateDrawEffect::None, BackdropColor);

	++Layer;
	PaintGrid(OutDrawElements, Layer, AllottedGeometry, MaletaGrid.Get(), L.MaletaOrigin, MaletaFrameColor, TEXT("MALETA TATICA"));
	PaintGrid(OutDrawElements, Layer, AllottedGeometry, SafeGrid.Get(), L.SafeOrigin, LeadFrameColor, TEXT("SAFE POCKET  -  BLINDAGEM DE CHUMBO"));

	Layer += 2;
	PaintItems(OutDrawElements, Layer, AllottedGeometry, MaletaGrid.Get(), L.MaletaOrigin);
	PaintItems(OutDrawElements, Layer, AllottedGeometry, SafeGrid.Get(), L.SafeOrigin);

	Layer += 2;
	PaintSynergyLinks(OutDrawElements, Layer, AllottedGeometry, L.MaletaOrigin);

	Layer += 2;
	if (bDragging)
	{
		PaintDragPreview(OutDrawElements, Layer, AllottedGeometry, L);
	}

	Layer += 3;
	PaintInfoPanel(OutDrawElements, Layer, AllottedGeometry, L);

	return Layer + 2;
}

void UKrastovMaletaWidget::PaintGrid(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const UInventoryGridComponent* Grid,
	const FVector2D& Origin, const FLinearColor& FrameColor, const FString& Title) const
{
	if (!Grid)
	{
		return;
	}
	const FVector2D Size(Grid->GridWidth * CellSize, Grid->GridHeight * CellSize);

	FSlateDrawElement::MakeBox(Out, Layer, MakeGeo(Geo, Origin - FVector2D(6.f, 6.f), Size + FVector2D(12.f, 12.f)),
		WhiteBrush(), ESlateDrawEffect::None, FrameColor);

	const float Inner = CellSize - 2.f * CellPadding;
	for (int32 Y = 0; Y < Grid->GridHeight; ++Y)
	{
		for (int32 X = 0; X < Grid->GridWidth; ++X)
		{
			const FVector2D CellPos = Origin + FVector2D(X * CellSize + CellPadding, Y * CellSize + CellPadding);
			FSlateDrawElement::MakeBox(Out, Layer + 1, MakeGeo(Geo, CellPos, FVector2D(Inner, Inner)),
				WhiteBrush(), ESlateDrawEffect::None, CellColor);
		}
	}

	FSlateDrawElement::MakeText(Out, Layer + 1, MakeGeo(Geo, Origin + FVector2D(0.f, -32.f), FVector2D(Size.X, 24.f)),
		Title, FCoreStyle::GetDefaultFontStyle("Bold", 14), ESlateDrawEffect::None, TitleColor);
}

void UKrastovMaletaWidget::PaintItems(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const UInventoryGridComponent* Grid,
	const FVector2D& Origin) const
{
	if (!Grid)
	{
		return;
	}
	for (const FInventoryItem& Item : Grid->GetItemsRef())
	{
		const UKrastovItemDefinition* Def = Item.Definition;
		if (!Def)
		{
			continue;
		}
		const FIntPoint F = Item.GetFootprint();
		const FVector2D Pos  = Origin + FVector2D(Item.Position.X * CellSize + CellPadding, Item.Position.Y * CellSize + CellPadding);
		const FVector2D Size = FVector2D(F.X * CellSize - 2.f * CellPadding, F.Y * CellSize - 2.f * CellPadding);

		const bool bIsHeld = bDragging && Item.InstanceId == HeldId;
		const float Alpha = bIsHeld ? 0.3f : 1.f;

		FLinearColor Background = GetItemColor(Def);
		Background.A = 0.30f * Alpha;
		FSlateDrawElement::MakeBox(Out, Layer, MakeGeo(Geo, Pos, Size), WhiteBrush(), ESlateDrawEffect::None, Background);

		PaintItemIcon(Out, Layer + 1, Geo, Def, Pos, Size, Item.bRotated, Alpha);
	}
}

void UKrastovMaletaWidget::PaintItemIcon(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const UKrastovItemDefinition* Def,
	const FVector2D& Pos, const FVector2D& Size, bool bRotated, float Alpha) const
{
	const FLinearColor Tint(1.f, 1.f, 1.f, Alpha);

	if (const FSlateBrush* Brush = FindIconBrush(Def))
	{
		if (!bRotated)
		{
			FSlateDrawElement::MakeBox(Out, Layer, MakeGeo(Geo, Pos, Size), Brush, ESlateDrawEffect::None, Tint);
		}
		else
		{
			// Ãcone desenhado na horizontal e girado 90Â° em torno do centro
			const FVector2D Center = Pos + Size * 0.5f;
			const FVector2D Unrotated(Size.Y, Size.X);
			FSlateDrawElement::MakeRotatedBox(Out, Layer, MakeGeo(Geo, Center - Unrotated * 0.5f, Unrotated), Brush,
				ESlateDrawEffect::None, UE_HALF_PI, TOptional<FVector2f>(), FSlateDrawElement::RelativeToElement, Tint);
		}
	}
	else
	{
		// Sem Ã­cone: nome do item como provisÃ³rio
		FSlateDrawElement::MakeText(Out, Layer, MakeGeo(Geo, Pos + FVector2D(4.f, 4.f), Size - FVector2D(8.f, 8.f)),
			GetItemName(Def), FCoreStyle::GetDefaultFontStyle("Regular", 9), ESlateDrawEffect::None, Tint);
	}
}

void UKrastovMaletaWidget::PaintNeonSegment(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
	const FVector2D& From, const FVector2D& To, const FLinearColor& Color, float Pulse) const
{
	TArray<FVector2f> Points;
	Points.Add(FVector2f(From));
	Points.Add(FVector2f(To));

	FLinearColor Glow = Color;
	Glow.A = 0.30f * Pulse;
	FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None, Glow, true, 10.f);

	FLinearColor Core = Color;
	Core.A = Pulse;
	FSlateDrawElement::MakeLines(Out, Layer + 1, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None, Core, true, 3.f);
}

void UKrastovMaletaWidget::PaintSynergyLinks(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FVector2D& Origin) const
{
	const UInventoryGridComponent* Grid = MaletaGrid.Get();
	if (!Grid)
	{
		return;
	}

	const float Time  = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Pulse = 0.75f + 0.25f * FMath::Sin(Time * 4.f); // brilho pulsante estilo neon

	const FInventorySynergyReport Report = Grid->GetSynergyReport();
	for (const FSynergyLink& Link : Report.Links)
	{
		const FVector2D A = Origin + (FVector2D(Link.CellA.X, Link.CellA.Y) + FVector2D(0.5f, 0.5f)) * CellSize;
		const FVector2D B = Origin + (FVector2D(Link.CellB.X, Link.CellB.Y) + FVector2D(0.5f, 0.5f)) * CellSize;
		const FVector2D Mid = (A + B) * 0.5f;

		const EArtifactElement ElemA = (Link.ElementA != EArtifactElement::None) ? Link.ElementA : Link.ElementB;
		const EArtifactElement ElemB = (Link.ElementB != EArtifactElement::None) ? Link.ElementB : Link.ElementA;

		// Duas metades: gradiente quando os elementos sÃ£o diferentes
		PaintNeonSegment(Out, Layer, Geo, A, Mid, GetElementColor(ElemA), Pulse);
		PaintNeonSegment(Out, Layer, Geo, Mid, B, GetElementColor(ElemB), Pulse);
	}
}

void UKrastovMaletaWidget::PaintDragPreview(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FGridLayout& Layout) const
{
	if (!HeldDef)
	{
		return;
	}
	const FIntPoint F = bHeldRotated ? FIntPoint(HeldDef->Size.Y, HeldDef->Size.X) : HeldDef->Size;
	const FVector2D FootSize(F.X * CellSize, F.Y * CellSize);

	// Destaque verde (cabe) ou vermelho (nÃ£o cabe)
	UInventoryGridComponent* Target = nullptr;
	FIntPoint TargetPos;
	if (GetDropTarget(Geo.GetLocalSize(), Target, TargetPos))
	{
		const FGuid Ignore = (Target == HeldSourceGrid.Get()) ? HeldId : FGuid();
		const bool bValid = Target->CanPlace(HeldDef, TargetPos, bHeldRotated, Ignore);
		const FLinearColor Color = bValid ? FLinearColor(0.1f, 1.f, 0.2f, 0.35f) : FLinearColor(1.f, 0.1f, 0.1f, 0.35f);
		const FVector2D Origin = GetGridOrigin(Target, Layout);
		FSlateDrawElement::MakeBox(Out, Layer, MakeGeo(Geo, Origin + FVector2D(TargetPos.X * CellSize, TargetPos.Y * CellSize), FootSize),
			WhiteBrush(), ESlateDrawEffect::None, Color);
	}

	// Item "fantasma" seguindo o cursor
	const FVector2D GhostPos = MouseLocal - FVector2D((GrabOffset.X + 0.5f) * CellSize, (GrabOffset.Y + 0.5f) * CellSize);
	FLinearColor Background = GetItemColor(HeldDef);
	Background.A = 0.45f;
	FSlateDrawElement::MakeBox(Out, Layer + 1, MakeGeo(Geo, GhostPos, FootSize), WhiteBrush(), ESlateDrawEffect::None, Background);
	PaintItemIcon(Out, Layer + 2, Geo, HeldDef, GhostPos, FootSize, bHeldRotated, 0.9f);
}

void UKrastovMaletaWidget::PaintInfoPanel(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FGridLayout& Layout) const
{
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateFontInfo BodyFont  = FCoreStyle::GetDefaultFontStyle("Regular", 11);

	FSlateDrawElement::MakeBox(Out, Layer, MakeGeo(Geo, Layout.PanelOrigin - FVector2D(6.f, 6.f), FVector2D(PanelWidth + 12.f, Layout.PanelHeight + 12.f)),
		WhiteBrush(), ESlateDrawEffect::None, PanelColor);

	FVector2D TextPos = Layout.PanelOrigin + FVector2D(8.f, 4.f);
	auto Line = [&](const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeText(Out, Layer + 1, MakeGeo(Geo, TextPos, FVector2D(PanelWidth - 16.f, 20.f)), Text, Font, ESlateDrawEffect::None, Color);
		TextPos.Y += 20.f;
	};

	// Peso
	const float Weight = (MaletaGrid.IsValid() ? MaletaGrid->GetTotalWeight() : 0.f) + (SafeGrid.IsValid() ? SafeGrid->GetTotalWeight() : 0.f);
	float MaxWeight = 35.f;
	if (const AOperatorCharacter* Op = Operator.Get())
	{
		if (const UAbilitySystemComponent* ASC = Op->GetAbilitySystemComponent())
		{
			MaxWeight = ASC->GetNumericAttribute(UKrastovAttributeSet::GetMaxCarryWeightAttribute());
		}
	}
	Line(TEXT("CARGA"), TitleFont, TitleColor);
	Line(FString::Printf(TEXT("%.1f / %.0f kg"), Weight, MaxWeight), BodyFont, Weight > MaxWeight * 0.5f ? WarnColor : TextColor);
	TextPos.Y += 10.f;

	// Armas energizadas
	Line(TEXT("ARMAS"), TitleFont, TitleColor);
	const FInventorySynergyReport Report = MaletaGrid.IsValid() ? MaletaGrid->GetSynergyReport() : FInventorySynergyReport();
	if (Report.Weapons.Num() == 0)
	{
		Line(TEXT("  Nenhuma arma energizada"), BodyFont, TextColor);
	}
	for (const FWeaponSynergy& WS : Report.Weapons)
	{
		FString Elements;
		if (WS.bRadioactive) { Elements += TEXT("RAD "); }
		if (WS.bElectric)    { Elements += TEXT("ELEC "); }
		if (WS.bCryo)        { Elements += TEXT("CRYO "); }
		Elements.TrimEndInline();

		const FLinearColor WeaponColor = WS.bRadioactive ? GetElementColor(EArtifactElement::RadGreen)
			: (WS.bElectric ? GetElementColor(EArtifactElement::ElecPurple) : GetElementColor(EArtifactElement::CryoBlue));

		Line(FString::Printf(TEXT("  %s  [%s]"), *FindItemName(MaletaGrid.Get(), WS.WeaponId), *Elements), BodyFont, WeaponColor);
		Line(FString::Printf(TEXT("    Dano x%.2f   Cadencia x%.2f"), WS.DamageMultiplier, WS.FireRateMultiplier), BodyFont, TextColor);
		Line(FString::Printf(TEXT("    No x%.2f   (%d artefato(s))"), WS.NodeMultiplier, WS.ArtifactCount), BodyFont, TextColor);
	}
	TextPos.Y += 10.f;

	// MÃ©dicos
	if (Report.BoostedMedicalItems.Num() > 0)
	{
		Line(TEXT("MEDICOS"), TitleFont, TitleColor);
		for (const FGuid& MedId : Report.BoostedMedicalItems)
		{
			Line(FString::Printf(TEXT("  %s  cura x%.2f"), *FindItemName(MaletaGrid.Get(), MedId), MaletaGrid->MedicalHealingMultiplier), BodyFont, TextColor);
		}
		TextPos.Y += 10.f;
	}

	// Item sob o cursor
	UInventoryGridComponent* HoverGrid = nullptr;
	FIntPoint HoverCell;
	FInventoryItem HoverItem;
	if (!bDragging && PickCell(MouseLocal, Layout, HoverGrid, HoverCell) && HoverGrid->GetItemAtCell(HoverCell, HoverItem) && HoverItem.Definition)
	{
		const UKrastovItemDefinition* Def = HoverItem.Definition;
		Line(TEXT("ITEM"), TitleFont, TitleColor);
		Line(FString::Printf(TEXT("  %s"), *GetItemName(Def)), BodyFont, GetItemColor(Def));
		Line(FString::Printf(TEXT("  %s  -  %.2f kg"), *UEnum::GetDisplayValueAsText(Def->Category).ToString(), Def->Weight), BodyFont, TextColor);
	}

	// Controles
	TextPos = Layout.PanelOrigin + FVector2D(8.f, Layout.PanelHeight - 24.f);
	Line(TEXT("Arrastar: mover   R: girar   Tab: fechar"), BodyFont, FLinearColor(0.5f, 0.5f, 0.5f, 1.f));
}
