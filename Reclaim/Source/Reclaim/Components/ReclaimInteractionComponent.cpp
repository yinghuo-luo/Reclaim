// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/ReclaimInteractionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Core/ReclaimCooperationRules.h"
#include "Core/ReclaimGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

UReclaimInteractionComponent::UReclaimInteractionComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UReclaimInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveTimerHandle);
		World->GetTimerManager().ClearTimer(ReviveValidationTimerHandle);
	}
	SetRevivingTag_Server(false);

	Super::EndPlay(EndPlayReason);
}

void UReclaimInteractionComponent::RequestBeginRevive(AActor* TargetActor)
{
	Server_RequestBeginRevive(TargetActor);
}

void UReclaimInteractionComponent::RequestCancelRevive()
{
	Server_RequestCancelRevive();
}

void UReclaimInteractionComponent::Server_RequestBeginRevive_Implementation(AActor* TargetActor)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	EReclaimReviveValidationResult ValidationResult = EReclaimReviveValidationResult::InvalidTarget;
	if (!ValidateReviveTarget_Server(TargetActor, ValidationResult))
	{
		ClearReviveState_Server();
		return;
	}

	PendingReviveTarget = TargetActor;
	SetRevivingTag_Server(true);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveTimerHandle);
		World->GetTimerManager().ClearTimer(ReviveValidationTimerHandle);
		World->GetTimerManager().SetTimer(ReviveValidationTimerHandle, this, &UReclaimInteractionComponent::ValidatePendingRevive_Server, FMath::Max(0.05f, ReviveValidationInterval), true);
		const float ClampedDuration = FMath::Max(0.0f, ReviveDurationSeconds);
		if (ClampedDuration <= 0.0f)
		{
			CompleteRevive_Server();
		}
		else
		{
			World->GetTimerManager().SetTimer(ReviveTimerHandle, this, &UReclaimInteractionComponent::CompleteRevive_Server, ClampedDuration, false);
		}
	}
}

void UReclaimInteractionComponent::Server_RequestCancelRevive_Implementation()
{
	CancelRevive_Server();
}

void UReclaimInteractionComponent::CancelRevive_Server()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	ClearReviveState_Server();
}

bool UReclaimInteractionComponent::ValidateReviveTarget_Server(AActor* TargetActor, EReclaimReviveValidationResult& OutResult) const
{
	OutResult = EReclaimReviveValidationResult::InvalidTarget;

	const APawn* ReviverPawn = Cast<APawn>(GetOwner());
	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	const AReclaimPlayerState* ReviverPlayerState = ReviverPawn ? ReviverPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	const AReclaimPlayerState* TargetPlayerState = TargetPawn ? TargetPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (!ReviverPawn || !TargetPawn || !ReviverPlayerState || !TargetPlayerState || TargetActor->IsPendingKillPending())
	{
		return false;
	}

	if (!TargetActor->FindComponentByClass<UReclaimHealthShieldComponent>())
	{
		return false;
	}

	const float Distance = FVector::Dist(ReviverPawn->GetActorLocation(), TargetPawn->GetActorLocation());
	OutResult = UReclaimCooperationRules::EvaluateReviveStart(
		ReviverPlayerState->GetLifeState(),
		TargetPlayerState->GetLifeState(),
		Distance,
		ReviveMaxDistance,
		ReviverPlayerState == TargetPlayerState,
		true);

	return OutResult == EReclaimReviveValidationResult::Success;
}

void UReclaimInteractionComponent::ValidatePendingRevive_Server()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	EReclaimReviveValidationResult ValidationResult = EReclaimReviveValidationResult::InvalidTarget;
	if (!ValidateReviveTarget_Server(PendingReviveTarget, ValidationResult))
	{
		ClearReviveState_Server();
	}
}

void UReclaimInteractionComponent::CompleteRevive_Server()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	EReclaimReviveValidationResult ValidationResult = EReclaimReviveValidationResult::InvalidTarget;
	AActor* TargetActor = PendingReviveTarget;
	if (!ValidateReviveTarget_Server(TargetActor, ValidationResult))
	{
		ClearReviveState_Server();
		return;
	}

	if (UReclaimHealthShieldComponent* HealthShield = TargetActor ? TargetActor->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr)
	{
		HealthShield->ReviveFromDowned_Server(OwnerActor, ReviveHealthFraction);
	}

	ClearReviveState_Server();
}

void UReclaimInteractionComponent::ClearReviveState_Server()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveTimerHandle);
		World->GetTimerManager().ClearTimer(ReviveValidationTimerHandle);
	}

	PendingReviveTarget = nullptr;
	SetRevivingTag_Server(false);
}

void UReclaimInteractionComponent::SetRevivingTag_Server(bool bReviving) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
	{
		AbilitySystem->SetLooseGameplayTagCount(ReclaimGameplayTags::State_Reviving, bReviving ? 1 : 0);
	}
}
