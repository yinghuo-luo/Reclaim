// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ReclaimEnemyActionRules.h"

EReclaimEnemyActionValidationResult UReclaimEnemyActionRules::EvaluateActionCommitPreconditions(
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
	bool bRequiresLineOfSight)
{
	if (!bHasEnemy)
	{
		return EReclaimEnemyActionValidationResult::InvalidEnemy;
	}

	if (!bEnemyCanAct)
	{
		return EReclaimEnemyActionValidationResult::EnemyUnavailable;
	}

	if (!bHasDefinition)
	{
		return EReclaimEnemyActionValidationResult::MissingDefinition;
	}

	if (!bHasTarget)
	{
		return EReclaimEnemyActionValidationResult::InvalidTarget;
	}

	if (!bTargetActive)
	{
		return EReclaimEnemyActionValidationResult::TargetUnavailable;
	}

	if (bRequiresDamageReceiver && !bHasDamageReceiver)
	{
		return EReclaimEnemyActionValidationResult::MissingDamageReceiver;
	}

	if (!bInRange)
	{
		return EReclaimEnemyActionValidationResult::OutOfRange;
	}

	if (bRequiresLineOfSight && !bHasLineOfSight)
	{
		return EReclaimEnemyActionValidationResult::LineOfSightBlocked;
	}

	if (!bCooldownReady)
	{
		return EReclaimEnemyActionValidationResult::CooldownBlocked;
	}

	return EReclaimEnemyActionValidationResult::Success;
}
