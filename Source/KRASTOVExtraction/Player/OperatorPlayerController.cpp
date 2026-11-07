#include "Player/OperatorPlayerController.h"

#include "Abilities/KrastovAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player/OperatorCharacter.h"

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
}

void AOperatorPlayerController::HandleMove(const FInputActionValue &Value) {
  const FVector2D Input = Value.Get<FVector2D>();
  if (APawn *ControlledPawn = GetPawn()) {
    // Câmera fixa com Yaw 0: X do mundo = "cima" na tela, Y = "direita"
    ControlledPawn->AddMovementInput(FVector::ForwardVector, Input.Y);
    ControlledPawn->AddMovementInput(FVector::RightVector, Input.X);
  }
}

void AOperatorPlayerController::HandleAimStick(const FInputActionValue &Value) {
  const FVector2D Input = Value.Get<FVector2D>();
  if (Input.SizeSquared() > 0.04f) {
    bUsingGamepadAim = true;
    const float Yaw = FVector(Input.Y, Input.X, 0.f).Rotation().Yaw;
    SetControlRotation(FRotator(0.f, Yaw, 0.f));
  }
}

void AOperatorPlayerController::PlayerTick(float DeltaTime) {
  Super::PlayerTick(DeltaTime);

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
