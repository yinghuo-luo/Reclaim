// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReclaimEnemyActionRules.generated.h"

UENUM(BlueprintType)
enum class EReclaimEnemyActionValidationResult : uint8
{
	Success,
	InvalidEnemy,
	EnemyUnavailable,
	MissingDefinition,
	InvalidTarget,
	TargetUnavailable,
	MissingDamageReceiver,
	OutOfRange,
	LineOfSightBlocked,
	CooldownBlocked
};

UCLASS()
class RECLAIM_API UReclaimEnemyActionRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|AI")
	static EReclaimEnemyActionValidationResult EvaluateActionCommitPreconditions(
		bool bHasEnemy,
		bool bEnemyCanAct,
		bool bHasDefinition,
		bool bHasTarget,
		bool bTargetActive,
		bool bHasDamageReceiver,
		bool bInRange,
		bool bHasLineOfSight,
		bool bCooldownReady,
		bool bRequiresDamageReceiver,
		bool bRequiresLineOfSight);
};
