// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ReclaimGameplayAbility.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "AbilitySystem/ReclaimAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Core/ReclaimCooperationRules.h"
#include "Core/ReclaimGameplayTags.h"
#include "GameFramework/Controller.h"
#include "Player/ReclaimPlayerState.h"

UReclaimGameplayAbility::UReclaimGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

bool UReclaimGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AReclaimPlayerCharacter* Character = Cast<AReclaimPlayerCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	const AReclaimPlayerState* PlayerState = Character ? Character->GetReclaimPlayerState() : nullptr;
	const EReclaimPlayerLifeState LifeState = PlayerState ? PlayerState->GetLifeState() : EReclaimPlayerLifeState::Destroyed;

	const UAbilitySystemComponent* AbilitySystem = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const bool bCannotUseAbilityTag = AbilitySystem && AbilitySystem->HasMatchingGameplayTag(ReclaimGameplayTags::State_CannotUseAbility);
	if (!UReclaimCooperationRules::CanUseNormalAbility(LifeState, bCannotUseAbilityTag))
	{
		return false;
	}

	const UReclaimAbilityDefinition* AbilityDefinition = GetReclaimAbilityDefinition();
	const UReclaimAbilitySystemComponent* ReclaimAbilitySystem = Cast<UReclaimAbilitySystemComponent>(AbilitySystem);
	if (ReclaimAbilitySystem && AbilityDefinition)
	{
		FString FailureReason;
		if (!ReclaimAbilitySystem->IsAbilityRuntimeReady(AbilityDefinition, FailureReason))
		{
			return false;
		}
	}

	return true;
}

const UReclaimAbilityDefinition* UReclaimGameplayAbility::GetReclaimAbilityDefinition() const
{
	if (Definition)
	{
		return Definition;
	}

	return Cast<UReclaimAbilityDefinition>(GetCurrentSourceObject());
}

UReclaimAbilitySystemComponent* UReclaimGameplayAbility::GetReclaimAbilitySystemComponentFromActorInfo() const
{
	return Cast<UReclaimAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

AReclaimPlayerCharacter* UReclaimGameplayAbility::GetReclaimAvatarCharacter() const
{
	return Cast<AReclaimPlayerCharacter>(GetAvatarActorFromActorInfo());
}

bool UReclaimGameplayAbility::ConsumeActivationPayload(FReclaimAbilityActivationPayload& OutPayload) const
{
	AReclaimPlayerCharacter* Character = GetReclaimAvatarCharacter();
	return Character && Character->ConsumePendingAbilityActivationPayload(OutPayload);
}

bool UReclaimGameplayAbility::ValidateCommonPayload(const FReclaimAbilityActivationPayload& Payload, float MaxAimOriginDistance, float MaxAimAngleDegrees) const
{
	const AReclaimPlayerCharacter* Character = GetReclaimAvatarCharacter();
	const AController* Controller = Character ? Character->GetController() : nullptr;
	if (!Character || !Controller || Payload.AimOrigin.ContainsNaN() || Payload.AimDirection.ContainsNaN() || Payload.AimDirection.IsNearlyZero())
	{
		return false;
	}

	FVector ServerViewOrigin = FVector::ZeroVector;
	FRotator ServerViewRotation = FRotator::ZeroRotator;
	Controller->GetPlayerViewPoint(ServerViewOrigin, ServerViewRotation);

	if (MaxAimOriginDistance > 0.0f && FVector::DistSquared(ServerViewOrigin, Payload.AimOrigin) > FMath::Square(MaxAimOriginDistance))
	{
		return false;
	}

	const FVector NormalizedAim = FVector(Payload.AimDirection).GetSafeNormal();
	const FVector ServerForward = ServerViewRotation.Vector().GetSafeNormal();
	if (NormalizedAim.IsNearlyZero() || ServerForward.IsNearlyZero())
	{
		return false;
	}

	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(MaxAimAngleDegrees));
	return FVector::DotProduct(NormalizedAim, ServerForward) >= MinimumDot;
}

bool UReclaimGameplayAbility::CommitReclaimAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	const UReclaimAbilityDefinition* AbilityDefinition = GetReclaimAbilityDefinition();
	UReclaimAbilitySystemComponent* ReclaimAbilitySystem = GetReclaimAbilitySystemComponentFromActorInfo();
	if (!AbilityDefinition || !ReclaimAbilitySystem)
	{
		return false;
	}

	ReclaimAbilitySystem->RefreshAbilityRuntimeState_Server(AbilityDefinition);

	FString FailureReason;
	if (!ReclaimAbilitySystem->IsAbilityRuntimeReady(AbilityDefinition, FailureReason))
	{
		return false;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return false;
	}

	ReclaimAbilitySystem->CommitAbilityRuntimeState_Server(AbilityDefinition);
	return true;
}

void UReclaimGameplayAbility::ConfirmAbilityPresentation(FGameplayTag AbilityTag, FVector Location) const
{
	if (AReclaimPlayerCharacter* Character = GetReclaimAvatarCharacter())
	{
		Character->Multicast_RoleAbilityConfirmed(AbilityTag, Location);
	}
}
