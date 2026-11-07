#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "KrastovAttributeSet.generated.h"

#define KRASTOV_ATTRIBUTE_ACCESSORS(ClassName, PropertyName)                   \
  GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)                   \
  GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)                                 \
  GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)                                 \
  GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class KRASTOVEXTRACTION_API UKrastovAttributeSet : public UAttributeSet {
  GENERATED_BODY()

public:
  UKrastovAttributeSet();

  // ---- Vitais ----
  UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
  FGameplayAttributeData Health;
  KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, Health)

  UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
  FGameplayAttributeData MaxHealth;
  KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, MaxHealth)

  UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
  FGameplayAttributeData Stamina;
  KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, Stamina)

  UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
  FGameplayAttributeData MaxStamina;
  KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, MaxStamina)

  // ---- Ameaças ----
  /** Dose acumulada (0..1000) */
  UPROPERTY(BlueprintReadOnly, Category = "Krastov|Hazard")
  FGameplayAttributeData Radiation;
  KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, Radiation)

  // ---- Carga ----
  /** Peso carregado em kg (o inventário vai alimentar isso na Etapa 4) */
  UPROPERTY(BlueprintReadOnly, Category = "Krastov|Load")
  FGameplayAttributeData CarryWeight;
  KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, CarryWeight)

  UPROPERTY(BlueprintReadOnly, Category = "Krastov|Load")
  FGameplayAttributeData MaxCarryWeight;
  KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, MaxCarryWeight)

  virtual void PreAttributeChange(const FGameplayAttribute &Attribute,
                                  float &NewValue) override;
  virtual void PostGameplayEffectExecute(
      const struct FGameplayEffectModCallbackData &Data) override;
};