// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/ReclaimEnemyCombatBrain.h"
#include "ReclaimEnemyAIController.generated.h"

class UReclaimEnemyActionExecutorComponent;

UCLASS()
class RECLAIM_API AReclaimEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AReclaimEnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|AI")
	void StopAI_Server();

protected:
	void StartBrainLoop_Server();
	void TickBrain_Server();
	AActor* SelectTarget_Server() const;
	bool IsValidEnemyTarget_Server(AActor* CandidateActor) const;
	bool HasLineOfSightToTarget_Server(AActor* TargetActor) const;
	FReclaimEnemyFacts BuildFacts_Server(AActor* TargetActor) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|AI")
	TObjectPtr<UReclaimEnemyActionExecutorComponent> ActionExecutorComponent;

	UPROPERTY(Transient)
	TObjectPtr<UReclaimEnemyCombatBrain> CombatBrain;

	FTimerHandle BrainTimerHandle;
	int32 DecisionCounter = 0;
};
