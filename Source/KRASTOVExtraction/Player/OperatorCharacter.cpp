#include "Player/OperatorCharacter.h"

#include "Abilities/KrastovAttributeSet.h"
#include "Abilities/KrastovTags.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

namespace {
constexpr float FootstepCheckInterval = 0.05f;
}

AOperatorCharacter::AOperatorCharacter() {
  PrimaryActorTick.bCanEverTick = false;

  // GAS: o ASC detecta automaticamente AttributeSets criados como subobject do
  // dono
  AbilitySystem =
      CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
  Attributes = CreateDefaultSubobject<UKrastovAttributeSet>(TEXT("Attributes"));

  // Câmera top-down fixa (não gira com o personagem)
  CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
  CameraBoom->SetupAttachment(RootComponent);
  CameraBoom->SetUsingAbsoluteRotation(true);
  CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
  CameraBoom->TargetArmLength = 1400.f;
  CameraBoom->bDoCollisionTest = false;
  CameraBoom->bEnableCameraLag = true;
  CameraBoom->CameraLagSpeed = 6.f;

  TopDownCamera =
      CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
  TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
  TopDownCamera->bUsePawnControlRotation = false;

  // Lanterna: presa ao root por enquanto; no BP você pode reanexar a um socket
  // da arma
  Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
  Flashlight->SetupAttachment(RootComponent);
  Flashlight->SetRelativeLocation(FVector(30.f, 0.f, 40.f));
  Flashlight->SetInnerConeAngle(12.f);
  Flashlight->SetOuterConeAngle(28.f);
  Flashlight->SetAttenuationRadius(2500.f);
  Flashlight->SetVolumetricScatteringIntensity(1.5f);
  Flashlight->bCastVolumetricShadow = true;
  Flashlight->SetVisibility(false);

  // Rotação: o personagem gira em direção à mira com taxa limitada (inércia)
  bUseControllerRotationPitch = false;
  bUseControllerRotationYaw = false;
  bUseControllerRotationRoll = false;

  UCharacterMovementComponent *Move = GetCharacterMovement();
  Move->bOrientRotationToMovement = false;
  Move->bUseControllerDesiredRotation = true;
  Move->RotationRate = FRotator(0.f, MaxTurnRate, 0.f);
  Move->MaxWalkSpeed = BaseWalkSpeed;
  Move->MaxAcceleration = BaseAcceleration;
  Move->BrakingDecelerationWalking = 1400.f;
  Move->GroundFriction = 6.f;
  Move->AirControl = 0.05f;
  Move->GetNavAgentPropertiesRef().bCanCrouch = true;

  JumpMaxCount = 1;
}

UAbilitySystemComponent *AOperatorCharacter::GetAbilitySystemComponent() const {
  return AbilitySystem;
}

void AOperatorCharacter::BeginPlay() {
  Super::BeginPlay();

  AbilitySystem->InitAbilityActorInfo(this, this);

  for (const TSubclassOf<UGameplayEffect> &Effect : StartupEffects) {
    ApplyEffectToSelf(Effect);
  }

  AbilitySystem
      ->GetGameplayAttributeValueChangeDelegate(
          UKrastovAttributeSet::GetCarryWeightAttribute())
      .AddUObject(this, &AOperatorCharacter::HandleCarryWeightChanged);
  AbilitySystem
      ->GetGameplayAttributeValueChangeDelegate(
          UKrastovAttributeSet::GetMaxCarryWeightAttribute())
      .AddUObject(this, &AOperatorCharacter::HandleCarryWeightChanged);
  AbilitySystem
      ->GetGameplayAttributeValueChangeDelegate(
          UKrastovAttributeSet::GetStaminaAttribute())
      .AddUObject(this, &AOperatorCharacter::HandleStaminaChanged);

  GetWorldTimerManager().SetTimer(FootstepTimer, this,
                                  &AOperatorCharacter::UpdateFootsteps,
                                  FootstepCheckInterval, true);

  RecalculateMovement();
}

void AOperatorCharacter::PossessedBy(AController *NewController) {
  Super::PossessedBy(NewController);
  // Agora o PlayerController é resolvível a partir do dono
  AbilitySystem->RefreshAbilityActorInfo();
}

FActiveGameplayEffectHandle AOperatorCharacter::ApplyEffectToSelf(
    TSubclassOf<UGameplayEffect> EffectClass) {
  if (!EffectClass) {
    return FActiveGameplayEffectHandle();
  }

  FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
  Context.AddSourceObject(this);

  const FGameplayEffectSpecHandle Spec =
      AbilitySystem->MakeOutgoingSpec(EffectClass, 1.f, Context);
  return Spec.IsValid()
             ? AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get())
             : FActiveGameplayEffectHandle();
}

void AOperatorCharacter::RecalculateMovement() {
  if (!Attributes) {
    return;
  }

  const float MaxLoad = FMath::Max(1.f, Attributes->GetMaxCarryWeight());
  const float LoadRatio = Attributes->GetCarryWeight() / MaxLoad;

  // Até 50% da carga: sem penalidade. De 50% a 120%: cai até
  // OverloadedSpeedFactor.
  const float Penalty = FMath::GetMappedRangeValueClamped(
      FVector2D(0.5f, 1.2f), FVector2D(1.f, OverloadedSpeedFactor), LoadRatio);

  const bool bSprintActive = bWantsSprint && !bExhausted;

  UCharacterMovementComponent *Move = GetCharacterMovement();
  Move->MaxWalkSpeed =
      BaseWalkSpeed * Penalty * (bSprintActive ? SprintMultiplier : 1.f);
  Move->MaxAcceleration = BaseAcceleration * Penalty;
  Move->MaxWalkSpeedCrouched = BaseWalkSpeed * Penalty * CrouchSpeedFactor;
  Move->RotationRate = FRotator(0.f,
                                FMath::Lerp(MaxTurnRate, LoadedTurnRate,
                                            FMath::Clamp(LoadRatio, 0.f, 1.f)),
                                0.f);
}

void AOperatorCharacter::SetSprinting(bool bNewWantsSprint) {
  if (bNewWantsSprint && bIsCrouched) {
    UnCrouch(); // correr levanta o Operador
  }
  bWantsSprint = bNewWantsSprint;
  const bool bShouldDrain = bWantsSprint && !bExhausted;

  if (bShouldDrain && !SprintDrainHandle.IsValid()) {
    SprintDrainHandle = ApplyEffectToSelf(SprintDrainEffect);
    if (SprintDrainHandle.IsValid()) {
      AbilitySystem->AddLooseGameplayTag(KrastovTags::State_Sprinting);
    }
  } else if (!bShouldDrain) {
    StopSprintDrain();
  }

  RecalculateMovement();
}

void AOperatorCharacter::StopSprintDrain() {
  if (SprintDrainHandle.IsValid()) {
    AbilitySystem->RemoveActiveGameplayEffect(SprintDrainHandle);
    AbilitySystem->RemoveLooseGameplayTag(KrastovTags::State_Sprinting);
    SprintDrainHandle.Invalidate();
  }
}

void AOperatorCharacter::HandleCarryWeightChanged(
    const FOnAttributeChangeData &Data) {
  RecalculateMovement();
}

void AOperatorCharacter::HandleStaminaChanged(
    const FOnAttributeChangeData &Data) {
  if (!bExhausted && Data.NewValue <= KINDA_SMALL_NUMBER) {
    bExhausted = true;
    AbilitySystem->AddLooseGameplayTag(KrastovTags::State_Exhausted);
    StopSprintDrain();
    RecalculateMovement();
    OnExhausted();
  } else if (bExhausted && Data.NewValue >= Attributes->GetMaxStamina() *
                                                ExhaustionRecoverRatio) {
    bExhausted = false;
    AbilitySystem->RemoveLooseGameplayTag(KrastovTags::State_Exhausted);
    SetSprinting(
        bWantsSprint); // volta a correr se o botão ainda estiver pressionado
    OnRecoveredFromExhaustion();
  }
}

void AOperatorCharacter::ToggleFlashlight() {
  const bool bOn = !Flashlight->IsVisible();
  Flashlight->SetVisibility(bOn);

  if (bOn) {
    AbilitySystem->AddLooseGameplayTag(KrastovTags::State_FlashlightOn);
  } else {
    AbilitySystem->RemoveLooseGameplayTag(KrastovTags::State_FlashlightOn);
  }

  OnFlashlightToggled(bOn);
}

bool AOperatorCharacter::IsFlashlightOn() const {
  return Flashlight && Flashlight->IsVisible();
}

void AOperatorCharacter::ToggleCrouch() {
  if (bIsCrouched) {
    UnCrouch();
  } else {
    SetSprinting(false);
    Crouch();
  }
}

void AOperatorCharacter::UpdateFootsteps() {
  const UCharacterMovementComponent *Move = GetCharacterMovement();
  const float Speed = GetVelocity().Size2D();

  if (!Move->IsMovingOnGround() || Speed < 10.f) {
    StrideAccumulator = 0.f;
    return;
  }

  StrideAccumulator += Speed * FootstepCheckInterval;
  if (StrideAccumulator < StrideLength) {
    return;
  }
  StrideAccumulator = 0.f;

  const bool bSprintActive = bWantsSprint && !bExhausted && !bIsCrouched;
  const float NoiseRange =
      bIsCrouched ? CrouchNoiseRange
                  : (bSprintActive ? SprintNoiseRange : WalkNoiseRange);

  UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.f, this,
                                     NoiseRange, TEXT("Footstep"));

  if (bDebugDrawNoise) {
    DrawDebugSphere(GetWorld(), GetActorLocation(), NoiseRange, 16,
                    FColor::Yellow, false, 0.4f);
  }

  OnFootstep(NoiseRange); // BP: som do passo
}
