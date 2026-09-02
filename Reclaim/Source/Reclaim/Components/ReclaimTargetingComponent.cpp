// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/ReclaimTargetingComponent.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UReclaimTargetingComponent::UReclaimTargetingComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UReclaimTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MarkTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UReclaimTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UReclaimTargetingComponent, TargetTags);
	DOREPLIFETIME(UReclaimTargetingComponent, bMarked);
	DOREPLIFETIME(UReclaimTargetingComponent, MarkTag);
	DOREPLIFETIME(UReclaimTargetingComponent, MarkEndServerTime);
	DOREPLIFETIME(UReclaimTargetingComponent, MarkSourceActor);
}

void UReclaimTargetingComponent::SetTargetTags_Server(const FGameplayTagContainer& NewTargetTags)
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->HasAuthority())
	{
		TargetTags = NewTargetTags;
	}
}

void UReclaimTargetingComponent::ApplyMark_Server(AActor* SourceActor, float Duration, FGameplayTag InMarkTag)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !World)
	{
		return;
	}

	bMarked = true;
	MarkTag = InMarkTag;
	MarkSourceActor = SourceActor;
	MarkEndServerTime = World->GetTimeSeconds() + FMath::Max(0.0f, Duration);

	World->GetTimerManager().ClearTimer(MarkTimerHandle);
	if (Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(MarkTimerHandle, this, &UReclaimTargetingComponent::ClearMark_Server, Duration, false);
	}

	OnMarkStateChangedDelegate.Broadcast();
	OnMarkStateChanged();
}

void UReclaimTargetingComponent::ClearMark_Server()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MarkTimerHandle);
	}

	bMarked = false;
	MarkTag = FGameplayTag();
	MarkSourceActor = nullptr;
	MarkEndServerTime = 0.0f;
	OnMarkStateChangedDelegate.Broadcast();
	OnMarkStateChanged();
}

void UReclaimTargetingComponent::OnRep_Marked()
{
	OnMarkStateChangedDelegate.Broadcast();
	OnMarkStateChanged();
}

void UReclaimTargetingComponent::OnMarkStateChanged_Implementation()
{
}
