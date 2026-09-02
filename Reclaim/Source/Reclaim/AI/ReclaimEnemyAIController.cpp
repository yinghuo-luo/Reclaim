// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ReclaimEnemyAIController.h"

#include "AI/ReclaimAIConfig.h"
#include "AI/ReclaimEnemyActionExecutorComponent.h"
#include "AI/ReclaimEnemyCombatBrain.h"
#include "Characters/ReclaimEnemyCharacter.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Core/ReclaimCooperationRules.h"
#include "Core/ReclaimDeterministicRandom.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

AReclaimEnemyAIController::AReclaimEnemyAIController()
{
	ActionExecutorComponent = CreateDefaultSubobject<UReclaimEnemyActionExecutorComponent>(TEXT("ActionExecutorComponent"));
	CombatBrain = CreateDefaultSubobject<UReclaimEnemyCombatBrain>(TEXT("CombatBrain"));
	PrimaryActorTick.bCanEverTick = false;
}

void AReclaimEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HasAuthority())
	{
		StartBrainLoop_Server();
	}
}

void AReclaimEnemyAIController::OnUnPossess()
{
	StopAI_Server();
	Super::OnUnPossess();
}

void AReclaimEnemyAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAI_Server();
	Super::EndPlay(EndPlayReason);
}

void AReclaimEnemyAIController::StopAI_Server()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BrainTimerHandle);
	}

	StopMovement();
}

void AReclaimEnemyAIController::StartBrainLoop_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(GetPawn());
	if (!Enemy || Enemy->IsDead())
	{
		return;
	}

	const UReclaimAIConfig* Config = Enemy->GetAIConfig();
	const float Interval = Config ? FMath::Max(0.05f, Config->DecisionInterval) : 0.35f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BrainTimerHandle, this, &AReclaimEnemyAIController::TickBrain_Server, Interval, true, 0.05f);
	}
}

void AReclaimEnemyAIController::TickBrain_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(GetPawn());
	if (!Enemy || Enemy->IsDead())
	{
		StopAI_Server();
		return;
	}

	AActor* TargetActor = SelectTarget_Server();
	const FReclaimEnemyFacts Facts = BuildFacts_Server(TargetActor);
	const FReclaimEnemyIntent Intent = CombatBrain
		? CombatBrain->DecideIntentForEnemy(Facts, Enemy->GetEnemyDefinition(), Enemy->GetAIConfig())
		: FReclaimEnemyIntent();

	Enemy->SetCurrentAITarget_Server(TargetActor);
	Enemy->SetCurrentAIIntent_Server(Intent.Type);

	if (ActionExecutorComponent)
	{
		ActionExecutorComponent->ExecuteIntentAgainstTarget(Intent, TargetActor);
	}

	++DecisionCounter;
}

AActor* AReclaimEnemyAIController::SelectTarget_Server() const
{
	const APawn* EnemyPawn = GetPawn();
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!EnemyPawn || !GameState)
	{
		return nullptr;
	}

	const AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(EnemyPawn);
	const UReclaimAIConfig* Config = Enemy ? Enemy->GetAIConfig() : nullptr;
	const float AcquireRadius = Config ? FMath::Max(0.0f, Config->TargetAcquireRadius) : 5000.0f;

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (APlayerState* CandidatePlayerState : GameState->PlayerArray)
	{
		const AReclaimPlayerState* ReclaimPlayerState = Cast<AReclaimPlayerState>(CandidatePlayerState);
		AController* PlayerController = ReclaimPlayerState ? Cast<AController>(ReclaimPlayerState->GetOwner()) : nullptr;
		AActor* CandidateActor = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!IsValidEnemyTarget_Server(CandidateActor))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(EnemyPawn->GetActorLocation(), CandidateActor->GetActorLocation());
		if (AcquireRadius > 0.0f && DistanceSquared > FMath::Square(AcquireRadius))
		{
			continue;
		}

		const bool bHasLOS = HasLineOfSightToTarget_Server(CandidateActor);
		const float Score = DistanceSquared * (bHasLOS ? 1.0f : 1.35f);
		if (Score < BestScore)
		{
			BestTarget = CandidateActor;
			BestScore = Score;
		}
	}

	return BestTarget;
}

bool AReclaimEnemyAIController::IsValidEnemyTarget_Server(AActor* CandidateActor) const
{
	const APawn* CandidatePawn = Cast<APawn>(CandidateActor);
	const AReclaimPlayerState* ReclaimPlayerState = CandidatePawn ? CandidatePawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	return CandidatePawn
		&& ReclaimPlayerState
		&& UReclaimCooperationRules::IsCombatCapable(ReclaimPlayerState->GetLifeState())
		&& !CandidatePawn->IsPendingKillPending();
}

bool AReclaimEnemyAIController::HasLineOfSightToTarget_Server(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	return LineOfSightTo(TargetActor);
}

FReclaimEnemyFacts AReclaimEnemyAIController::BuildFacts_Server(AActor* TargetActor) const
{
	FReclaimEnemyFacts Facts;
	const AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(GetPawn());
	if (!Enemy)
	{
		Facts.bDead = true;
		return Facts;
	}

	Facts.bDead = Enemy->IsDead();
	Facts.bHasValidTarget = IsValidEnemyTarget_Server(TargetActor);
	Facts.bHasLineOfSight = HasLineOfSightToTarget_Server(TargetActor);
	Facts.PreviousIntent = Enemy->GetCurrentAIIntent();
	Facts.DecisionSeed = UReclaimDeterministicRandom::MakeSubSeed(Enemy->GetDecisionSeed(), EReclaimRandomDomain::AICombatDecision, DecisionCounter);
	Facts.bMeleeReady = Enemy->IsActionReady(EReclaimEnemyIntentType::MeleeAttack);
	Facts.bRangedReady = Enemy->IsActionReady(EReclaimEnemyIntentType::RangedAttack);
	Facts.bLeapReady = Enemy->IsActionReady(EReclaimEnemyIntentType::LeapAttack);
	Facts.bHeavyReady = Enemy->IsActionReady(EReclaimEnemyIntentType::HeavyAttack);
	Facts.bChargeReady = Enemy->IsActionReady(EReclaimEnemyIntentType::Charge);
	Facts.bSkillReady = Facts.bMeleeReady || Facts.bRangedReady || Facts.bLeapReady || Facts.bHeavyReady || Facts.bChargeReady;

	if (TargetActor)
	{
		Facts.DistanceToTarget = FVector::Dist(Enemy->GetActorLocation(), TargetActor->GetActorLocation());
		if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
		{
			const AReclaimPlayerState* TargetPlayerState = TargetPawn->GetPlayerState<AReclaimPlayerState>();
			Facts.TargetLifeState = TargetPlayerState ? TargetPlayerState->GetLifeState() : EReclaimPlayerLifeState::Destroyed;
		}
	}

	if (const UReclaimHealthShieldComponent* HealthShield = Enemy->GetHealthShieldComponent())
	{
		const float MaxHealth = FMath::Max(1.0f, HealthShield->GetMaxHealth());
		Facts.OwnHealthRatio = FMath::Clamp(HealthShield->GetHealth() / MaxHealth, 0.0f, 1.0f);
	}

	return Facts;
}
