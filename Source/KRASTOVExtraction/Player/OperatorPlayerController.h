#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "OperatorPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS(Abstract)
class KRASTOVEXTRACTION_API AOperatorPlayerController
    : public APlayerController {
  GENERATED_BODY()

public:
  /** Console: KrastovSetLoad 30  (testa a penalidade de peso) */
  UFUNCTION(Exec)
  void KrastovSetLoad(float Kg);

protected:
  virtual void BeginPlay() override;
  virtual void SetupInputComponent() override;
  virtual void PlayerTick(float DeltaTime) override;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
  TObjectPtr<UInputMappingContext> ZoneMappingContext;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
  TObjectPtr<UInputAction> MoveAction;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
  TObjectPtr<UInputAction> AimStickAction;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
  TObjectPtr<UInputAction> SprintAction;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
  TObjectPtr<UInputAction> FlashlightAction;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
  TObjectPtr<UInputAction> CrouchAction;

private:
  void HandleMove(const FInputActionValue &Value);
  void HandleAimStick(const FInputActionValue &Value);
  void HandleSprintStarted();
  void HandleSprintCompleted();
  void HandleFlashlight();
  void AimAtCursor();
  void HandleCrouch();

  bool bUsingGamepadAim = false;
};
