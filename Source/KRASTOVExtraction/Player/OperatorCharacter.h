#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "OperatorCharacter.generated.h"


class UAbilitySystemComponent;
class UKrastovAttributeSet;
class USpotLightComponent;
class USpringArmComponent;
class UCameraComponent;
class UGameplayEffect;

UCLASS(Abstract)
class KRASTOVEXTRACTION_API AOperatorCharacter
    : public ACharacter,
      public IAbilitySystemInterface {
  GENERATED_BODY()

public:
  AOperatorCharacter();

  virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
  virtual void PossessedBy(AController *NewController) override;

  UFUNCTION(BlueprintCallable, Category = "Krastov|Movement")
  void SetSprinting(bool bNewWantsSprint);

  UFUNCTION(BlueprintCallable, Category = "Krastov|Equipment")
  void ToggleFlashlight();

  UFUNCTION(BlueprintPure, Category = "Krastov|Equipment")
  bool IsFlashlightOn() const;

  UFUNCTION(BlueprintPure, Category = "Krastov|Movement")
  bool IsExhausted() const { return bExhausted; }

protected:
  virtual void BeginPlay() override;

  // ---------- Componentes ----------
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  TObjectPtr<USpringArmComponent> CameraBoom;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  TObjectPtr<UCameraComponent> TopDownCamera;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  TObjectPtr<USpotLightComponent> Flashlight;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
  TObjectPtr<UAbilitySystemComponent> AbilitySystem;

  UPROPERTY()
  TObjectPtr<UKrastovAttributeSet> Attributes;

  // ---------- Tuning (editável no BP_Operator) ----------
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
  float BaseWalkSpeed = 300.f;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
  float SprintMultiplier = 1.6f;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
  float BaseAcceleration = 900.f;

  /** Fator de velocidade com 120% da carga máxima */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement",
            meta = (ClampMin = "0.1", ClampMax = "1.0"))
  float OverloadedSpeedFactor = 0.45f;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
  float MaxTurnRate = 360.f;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
  float LoadedTurnRate = 180.f;

  /** Fração da estamina necessária para sair da exaustão */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement",
            meta = (ClampMin = "0.0", ClampMax = "1.0"))
  float ExhaustionRecoverRatio = 0.25f;

  // ---------- GAS (assets definidos no BP) ----------
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|GAS")
  TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|GAS")
  TSubclassOf<UGameplayEffect> SprintDrainEffect;

  // ---------- Ganchos para o Blueprint ----------
  UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
  void OnFlashlightToggled(bool bOn);

  UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
  void OnExhausted();

  UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
  void OnRecoveredFromExhaustion();

private:
  FActiveGameplayEffectHandle
  ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass);
  void RecalculateMovement();
  void StopSprintDrain();
  void HandleCarryWeightChanged(const FOnAttributeChangeData &Data);
  void HandleStaminaChanged(const FOnAttributeChangeData &Data);

  bool bWantsSprint = false;
  bool bExhausted = false;
  FActiveGameplayEffectHandle SprintDrainHandle;
};
