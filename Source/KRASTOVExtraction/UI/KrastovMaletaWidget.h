#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryGridComponent.h"

#include "KrastovMaletaWidget.generated.h"

class AOperatorCharacter;
class UTexture2D;

/** Maleta Tática + Safe Pocket desenhados em C++ (grades, ícones, neon, arraste) */
UCLASS()
class KRASTOVEXTRACTION_API UKrastovMaletaWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Krastov|UI")
	void SetOperator(AOperatorCharacter* InOperator);

	/** Gira o item que está sendo arrastado (tecla R) */
	UFUNCTION(BlueprintCallable, Category = "Krastov|UI")
	void RotateHeldItem();

	UFUNCTION(BlueprintCallable, Category = "Krastov|UI")
	void CancelDrag();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// ---------- Visual ----------
	UPROPERTY(EditAnywhere, Category = "Krastov|UI") float CellSize = 64.f;
	UPROPERTY(EditAnywhere, Category = "Krastov|UI") float CellPadding = 2.f;
	UPROPERTY(EditAnywhere, Category = "Krastov|UI") float GridGap = 64.f;
	UPROPERTY(EditAnywhere, Category = "Krastov|UI") float PanelWidth = 340.f;

	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor BackdropColor    = FLinearColor(0.f, 0.f, 0.f, 0.65f);
	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor CellColor        = FLinearColor(0.06f, 0.06f, 0.07f, 0.95f);
	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor MaletaFrameColor = FLinearColor(0.20f, 0.20f, 0.22f, 1.f);
	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor LeadFrameColor   = FLinearColor(0.38f, 0.40f, 0.43f, 1.f);
	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor PanelColor       = FLinearColor(0.08f, 0.08f, 0.09f, 0.92f);
	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor TitleColor       = FLinearColor(0.85f, 0.85f, 0.80f, 1.f);
	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor TextColor        = FLinearColor(0.75f, 0.75f, 0.75f, 1.f);
	UPROPERTY(EditAnywhere, Category = "Krastov|UI|Colors") FLinearColor WarnColor        = FLinearColor(1.f, 0.6f, 0.2f, 1.f);

private:
	struct FGridLayout
	{
		FVector2D MaletaOrigin = FVector2D::ZeroVector;
		FVector2D SafeOrigin   = FVector2D::ZeroVector;
		FVector2D PanelOrigin  = FVector2D::ZeroVector;
		float     PanelHeight  = 0.f;
	};

	FGridLayout ComputeLayout(const FVector2D& LocalSize) const;
	FVector2D GetGridOrigin(const UInventoryGridComponent* Grid, const FGridLayout& Layout) const;
	bool PickCell(const FVector2D& LocalPos, const FGridLayout& Layout, UInventoryGridComponent*& OutGrid, FIntPoint& OutCell) const;
	bool GetDropTarget(const FVector2D& LocalSize, UInventoryGridComponent*& OutGrid, FIntPoint& OutPosition) const;
	void TryDrop(const FVector2D& LocalSize);

	void CacheIconBrushes();
	const FSlateBrush* FindIconBrush(const UKrastovItemDefinition* Def) const;
	FLinearColor GetItemColor(const UKrastovItemDefinition* Def) const;
	static FLinearColor GetElementColor(EArtifactElement Element);
	static FString GetItemName(const UKrastovItemDefinition* Def);
	static FString FindItemName(const UInventoryGridComponent* Grid, const FGuid& Id);

	void PaintGrid(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const UInventoryGridComponent* Grid,
		const FVector2D& Origin, const FLinearColor& FrameColor, const FString& Title) const;
	void PaintItems(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const UInventoryGridComponent* Grid,
		const FVector2D& Origin) const;
	void PaintItemIcon(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const UKrastovItemDefinition* Def,
		const FVector2D& Pos, const FVector2D& Size, bool bRotated, float Alpha) const;
	void PaintSynergyLinks(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FVector2D& Origin) const;
	void PaintNeonSegment(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const FVector2D& From, const FVector2D& To, const FLinearColor& Color, float Pulse) const;
	void PaintDragPreview(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FGridLayout& Layout) const;
	void PaintInfoPanel(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FGridLayout& Layout) const;

	TWeakObjectPtr<AOperatorCharacter> Operator;
	TWeakObjectPtr<UInventoryGridComponent> MaletaGrid;
	TWeakObjectPtr<UInventoryGridComponent> SafeGrid;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UTexture2D>, FSlateBrush> IconBrushes;

	// ---------- Arraste ----------
	bool bDragging = false;
	FGuid HeldId;
	TWeakObjectPtr<UInventoryGridComponent> HeldSourceGrid;

	UPROPERTY(Transient)
	TObjectPtr<UKrastovItemDefinition> HeldDef;

	bool bHeldRotated = false;
	FIntPoint GrabOffset = FIntPoint::ZeroValue;
	FVector2D MouseLocal = FVector2D::ZeroVector;
};
