// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/ReclaimGenericCombatAbility.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Core/ReclaimGameplayTags.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Interfaces/ReclaimTargetContracts.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

namespace
{
	bool IsActiveSourceCharacter(const AReclaimPlayerCharacter* Character)
	{
		const AReclaimPlayerState* PlayerState = Character ? Character->GetReclaimPlayerState() : nullptr;
		return Character && Character->HasAuthority() && PlayerState && PlayerState->GetLifeState() == EReclaimPlayerLifeState::Active;
	}

	float ResolveOverclockRange(const UReclaimAbilityDefinition* AbilityDefinition)
	{
		if (!AbilityDefinition)
		{
			return 0.0f;
		}

		if (AbilityDefinition->Range > 0.0f)
		{
			return AbilityDefinition->Range;
		}

		return AbilityDefinition->Radius > 0.0f ? AbilityDefinition->Radius : 1200.0f;
	}

	bool IsOverclockTargetValid(AReclaimPlayerCharacter* Character, AActor* Candidate, const UReclaimAbilityDefinition* AbilityDefinition)
	{
		if (!IsActiveSourceCharacter(Character) || !Candidate || Candidate->IsPendingKillPending() || !AbilityDefinition)
		{
			return false;
		}

		if (!Candidate->GetClass()->ImplementsInterface(UReclaimOverclockableInterface::StaticClass()))
		{
			return false;
		}

		const float Range = ResolveOverclockRange(AbilityDefinition);
		if (Range > 0.0f && FVector::DistSquared(Character->GetActorLocation(), Candidate->GetActorLocation()) > FMath::Square(Range))
		{
			return false;
		}

		return IReclaimOverclockableInterface::Execute_CanBeOverclockedBy(Candidate, Character->GetController());
	}

	AActor* ResolveOverclockTarget(AReclaimPlayerCharacter* Character, const FReclaimAbilityActivationPayload& Payload, const UReclaimAbilityDefinition* AbilityDefinition)
	{
		if (IsOverclockTargetValid(Character, Payload.TargetActor.Get(), AbilityDefinition))
		{
			return Payload.TargetActor.Get();
		}

		TArray<AActor*> Candidates;
		UGameplayStatics::GetAllActorsWithInterface(Character, UReclaimOverclockableInterface::StaticClass(), Candidates);
		AActor* BestCandidate = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (AActor* Candidate : Candidates)
		{
			if (!IsOverclockTargetValid(Character, Candidate, AbilityDefinition))
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared(Character->GetActorLocation(), Candidate->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestCandidate = Candidate;
				BestDistanceSquared = DistanceSquared;
			}
		}

		return BestCandidate;
	}
}

void UReclaimGenericCombatAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	const UReclaimAbilityDefinition* AbilityDefinition = GetReclaimAbilityDefinition();
	AReclaimPlayerCharacter* Character = GetReclaimAvatarCharacter();
	if (!AbilityDefinition || !Character || !Character->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FReclaimAbilityActivationPayload Payload;
	ConsumeActivationPayload(Payload);

	AActor* OverclockTarget = nullptr;
	if (AbilityDefinition->CombatExecutionMode == EReclaimCombatExecutionMode::Overclock)
	{
		OverclockTarget = ResolveOverclockTarget(Character, Payload, AbilityDefinition);
		if (!OverclockTarget)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	if (!CommitReclaimAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AbilityDefinition->CombatExecutionMode == EReclaimCombatExecutionMode::PurificationPulse)
	{
		UWorld* World = Character->GetWorld();
		if (World)
		{
			const float Radius = FMath::Max(0.0f, AbilityDefinition->Radius);
			TArray<FOverlapResult> Overlaps;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimPurificationPulse), true, Character);
			QueryParams.AddIgnoredActor(Character);
			World->OverlapMultiByChannel(Overlaps, Character->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), QueryParams);

			for (const FOverlapResult& Overlap : Overlaps)
			{
				AActor* TargetActor = Overlap.GetActor();
				if (!TargetActor || !TargetActor->GetClass()->ImplementsInterface(UReclaimPurifiableInterface::StaticClass()))
				{
					continue;
				}

				if (IReclaimPurifiableInterface::Execute_CanBePurifiedBy(TargetActor, Character))
				{
					IReclaimPurifiableInterface::Execute_Purify_Server(TargetActor, Character, AbilityDefinition->Value);
				}
			}
		}
	}
	else if (AbilityDefinition->CombatExecutionMode == EReclaimCombatExecutionMode::Overclock)
	{
		if (IsOverclockTargetValid(Character, OverclockTarget, AbilityDefinition))
		{
			IReclaimOverclockableInterface::Execute_ApplyOverclock_Server(OverclockTarget, Character->GetController(), AbilityDefinition->Duration, 1.0f + AbilityDefinition->Value, 1.0f, 1.0f + AbilityDefinition->SecondaryValue);
		}
	}

	ConfirmAbilityPresentation(AbilityDefinition->GetPrimaryAbilityTag(), Character->GetActorLocation());
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
