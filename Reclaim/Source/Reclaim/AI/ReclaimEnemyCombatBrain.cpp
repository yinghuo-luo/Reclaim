// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ReclaimEnemyCombatBrain.h"

#include "AI/ReclaimEnemyDefinition.h"
#include "Core/ReclaimGameplayTags.h"

namespace
{
	float ResolvePreferredDistance(const UReclaimEnemyDefinition* EnemyDefinition, const UReclaimAIConfig* Config)
	{
		if (EnemyDefinition && EnemyDefinition->PreferredDistance > 0.0f)
		{
			return EnemyDefinition->PreferredDistance;
		}

		return Config ? Config->DefaultPreferredDistance : 0.0f;
	}

	float ResolveAttackDistance(const UReclaimEnemyDefinition* EnemyDefinition, const UReclaimAIConfig* Config)
	{
		if (EnemyDefinition && EnemyDefinition->AttackDistance > 0.0f)
		{
			return EnemyDefinition->AttackDistance;
		}

		return Config ? Config->DefaultAttackDistance : 0.0f;
	}

	float ResolveRetreatDistance(const UReclaimEnemyDefinition* EnemyDefinition, const UReclaimAIConfig* Config)
	{
		if (EnemyDefinition && EnemyDefinition->RetreatDistance > 0.0f)
		{
			return EnemyDefinition->RetreatDistance;
		}

		return Config ? Config->DefaultRetreatDistance : 0.0f;
	}

	bool WantsReposition(const FReclaimEnemyFacts& Facts)
	{
		return (Facts.DecisionSeed & 1) == 0;
	}
}

FReclaimEnemyIntent UReclaimEnemyCombatBrain::DecideIntent(const FReclaimEnemyFacts& Facts, const UReclaimAIConfig* Config) const
{
	return DecideIntentForEnemy(Facts, nullptr, Config);
}

FReclaimEnemyIntent UReclaimEnemyCombatBrain::DecideIntentForEnemy(const FReclaimEnemyFacts& Facts, const UReclaimEnemyDefinition* EnemyDefinition, const UReclaimAIConfig* Config) const
{
	FReclaimEnemyIntent Intent;
	Intent.DesiredRange = ResolvePreferredDistance(EnemyDefinition, Config);

	if (Facts.bDead)
	{
		Intent.Type = EReclaimEnemyIntentType::Dead;
		LastIntent = Intent;
		return Intent;
	}

	if (!Facts.bHasValidTarget || Facts.TargetLifeState != EReclaimPlayerLifeState::Active)
	{
		Intent.Type = EReclaimEnemyIntentType::Wait;
		LastIntent = Intent;
		return Intent;
	}

	const float AttackDistance = ResolveAttackDistance(EnemyDefinition, Config);
	const float RetreatDistance = ResolveRetreatDistance(EnemyDefinition, Config);
	const bool bCanMelee = !EnemyDefinition || EnemyDefinition->bCanMeleeAttack;
	const bool bCanRanged = EnemyDefinition && EnemyDefinition->bCanRangedAttack;
	const bool bCanLeap = EnemyDefinition && EnemyDefinition->bCanLeapAttack;
	const bool bCanHeavy = EnemyDefinition && EnemyDefinition->bCanHeavyAttack;
	const bool bCanCharge = EnemyDefinition && EnemyDefinition->bCanCharge;
	const bool bCanReposition = !EnemyDefinition || EnemyDefinition->bCanReposition;

	if (bCanCharge && Facts.bChargeReady && Facts.DistanceToTarget >= EnemyDefinition->ChargeMinDistance && Facts.DistanceToTarget <= EnemyDefinition->ChargeMaxDistance && Facts.bHasLineOfSight)
	{
		Intent.Type = EReclaimEnemyIntentType::Charge;
		LastIntent = Intent;
		return Intent;
	}

	if (bCanLeap && Facts.bLeapReady && Facts.DistanceToTarget >= EnemyDefinition->LeapMinDistance && Facts.DistanceToTarget <= EnemyDefinition->LeapMaxDistance && Facts.bHasLineOfSight)
	{
		Intent.Type = EReclaimEnemyIntentType::LeapAttack;
		LastIntent = Intent;
		return Intent;
	}

	if (bCanHeavy && Facts.bHeavyReady && Facts.DistanceToTarget <= FMath::Max(AttackDistance, EnemyDefinition->AttackDistance))
	{
		Intent.Type = EReclaimEnemyIntentType::HeavyAttack;
		LastIntent = Intent;
		return Intent;
	}

	if (bCanMelee && Facts.bMeleeReady && Facts.DistanceToTarget <= AttackDistance)
	{
		Intent.Type = EReclaimEnemyIntentType::MeleeAttack;
		LastIntent = Intent;
		return Intent;
	}

	if (bCanRanged)
	{
		if (Facts.DistanceToTarget < RetreatDistance)
		{
			Intent.Type = bCanReposition ? EReclaimEnemyIntentType::Reposition : EReclaimEnemyIntentType::Retreat;
			LastIntent = Intent;
			return Intent;
		}

		if (Facts.bRangedReady && Facts.bHasLineOfSight && Facts.DistanceToTarget <= Intent.DesiredRange * 1.35f)
		{
			Intent.Type = EReclaimEnemyIntentType::RangedAttack;
			LastIntent = Intent;
			return Intent;
		}

		if (Facts.DistanceToTarget > Intent.DesiredRange || !Facts.bHasLineOfSight)
		{
			Intent.Type = EReclaimEnemyIntentType::Approach;
			LastIntent = Intent;
			return Intent;
		}

		Intent.Type = WantsReposition(Facts) && bCanReposition ? EReclaimEnemyIntentType::Reposition : EReclaimEnemyIntentType::MaintainRange;
		LastIntent = Intent;
		return Intent;
	}

	if (Facts.DistanceToTarget > AttackDistance)
	{
		Intent.Type = EReclaimEnemyIntentType::Approach;
		LastIntent = Intent;
		return Intent;
	}

	Intent.Type = EReclaimEnemyIntentType::Wait;
	LastIntent = Intent;
	return Intent;
}
