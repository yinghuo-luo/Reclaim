// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ReclaimCooperationRules.h"

bool UReclaimCooperationRules::IsCombatCapable(EReclaimPlayerLifeState LifeState)
{
	return LifeState == EReclaimPlayerLifeState::Active;
}

bool UReclaimCooperationRules::CanUseNormalAbility(EReclaimPlayerLifeState LifeState, bool bHasCannotUseAbilityTag)
{
	return IsCombatCapable(LifeState) && !bHasCannotUseAbilityTag;
}

bool UReclaimCooperationRules::CanFireWeapon(EReclaimPlayerLifeState LifeState, bool bHasCannotFireTag)
{
	return IsCombatCapable(LifeState) && !bHasCannotFireTag;
}

EReclaimReviveValidationResult UReclaimCooperationRules::EvaluateReviveStart(
	EReclaimPlayerLifeState ReviverLifeState,
	EReclaimPlayerLifeState TargetLifeState,
	float Distance,
	float MaxDistance,
	bool bSamePlayer,
	bool bFriendly)
{
	if (!IsCombatCapable(ReviverLifeState))
	{
		return EReclaimReviveValidationResult::ReviverUnavailable;
	}

	if (TargetLifeState != EReclaimPlayerLifeState::Downed)
	{
		return EReclaimReviveValidationResult::TargetNotDowned;
	}

	if (bSamePlayer)
	{
		return EReclaimReviveValidationResult::SamePlayer;
	}

	if (!bFriendly)
	{
		return EReclaimReviveValidationResult::NotFriendly;
	}

	if (MaxDistance >= 0.0f && Distance > MaxDistance)
	{
		return EReclaimReviveValidationResult::TooFar;
	}

	return EReclaimReviveValidationResult::Success;
}

bool UReclaimCooperationRules::ShouldInterruptRevive(
	EReclaimPlayerLifeState ReviverLifeState,
	EReclaimPlayerLifeState TargetLifeState,
	float Distance,
	float MaxDistance,
	bool bCancelRequested)
{
	if (bCancelRequested)
	{
		return true;
	}

	return EvaluateReviveStart(ReviverLifeState, TargetLifeState, Distance, MaxDistance, false, true) != EReclaimReviveValidationResult::Success;
}

bool UReclaimCooperationRules::CanRedeploy(EReclaimPlayerLifeState LifeState, int32 RedeploysRemaining)
{
	return LifeState == EReclaimPlayerLifeState::Destroyed && RedeploysRemaining > 0;
}

bool UReclaimCooperationRules::ShouldTriggerTeamWipe(const TArray<FReclaimParticipantLifeSnapshot>& Participants)
{
	bool bFoundParticipant = false;
	for (const FReclaimParticipantLifeSnapshot& Participant : Participants)
	{
		if (!Participant.bParticipating)
		{
			continue;
		}

		bFoundParticipant = true;
		if (Participant.LifeState != EReclaimPlayerLifeState::Destroyed || Participant.RedeploysRemaining > 0)
		{
			return false;
		}
	}

	return bFoundParticipant;
}
