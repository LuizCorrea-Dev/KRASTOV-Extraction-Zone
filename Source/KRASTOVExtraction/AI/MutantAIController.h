#pragma once

#include "AI/MutantCharacter.h"
#include "AIController.h"
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "MutantAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AOperatorCharacter;

UCLASS()
class KRASTOVEXTRACTION_API AMutantAIController : public AAIController {
  GENERATED_BODY()

public:
  AMutantAIController();

  UFUNCTION(BlueprintPure, Category = "Krastov|AI")
  EMutantState GetMutantState() const { return State; }

protected:
  virtual void BeginPlay() override;
  virtual void OnPossess(APawn *InPawn) override;
  virtual void OnUnPossess() override;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Krastov|AI")
  TObjectPtr<UAIPerceptionComponent> Perception;

  UPROPERTY()
  TObjectPtr<UAISenseConfig_Sight> SightConfig;

  UPROPERTY()
  TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

  // ---------- Percepção ----------
  /** Até onde enxerga um Operador com a lanterna APAGADA */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception")
  float DarkSightRange = 1000.f;

  /** Até onde enxerga um Operador com a lanterna ACESA (também é o raio do
   * sensor) */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception")
  float FlashlightSightRange = 2400.f;

  /** Meio-ângulo do cone de visão (45 = cone total de 90°) */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception",
            meta = (ClampMin = "5", ClampMax = "180"))
  float VisionHalfAngle = 45.f;

  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception")
  float HearingRange = 2500.f;

  // ---------- Comportamento ----------
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float PatrolRadius = 1500.f;
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float PatrolSpeed = 150.f;
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float InvestigateSpeed = 250.f;
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float ChaseSpeed = 400.f;
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float AttackRange = 150.f;
  /** Tempo parado "farejando" no local investigado */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float InvestigateWaitTime = 4.f;
  /** Quanto tempo continua perseguindo depois de perder o Operador de vista */
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float ChaseMemoryTime = 2.f;
  UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior")
  float ThinkInterval = 0.2f;

private:
  UFUNCTION()
  void HandleTargetPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus);

  void Think();
  void SetState(EMutantState NewState);
  void EnterState(EMutantState NewState);
  void StartInvestigation(const FVector &Location);

  void TickPatrol();
  void TickInvestigate();
  void TickChase();
  void TickAttack();

  bool CanSeeOperatorNow() const;
  void ApplySenseConfig();

  EMutantState State = EMutantState::Patrol;
  TWeakObjectPtr<AOperatorCharacter> Operator;

  bool bOperatorInSightCone = false;
  bool bSeenThisThink = false;
  bool bWaitingAtInvestigation = false;
  float LastSeenTime = -1000.f;
  float StateEnterTime = 0.f;
  FVector LastKnownLocation = FVector::ZeroVector;

  FTimerHandle ThinkTimer;
};
