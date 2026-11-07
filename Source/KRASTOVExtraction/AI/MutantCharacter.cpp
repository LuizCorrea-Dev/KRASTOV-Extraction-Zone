#include "AI/MutantCharacter.h"

#include "AI/MutantAIController.h"
#include "Abilities/KrastovAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"

AMutantCharacter::AMutantCharacter() {
  PrimaryActorTick.bCanEverTick = false;

  AbilitySystem =
      CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
  Attributes = CreateDefaultSubobject<UKrastovAttributeSet>(TEXT("Attributes"));

  // Possuído automaticamente pelo controlador do mutante (no mapa ou quando
  // spawnado)
  AIControllerClass = AMutantAIController::StaticClass();
  AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

  bUseControllerRotationYaw = false;
  UCharacterMovementComponent *Move = GetCharacterMovement();
  Move->bOrientRotationToMovement = true;
  Move->RotationRate = FRotator(0.f, 300.f, 0.f);
  Move->MaxWalkSpeed = 150.f;
}

UAbilitySystemComponent *AMutantCharacter::GetAbilitySystemComponent() const {
  return AbilitySystem;
}

void AMutantCharacter::BeginPlay() {
  Super::BeginPlay();
  AbilitySystem->InitAbilityActorInfo(this, this);
}

bool AMutantCharacter::TryAttack(AActor *Target) {
  const float Now = GetWorld()->GetTimeSeconds();
  if (!Target || Now < NextAttackTime) {
    return false;
  }
  NextAttackTime = Now + AttackCooldown;

  OnAttack(Target); // BP: animação, som, VFX

  if (AttackEffect) {
    if (UAbilitySystemComponent *TargetASC =
            UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target)) {
      FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
      Context.AddSourceObject(this);
      const FGameplayEffectSpecHandle Spec =
          AbilitySystem->MakeOutgoingSpec(AttackEffect, 1.f, Context);
      if (Spec.IsValid()) {
        AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(),
                                                       TargetASC);
      }
    }
  }
  return true;
}
