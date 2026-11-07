#include "Abilities/KrastovAttributeSet.h"
#include "GameplayEffectExtension.h"

UKrastovAttributeSet::UKrastovAttributeSet() {
  // Valores padrão: o jogo funciona mesmo sem um GE de inicialização
  InitHealth(100.f);
  InitMaxHealth(100.f);
  InitStamina(100.f);
  InitMaxStamina(100.f);
  InitRadiation(0.f);
  InitCarryWeight(0.f);
  InitMaxCarryWeight(35.f);
}

void UKrastovAttributeSet::PreAttributeChange(
    const FGameplayAttribute &Attribute, float &NewValue) {
  Super::PreAttributeChange(Attribute, NewValue);

  if (Attribute == GetHealthAttribute()) {
    NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
  } else if (Attribute == GetStaminaAttribute()) {
    NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
  } else if (Attribute == GetRadiationAttribute()) {
    NewValue = FMath::Clamp(NewValue, 0.f, 1000.f);
  } else if (Attribute == GetCarryWeightAttribute()) {
    NewValue = FMath::Max(NewValue, 0.f);
  }
}

void UKrastovAttributeSet::PostGameplayEffectExecute(
    const FGameplayEffectModCallbackData &Data) {
  Super::PostGameplayEffectExecute(Data);

  // Efeitos Instant/Periodic alteram o Base value: clamp final aqui
  if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
    SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
  } else if (Data.EvaluatedData.Attribute == GetStaminaAttribute()) {
    SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
  } else if (Data.EvaluatedData.Attribute == GetRadiationAttribute()) {
    SetRadiation(FMath::Clamp(GetRadiation(), 0.f, 1000.f));
  }
}
