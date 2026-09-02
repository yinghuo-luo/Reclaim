// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/ReclaimScanAbility.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Core/ReclaimGameplayTags.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Interfaces/ReclaimTargetContracts.h"

void UReclaimScanAbility::ActivateAbility(
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
	if (!CommitReclaimAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = Character->GetWorld();
	if (World)
	{
		const float Radius = FMath::Max(0.0f, AbilityDefinition->Radius);
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimTacticalScan), true, Character);
		QueryParams.AddIgnoredActor(Character);
		World->OverlapMultiByChannel(Overlaps, Character->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), QueryParams);

		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* TargetActor = Overlap.GetActor();
			if (!TargetActor || !TargetActor->GetClass()->ImplementsInterface(UReclaimTargetableInterface::StaticClass()))
			{
				continue;
			}

			if (IReclaimTargetableInterface::Execute_CanBeMarkedBy(TargetActor, Character))
			{
				IReclaimTargetableInterface::Execute_ApplyMark_Server(TargetActor, Character, AbilityDefinition->Duration, ReclaimGameplayTags::State_Marked);
			}
		}
	}

	ConfirmAbilityPresentation(AbilityDefinition->GetPrimaryAbilityTag(), Character->GetActorLocation());
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
