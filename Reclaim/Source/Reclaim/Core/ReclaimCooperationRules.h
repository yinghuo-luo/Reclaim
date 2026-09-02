// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimCooperationRules.generated.h"

UCLASS()
class RECLAIM_API UReclaimCooperationRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|Cooperation")
	static bool IsCombatCapable(EReclaimPlayerLifeState LifeState);

	UFUNCTION(BlueprintPure, Category="Reclaim|Cooperation")
	static bool CanUseNormalAbility(EReclaimPlayerLifeState LifeState, bool bHasCannotUseAbilityTag);

	UFUNCTION(BlueprintPure, Category="Reclaim|Cooperation")
	static bool CanFireWeapon(EReclaimPlayerLifeState LifeState, bool bHasCannotFireTag);

	UFUNCTION(BlueprintPure, Category="Reclaim|Cooperation")
	static EReclaimReviveValidationResult EvaluateReviveStart(
		EReclaimPlayerLifeState ReviverLifeState,
		EReclaimPlayerLifeState TargetLifeState,
		float Distance,
		float MaxDistance,
		bool bSamePlayer,
		bool bFriendly);

	UFUNCTION(BlueprintPure, Category="Reclaim|Cooperation")
	static bool ShouldInterruptRevive(
		EReclaimPlayerLifeState ReviverLifeState,
		EReclaimPlayerLifeState TargetLifeState,
		float Distance,
		float MaxDistance,
		bool bCancelRequested);

	UFUNCTION(BlueprintPure, Category="Reclaim|Cooperation")
	static bool CanRedeploy(EReclaimPlayerLifeState LifeState, int32 RedeploysRemaining);

	UFUNCTION(BlueprintPure, Category="Reclaim|Cooperation")
	static bool ShouldTriggerTeamWipe(const TArray<FReclaimParticipantLifeSnapshot>& Participants);
};
