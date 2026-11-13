#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "OperatorPlayerController.generated.h"

class UKrastovItemDefinition;
class UInventoryGridComponent;
class UInputMappingContext;
class UInputAction;

UCLASS(Abstract)
class KRASTOVEXTRACTION_API AOperatorPlayerController
    : public APlayerController {
  GENERATED_BODY()

public:
  /** Adiciona o item no primeiro espaço livre da Maleta */
  UFUNCTION(Exec) void KrastovGive(FName ItemId);

  /** Coloca o item em X,Y. Rot: 0/1. Safe: 0 = Maleta, 1 = Safe Pocket */
  UFUNCTION(Exec)
  void KrastovPlace(FName ItemId, int32 X, int32 Y, bool bRotated,
                    bool bSafePocket);

  /** Move o item [Index] para X,Y */
  UFUNCTION(Exec)
  void KrastovMove(int32 ItemIndex, int32 X, int32 Y, bool bRotated,
                   bool bSafePocket);

  /** Passa o item [Index] para a outra grade (Maleta <-> Safe Pocket) */
  UFUNCTION(Exec) void KrastovTransfer(int32 ItemIndex, bool bFromSafePocket);

  UFUNCTION(Exec) void KrastovRemove(int32 ItemIndex, bool bSafePocket);
  UFUNCTION(Exec) void KrastovClearInv();
  UFUNCTION(Exec) void KrastovDump();

  /** Console: KrastovSetLoad 30  (testa a penalidade de peso) */
  UFUNCTION(Exec)
  void KrastovSetLoad(float Kg);

protected:
  virtual void BeginPlay() override;
  virtual void SetupInputComponent() override;
  virtual void PlayerTick(float DeltaTime) override;

  /** Itens disponíveis para os comandos de teste (KrastovGive etc.) */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Debug")
  TArray<TObjectPtr<UKrastovItemDefinition>> DebugItemCatalog;

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

  UKrastovItemDefinition *FindItemDef(FName ItemId) const;
  UInventoryGridComponent *GetGrid(bool bSafePocket) const;
  static void LogMultiline(const FString &Text);

  bool bUsingGamepadAim = false;
};
