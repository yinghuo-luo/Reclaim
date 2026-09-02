// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/ReclaimEnemyCombatBrain.h"
#include "ReclaimEnemyActionExecutorComponent.generated.h"

UCLASS(ClassGroup=(Reclaim), meta=(BlueprintSpawnableComponent))
class RECLAIM_API UReclaimEnemyActionExecutorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReclaimEnemyActionExecutorComponent();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|AI")
	void ExecuteIntent(const FReclaimEnemyIntent& Intent);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|AI")
	void ExecuteIntentAgainstTarget(const FReclaimEnemyIntent& Intent, AActor* TargetActor);

protected:
	void StopMovement_Server() const;
	void MoveToActor_Server(AActor* TargetActor) const;
	void MoveToLocation_Server(const FVector& Destination) const;
	void FaceTarget_Server(AActor* TargetActor) const;
	void ExecuteMeleeDamage_Server(AActor* TargetActor, float DamageAmount, EReclaimEnemyIntentType CooldownIntent) const;
	void ExecuteProjectileAttack_Server(AActor* TargetActor) const;
	void ExecuteLeapAttack_Server(AActor* TargetActor) const;
	void ExecuteCharge_Server(AActor* TargetActor) const;
	FVector ProjectRepositionLocation_Server(AActor* TargetActor, float DesiredDistance, bool bRetreat) const;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|AI")
	FReclaimEnemyIntent LastExecutedIntent;
};
