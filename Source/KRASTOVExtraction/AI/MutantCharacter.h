#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MutantCharacter.generated.h"


class UAbilitySystemComponent;
class UKrastovAttributeSet;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EMutantState : uint8 { Patrol, Investigate, Chase, Attack };

UCLASS()
class KRASTOVEXTRACTION_API AMutantCharacter : public ACharacter,
                                               public IAbilitySystemInterface {
  GENERATED_BODY()

public:
  AMutantCharacter();

  virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

  /** Retorna true se o ataque aconteceu (respeita o cooldown) */
  bool TryAttack(AActor *Target);

  void NotifyAIStateChanged(EMutantState NewState) {
    OnAIStateChanged(NewState);
  }

protected:
  virtual void BeginPlay() override;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
  TObjectPtr<UAbilitySystemComponent> AbilitySystem;

  UPROPERTY()
  TObjectPtr<UKrastovAttributeSet> Attributes;

  /** GE aplicado no alvo a cada golpe (ex.: GE_MutantHit) */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Combat")
  TSubclassOf<UGameplayEffect> AttackEffect;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|Combat",
            meta = (ClampMin = "0.1"))
  float AttackCooldown = 1.2f;

  // ---------- Ganchos para o Blueprint ----------
  UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
  void OnAttack(AActor *Target);

  UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
  void OnAIStateChanged(EMutantState NewState);

private:
  float NextAttackTime = 0.f;
};
