// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/ReclaimRunInventoryComponent.h"

#include "Net/UnrealNetwork.h"

UReclaimRunInventoryComponent::UReclaimRunInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

void UReclaimRunInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UReclaimRunInventoryComponent, ActiveModifierIds, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UReclaimRunInventoryComponent, PendingRewardCandidates, COND_OwnerOnly);
}

void UReclaimRunInventoryComponent::AddModifier_Server(FName ModifierId)
{
	if (GetOwner() && GetOwner()->HasAuthority() && !ModifierId.IsNone())
	{
		ActiveModifierIds.AddUnique(ModifierId);
	}
}

void UReclaimRunInventoryComponent::SetPendingRewardCandidates_Server(const TArray<FReclaimRewardCandidate>& Candidates)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		PendingRewardCandidates = Candidates;
	}
}
