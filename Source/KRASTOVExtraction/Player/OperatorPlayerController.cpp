#include "Player/OperatorPlayerController.h"

#include "Abilities/KrastovAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player/OperatorCharacter.h"
#include "Inventory/InventoryGridComponent.h"
#include "Inventory/KrastovItemDefinition.h"
#include "UI/KrastovMaletaWidget.h"


void AOperatorPlayerController::BeginPlay() {
  Super::BeginPlay();

  bShowMouseCursor = true;

  FInputModeGameAndUI InputMode;
  InputMode.SetHideCursorDuringCapture(false);
  InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
  SetInputMode(InputMode);

  if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
          ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
              GetLocalPlayer())) {
    if (ZoneMappingContext) {
      Subsystem->AddMappingContext(ZoneMappingContext, 0);
    }
  }
}

void AOperatorPlayerController::SetupInputComponent() {
  Super::SetupInputComponent();

  UEnhancedInputComponent *EIC =
      CastChecked<UEnhancedInputComponent>(InputComponent);

  if (MoveAction)
    EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this,
                    &ThisClass::HandleMove);
  if (AimStickAction)
    EIC->BindAction(AimStickAction, ETriggerEvent::Triggered, this,
                    &ThisClass::HandleAimStick);
  if (SprintAction) {
    EIC->BindAction(SprintAction, ETriggerEvent::Started, this,
                    &ThisClass::HandleSprintStarted);
    EIC->BindAction(SprintAction, ETriggerEvent::Completed, this,
                    &ThisClass::HandleSprintCompleted);
  }
  if (FlashlightAction)
    EIC->BindAction(FlashlightAction, ETriggerEvent::Started, this,
                    &ThisClass::HandleFlashlight);
  if (CrouchAction)
    EIC->BindAction(CrouchAction, ETriggerEvent::Started, this,
                    &ThisClass::HandleCrouch);

  if (InventoryAction)  
    EIC->BindAction(InventoryAction,  ETriggerEvent::Started, this, 
                    &ThisClass::ToggleMaleta);
	if (RotateItemAction)
    EIC->BindAction(RotateItemAction, ETriggerEvent::Started, this,
                    &ThisClass::HandleRotateItem);

}

void AOperatorPlayerController::HandleMove(const FInputActionValue &Value) {
  const FVector2D Input = Value.Get<FVector2D>();
  if (bMaletaOpen) { return; }

  if (APawn *ControlledPawn = GetPawn()) {
    // Câmera fixa com Yaw 0: X do mundo = "cima" na tela, Y = "direita"
    ControlledPawn->AddMovementInput(FVector::ForwardVector, Input.Y);
    ControlledPawn->AddMovementInput(FVector::RightVector, Input.X);
  }
}

void AOperatorPlayerController::HandleAimStick(const FInputActionValue &Value) {
  if (bMaletaOpen) { return; }
  const FVector2D Input = Value.Get<FVector2D>();
  if (Input.SizeSquared() > 0.04f) {
    bUsingGamepadAim = true;
    const float Yaw = FVector(Input.Y, Input.X, 0.f).Rotation().Yaw;
    SetControlRotation(FRotator(0.f, Yaw, 0.f));
  }
}

void AOperatorPlayerController::PlayerTick(float DeltaTime) {
  Super::PlayerTick(DeltaTime);
  if (bMaletaOpen) { return; }

  // Mexeu o mouse: volta para mira por cursor
  float MouseX = 0.f, MouseY = 0.f;
  GetInputMouseDelta(MouseX, MouseY);
  if (!FMath::IsNearlyZero(MouseX) || !FMath::IsNearlyZero(MouseY)) {
    bUsingGamepadAim = false;
  }

  if (!bUsingGamepadAim) {
    AimAtCursor();
  }
}

void AOperatorPlayerController::AimAtCursor() {
  APawn *ControlledPawn = GetPawn();
  FVector RayOrigin, RayDirection;
  if (!ControlledPawn ||
      !DeprojectMousePositionToWorld(RayOrigin, RayDirection)) {
    return;
  }

  // Plano horizontal (XY) na altura do Operador; Z é o eixo vertical no Unreal
  const FVector PawnLocation = ControlledPawn->GetActorLocation();
  const FPlane AimPlane(PawnLocation, FVector::UpVector);
  const FVector AimPoint =
      FMath::RayPlaneIntersection(RayOrigin, RayDirection, AimPlane);

  const FVector ToTarget = AimPoint - PawnLocation;
  if (ToTarget.SizeSquared2D() > 100.f) {
    SetControlRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
  }
}

void AOperatorPlayerController::HandleSprintStarted() {
  if (bMaletaOpen) { return; }
  if (AOperatorCharacter *Operator = GetPawn<AOperatorCharacter>()) {
    Operator->SetSprinting(true);
  }
}

void AOperatorPlayerController::HandleSprintCompleted() {
  if (AOperatorCharacter *Operator = GetPawn<AOperatorCharacter>()) {
    Operator->SetSprinting(false);
  }
}

void AOperatorPlayerController::HandleFlashlight() {
  if (AOperatorCharacter *Operator = GetPawn<AOperatorCharacter>()) {
    Operator->ToggleFlashlight();
  }
}

void AOperatorPlayerController::KrastovSetLoad(float Kg) {
  if (AOperatorCharacter *Operator = GetPawn<AOperatorCharacter>()) {
    if (UAbilitySystemComponent *ASC = Operator->GetAbilitySystemComponent()) {
      ASC->SetNumericAttributeBase(
          UKrastovAttributeSet::GetCarryWeightAttribute(), Kg);
    }
  }
}

void AOperatorPlayerController::HandleCrouch() {
  if (AOperatorCharacter *Operator = GetPawn<AOperatorCharacter>()) {
    Operator->ToggleCrouch();
  }
}

// ---------------inventary ---------------
void AOperatorPlayerController::LogMultiline(const FString &Text) {
  TArray<FString> Lines;
  Text.ParseIntoArrayLines(Lines, false);
  for (const FString &Line : Lines) {
    UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
  }
}

UKrastovItemDefinition *
AOperatorPlayerController::FindItemDef(FName ItemId) const {
  for (UKrastovItemDefinition *Def : DebugItemCatalog) {
    if (Def && Def->ItemId == ItemId) {
      return Def;
    }
  }
  UE_LOG(
      LogTemp, Warning,
      TEXT("[Inv] Item '%s' nao esta no Debug Item Catalog do BP_OperatorPC"),
      *ItemId.ToString());
  return nullptr;
}

UInventoryGridComponent *
AOperatorPlayerController::GetGrid(bool bSafePocket) const {
  const AOperatorCharacter *Operator = GetPawn<AOperatorCharacter>();
  if (!Operator) {
    return nullptr;
  }
  return bSafePocket ? Operator->GetSafePocket() : Operator->GetMaleta();
}

void AOperatorPlayerController::KrastovGive(FName ItemId) {
  UInventoryGridComponent *Grid = GetGrid(false);
  UKrastovItemDefinition *Def = FindItemDef(ItemId);
  if (!Grid || !Def) {
    return;
  }

  FGuid NewId;
  if (!Grid->TryAddItem(Def, NewId)) {
    UE_LOG(LogTemp, Warning, TEXT("[Inv] Sem espaco na Maleta para %s"),
           *ItemId.ToString());
  }
  KrastovDump();
}

void AOperatorPlayerController::KrastovPlace(FName ItemId, int32 X, int32 Y,
                                             bool bRotated, bool bSafePocket) {
  UInventoryGridComponent *Grid = GetGrid(bSafePocket);
  UKrastovItemDefinition *Def = FindItemDef(ItemId);
  if (!Grid || !Def) {
    return;
  }

  FGuid NewId;
  if (!Grid->TryAddItemAt(Def, FIntPoint(X, Y), bRotated, NewId)) {
    UE_LOG(LogTemp, Warning, TEXT("[Inv] %s NAO CABE em (%d,%d)%s"),
           *ItemId.ToString(), X, Y, bRotated ? TEXT(" girado") : TEXT(""));
  }
  KrastovDump();
}

void AOperatorPlayerController::KrastovMove(int32 ItemIndex, int32 X, int32 Y,
                                            bool bRotated, bool bSafePocket) {
  UInventoryGridComponent *Grid = GetGrid(bSafePocket);
  if (!Grid || !Grid->GetItemsRef().IsValidIndex(ItemIndex)) {
    UE_LOG(LogTemp, Warning, TEXT("[Inv] Indice %d invalido"), ItemIndex);
    return;
  }

  const FGuid Id = Grid->GetItemsRef()[ItemIndex].InstanceId;
  if (!Grid->MoveItem(Id, FIntPoint(X, Y), bRotated)) {
    UE_LOG(LogTemp, Warning,
           TEXT("[Inv] Nao foi possivel mover [%d] para (%d,%d)"), ItemIndex, X,
           Y);
  }
  KrastovDump();
}

void AOperatorPlayerController::KrastovTransfer(int32 ItemIndex,
                                                bool bFromSafePocket) {
  UInventoryGridComponent *From = GetGrid(bFromSafePocket);
  UInventoryGridComponent *To = GetGrid(!bFromSafePocket);
  if (!From || !To || !From->GetItemsRef().IsValidIndex(ItemIndex)) {
    UE_LOG(LogTemp, Warning, TEXT("[Inv] Indice %d invalido"), ItemIndex);
    return;
  }

  const FInventoryItem Item = From->GetItemsRef()[ItemIndex];
  FGuid NewId;
  if (!To->TryAddItem(Item.Definition, NewId)) {
    UE_LOG(LogTemp, Warning, TEXT("[Inv] Sem espaco no destino"));
    return;
  }

  FInventoryItem Removed;
  From->RemoveItem(Item.InstanceId, Removed);
  KrastovDump();
}

void AOperatorPlayerController::KrastovRemove(int32 ItemIndex,
                                              bool bSafePocket) {
  UInventoryGridComponent *Grid = GetGrid(bSafePocket);
  if (!Grid || !Grid->GetItemsRef().IsValidIndex(ItemIndex)) {
    UE_LOG(LogTemp, Warning, TEXT("[Inv] Indice %d invalido"), ItemIndex);
    return;
  }

  FInventoryItem Removed;
  Grid->RemoveItem(Grid->GetItemsRef()[ItemIndex].InstanceId, Removed);
  KrastovDump();
}

void AOperatorPlayerController::KrastovClearInv() {
  if (UInventoryGridComponent *Grid = GetGrid(false)) {
    Grid->ClearAll();
  }
  if (UInventoryGridComponent *Grid = GetGrid(true)) {
    Grid->ClearAll();
  }
  KrastovDump();
}

void AOperatorPlayerController::KrastovDump() {
  const UInventoryGridComponent *Maleta = GetGrid(false);
  const UInventoryGridComponent *Safe = GetGrid(true);
  if (!Maleta || !Safe) {
    return;
  }

  LogMultiline(Maleta->BuildDebugString());
  LogMultiline(Maleta->BuildSynergyDebugString());
  LogMultiline(Safe->BuildDebugString());
  LogMultiline(Safe->BuildSynergyDebugString());
  UE_LOG(LogTemp, Display, TEXT("== Peso total: %.2f kg =="),
         Maleta->GetTotalWeight() + Safe->GetTotalWeight());
}

void AOperatorPlayerController::ToggleMaleta()
{
	AOperatorCharacter* Operator = GetPawn<AOperatorCharacter>();
	if (!Operator)
	{
		return;
	}
	if (!MaletaWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI] Maleta Widget Class nao configurado no BP_OperatorPC"));
		return;
	}

	if (!MaletaWidget)
	{
		MaletaWidget = CreateWidget<UKrastovMaletaWidget>(this, MaletaWidgetClass);
	}
	if (!MaletaWidget)
	{
		return;
	}

	bMaletaOpen = !bMaletaOpen;
	if (bMaletaOpen)
	{
		Operator->SetSprinting(false);
		MaletaWidget->SetOperator(Operator);
		MaletaWidget->AddToViewport(10);
	}
	else
	{
		MaletaWidget->CancelDrag();
		MaletaWidget->RemoveFromParent();
	}
}

void AOperatorPlayerController::HandleRotateItem()
{
	if (bMaletaOpen && MaletaWidget)
	{
		MaletaWidget->RotateHeldItem();
	}
}
