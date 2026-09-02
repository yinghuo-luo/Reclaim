// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ReclaimAbilitySystemComponent.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "Net/UnrealNetwork.h"

UReclaimAbilitySystemComponent::UReclaimAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	ReplicationMode = EGameplayEffectReplicationMode::Mixed;
}

void UReclaimAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimAbilitySystemComponent, AbilityRuntimeStates, COND_OwnerOnly, REPNOTIFY_Always);
}

bool UReclaimAbilitySystemComponent::IsAbilityRuntimeReady(const UReclaimAbilityDefinition* Definition, FString& OutFailureReason) const
{
	return IsRuntimeStateReadyAtTime(Definition, Definition ? FindRuntimeState(Definition->GetPrimaryAbilityTag()) : nullptr, GetServerTimeSeconds(), OutFailureReason);
}

bool UReclaimAbilitySystemComponent::IsRuntimeStateReadyAtTime(const UReclaimAbilityDefinition* Definition, const FReclaimAbilityRuntimeState* RuntimeState, float CurrentTime, FString& OutFailureReason)
{
	OutFailureReason.Reset();
	if (!Definition)
	{
		OutFailureReason = TEXT("missing definition");
		return false;
	}

	const FGameplayTag AbilityTag = Definition->GetPrimaryAbilityTag();
	if (!AbilityTag.IsValid())
	{
		OutFailureReason = TEXT("missing ability tag");
		return false;
	}
	if (RuntimeState)
	{
		const bool bCooldownActive = RuntimeState->CooldownEndServerTime > CurrentTime + UE_KINDA_SMALL_NUMBER;
		const bool bChargeWouldRecover = RuntimeState->ChargeRecoveryEndServerTime > 0.0f && RuntimeState->ChargeRecoveryEndServerTime <= CurrentTime + UE_KINDA_SMALL_NUMBER;
		if (bCooldownActive)
		{
			OutFailureReason = TEXT("cooldown");
			return false;
		}

		if (Definition->MaxCharges > 0 && RuntimeState->ChargesRemaining <= 0 && !bChargeWouldRecover)
		{
			OutFailureReason = TEXT("charges");
			return false;
		}
	}

	return true;
}

void UReclaimAbilitySystemComponent::RefreshRuntimeStateAtTime(const UReclaimAbilityDefinition* Definition, FReclaimAbilityRuntimeState& RuntimeState, float CurrentTime)
{
	if (!Definition)
	{
		return;
	}

	const int32 MaxCharges = FMath::Max(1, Definition->MaxCharges);
	if (RuntimeState.ChargesRemaining < MaxCharges && RuntimeState.ChargeRecoveryEndServerTime > 0.0f && RuntimeState.ChargeRecoveryEndServerTime <= CurrentTime + UE_KINDA_SMALL_NUMBER)
	{
		RuntimeState.ChargesRemaining = MaxCharges;
		RuntimeState.ChargeRecoveryEndServerTime = 0.0f;
	}
}

void UReclaimAbilitySystemComponent::CommitRuntimeStateAtTime(const UReclaimAbilityDefinition* Definition, FReclaimAbilityRuntimeState& RuntimeState, float CurrentTime)
{
	if (!Definition)
	{
		return;
	}

	RefreshRuntimeStateAtTime(Definition, RuntimeState, CurrentTime);

	const int32 MaxCharges = FMath::Max(1, Definition->MaxCharges);
	RuntimeState.ChargesRemaining = FMath::Clamp(RuntimeState.ChargesRemaining - 1, 0, MaxCharges);
	RuntimeState.CooldownEndServerTime = CurrentTime + FMath::Max(0.0f, Definition->CooldownSeconds);

	const float RecoverySeconds = Definition->ChargeRecoverySeconds > 0.0f ? Definition->ChargeRecoverySeconds : Definition->CooldownSeconds;
	RuntimeState.ChargeRecoveryEndServerTime = RuntimeState.ChargesRemaining < MaxCharges ? CurrentTime + FMath::Max(0.0f, RecoverySeconds) : 0.0f;
}

void UReclaimAbilitySystemComponent::RefreshAbilityRuntimeState_Server(const UReclaimAbilityDefinition* Definition)
{
	AActor* ComponentOwner = GetOwner();
	if (!ComponentOwner || !ComponentOwner->HasAuthority() || !Definition)
	{
		return;
	}

	FReclaimAbilityRuntimeState& RuntimeState = FindOrAddRuntimeState_Server(Definition);
	RefreshRuntimeStateAtTime(Definition, RuntimeState, GetServerTimeSeconds());
}

void UReclaimAbilitySystemComponent::CommitAbilityRuntimeState_Server(const UReclaimAbilityDefinition* Definition)
{
	AActor* ComponentOwner = GetOwner();
	if (!ComponentOwner || !ComponentOwner->HasAuthority() || !Definition)
	{
		return;
	}

	RefreshAbilityRuntimeState_Server(Definition);

	FReclaimAbilityRuntimeState& RuntimeState = FindOrAddRuntimeState_Server(Definition);
	CommitRuntimeStateAtTime(Definition, RuntimeState, GetServerTimeSeconds());
}

float UReclaimAbilitySystemComponent::GetAbilityCooldownRemaining(FGameplayTag AbilityTag) const
{
	const FReclaimAbilityRuntimeState* RuntimeState = FindRuntimeState(AbilityTag);
	if (!RuntimeState)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, RuntimeState->CooldownEndServerTime - GetServerTimeSeconds());
}

int32 UReclaimAbilitySystemComponent::GetAbilityChargesRemaining(FGameplayTag AbilityTag) const
{
	const FReclaimAbilityRuntimeState* RuntimeState = FindRuntimeState(AbilityTag);
	return RuntimeState ? RuntimeState->ChargesRemaining : 1;
}

void UReclaimAbilitySystemComponent::OnRep_AbilityRuntimeStates()
{
}

FReclaimAbilityRuntimeState* UReclaimAbilitySystemComponent::FindMutableRuntimeState(FGameplayTag AbilityTag)
{
	return AbilityRuntimeStates.FindByPredicate([AbilityTag](const FReclaimAbilityRuntimeState& Candidate)
	{
		return Candidate.AbilityTag == AbilityTag;
	});
}

const FReclaimAbilityRuntimeState* UReclaimAbilitySystemComponent::FindRuntimeState(FGameplayTag AbilityTag) const
{
	return AbilityRuntimeStates.FindByPredicate([AbilityTag](const FReclaimAbilityRuntimeState& Candidate)
	{
		return Candidate.AbilityTag == AbilityTag;
	});
}

FReclaimAbilityRuntimeState& UReclaimAbilitySystemComponent::FindOrAddRuntimeState_Server(const UReclaimAbilityDefinition* Definition)
{
	check(Definition);
	const FGameplayTag AbilityTag = Definition->GetPrimaryAbilityTag();
	check(AbilityTag.IsValid());

	if (FReclaimAbilityRuntimeState* ExistingState = FindMutableRuntimeState(AbilityTag))
	{
		return *ExistingState;
	}

	FReclaimAbilityRuntimeState& NewState = AbilityRuntimeStates.AddDefaulted_GetRef();
	NewState.AbilityTag = AbilityTag;
	NewState.ChargesRemaining = FMath::Max(1, Definition->MaxCharges);
	return NewState;
}

float UReclaimAbilitySystemComponent::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0f;
}
