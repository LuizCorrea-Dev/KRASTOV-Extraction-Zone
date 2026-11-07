#include "AI/MutantAIController.h"

#include "AI/MutantCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Player/OperatorCharacter.h"
#include "TimerManager.h"

AMutantAIController::AMutantAIController() {
  Perception =
      CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
  SetPerceptionComponent(*Perception);

  SightConfig =
      CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
  HearingConfig =
      CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

  SightConfig->DetectionByAffiliation.bDetectEnemies = true;
  SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
  SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
  SightConfig->SetMaxAge(5.f);

  HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
  HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
  HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
  HearingConfig->SetMaxAge(3.f);

  ApplySenseConfig();
  Perception->SetDominantSense(UAISense_Sight::StaticClass());
}

void AMutantAIController::ApplySenseConfig() {
  SightConfig->SightRadius = FlashlightSightRange;
  SightConfig->LoseSightRadius = FlashlightSightRange + 300.f;
  SightConfig->PeripheralVisionAngleDegrees = VisionHalfAngle;
  HearingConfig->HearingRange = HearingRange;

  Perception->ConfigureSense(*SightConfig);
  Perception->ConfigureSense(*HearingConfig);
}

void AMutantAIController::BeginPlay() {
  Super::BeginPlay();
  ApplySenseConfig(); // aplica valores editados no Blueprint
  Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(
      this, &AMutantAIController::HandleTargetPerceptionUpdated);
}

void AMutantAIController::OnPossess(APawn *InPawn) {
  Super::OnPossess(InPawn);
  EnterState(EMutantState::Patrol);
  GetWorldTimerManager().SetTimer(ThinkTimer, this, &AMutantAIController::Think,
                                  ThinkInterval, true);
}

void AMutantAIController::OnUnPossess() {
  GetWorldTimerManager().ClearTimer(ThinkTimer);
  Super::OnUnPossess();
}

// ============================================================ Percepção

void AMutantAIController::HandleTargetPerceptionUpdated(AActor *Actor,
                                                        FAIStimulus Stimulus) {
  AOperatorCharacter *SensedOperator = Cast<AOperatorCharacter>(Actor);
  if (!SensedOperator) {
    return; // ignora outros mutantes
  }
  Operator = SensedOperator;

  const TSubclassOf<UAISense> Sense =
      UAIPerceptionSystem::GetSenseClassForStimulus(this, Stimulus);

  if (Sense == UAISense_Sight::StaticClass()) {
    bOperatorInSightCone = Stimulus.WasSuccessfullySensed();
    if (!bOperatorInSightCone) {
      LastKnownLocation =
          Stimulus.StimulusLocation; // onde foi visto pela última vez
    }
  } else if (Sense == UAISense_Hearing::StaticClass()) {
    if (Stimulus.WasSuccessfullySensed() && State != EMutantState::Chase &&
        State != EMutantState::Attack) {
      StartInvestigation(Stimulus.StimulusLocation);
    }
  }
}

bool AMutantAIController::CanSeeOperatorNow() const {
  const AOperatorCharacter *Op = Operator.Get();
  const APawn *Me = GetPawn();
  if (!Op || !Me || !bOperatorInSightCone) {
    return false;
  }

  // No escuro o Operador só é visto de perto; com a lanterna acesa, de longe
  const float Range =
      Op->IsFlashlightOn() ? FlashlightSightRange : DarkSightRange;
  return FVector::DistSquared(Op->GetActorLocation(), Me->GetActorLocation()) <=
         FMath::Square(Range);
}

// ============================================================ FSM

void AMutantAIController::Think() {
  if (!GetPawn()) {
    return;
  }

  bSeenThisThink = CanSeeOperatorNow();
  if (bSeenThisThink) {
    LastKnownLocation = Operator->GetActorLocation();
    LastSeenTime = GetWorld()->GetTimeSeconds();
    if (State == EMutantState::Patrol || State == EMutantState::Investigate) {
      SetState(EMutantState::Chase);
    }
  }

  switch (State) {
  case EMutantState::Patrol:
    TickPatrol();
    break;
  case EMutantState::Investigate:
    TickInvestigate();
    break;
  case EMutantState::Chase:
    TickChase();
    break;
  case EMutantState::Attack:
    TickAttack();
    break;
  }
}

void AMutantAIController::SetState(EMutantState NewState) {
  if (NewState != State) {
    EnterState(NewState);
  }
}

void AMutantAIController::EnterState(EMutantState NewState) {
  State = NewState;
  StateEnterTime = GetWorld()->GetTimeSeconds();

  ACharacter *MyCharacter = Cast<ACharacter>(GetPawn());
  if (!MyCharacter) {
    return;
  }
  UCharacterMovementComponent *Move = MyCharacter->GetCharacterMovement();

  ClearFocus(EAIFocusPriority::Gameplay);
  Move->bOrientRotationToMovement = true;
  Move->bUseControllerDesiredRotation = false;

  switch (NewState) {
  case EMutantState::Patrol:
    Move->MaxWalkSpeed = PatrolSpeed;
    break;

  case EMutantState::Investigate:
    Move->MaxWalkSpeed = InvestigateSpeed;
    break;

  case EMutantState::Chase:
    Move->MaxWalkSpeed = ChaseSpeed;
    break;

  case EMutantState::Attack:
    StopMovement();
    Move->bOrientRotationToMovement = false;
    Move->bUseControllerDesiredRotation = true; // gira para encarar o alvo
    SetFocus(Operator.Get());
    break;
  }

  if (AMutantCharacter *Mutant = Cast<AMutantCharacter>(MyCharacter)) {
    Mutant->NotifyAIStateChanged(NewState);
  }
}

void AMutantAIController::StartInvestigation(const FVector &Location) {
  SetState(EMutantState::Investigate);
  bWaitingAtInvestigation = false;
  MoveToLocation(Location, 80.f, true, true, true);
}

void AMutantAIController::TickPatrol() {
  if (GetMoveStatus() != EPathFollowingStatus::Idle) {
    return; // ainda andando até o ponto anterior
  }

  UNavigationSystemV1 *NavSystem =
      FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  if (!NavSystem) {
    UE_LOG(LogTemp, Warning,
           TEXT("[Mutant] %s: sistema de navegacao nao encontrado"),
           *GetPawn()->GetName());
    return;
  }

  FNavLocation Point;
  if (!NavSystem->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(),
                                                  PatrolRadius, Point)) {
    UE_LOG(LogTemp, Warning, TEXT("[Mutant] %s: SEM NAVMESH na posicao %s"),
           *GetPawn()->GetName(), *GetPawn()->GetActorLocation().ToString());
    return;
  }

  const EPathFollowingRequestResult::Type Result =
      MoveToLocation(Point.Location, 50.f);
  if (Result == EPathFollowingRequestResult::Failed) {
    UE_LOG(LogTemp, Warning, TEXT("[Mutant] %s: MoveTo falhou para %s"),
           *GetPawn()->GetName(), *Point.Location.ToString());
  }
}

void AMutantAIController::TickInvestigate() {
  const float Now = GetWorld()->GetTimeSeconds();

  if (!bWaitingAtInvestigation) {
    if (GetMoveStatus() == EPathFollowingStatus::Idle) {
      bWaitingAtInvestigation =
          true; // chegou (ou não há caminho): fica parado ouvindo
      StateEnterTime = Now;
    }
  } else if (Now - StateEnterTime >= InvestigateWaitTime) {
    SetState(EMutantState::Patrol);
  }
}

void AMutantAIController::TickChase() {
  AOperatorCharacter *Op = Operator.Get();
  if (!Op) {
    SetState(EMutantState::Patrol);
    return;
  }

  const float Now = GetWorld()->GetTimeSeconds();

  if (bSeenThisThink) {
    const float Distance =
        FVector::Dist2D(Op->GetActorLocation(), GetPawn()->GetActorLocation());
    if (Distance <= AttackRange) {
      SetState(EMutantState::Attack);
      return;
    }
    MoveToActor(Op, AttackRange * 0.7f);
  } else if (Now - LastSeenTime <= ChaseMemoryTime) {
    MoveToLocation(LastKnownLocation,
                   50.f); // vai até onde viu por último (sem "visão de raio X")
  } else {
    StartInvestigation(LastKnownLocation);
  }
}

void AMutantAIController::TickAttack() {
  AOperatorCharacter *Op = Operator.Get();
  if (!Op) {
    SetState(EMutantState::Patrol);
    return;
  }

  const float Distance =
      FVector::Dist2D(Op->GetActorLocation(), GetPawn()->GetActorLocation());
  if (Distance > AttackRange * 1.3f) {
    SetState(EMutantState::Chase);
    return;
  }

  if (AMutantCharacter *Mutant = Cast<AMutantCharacter>(GetPawn())) {
    Mutant->TryAttack(Op);
  }
}
