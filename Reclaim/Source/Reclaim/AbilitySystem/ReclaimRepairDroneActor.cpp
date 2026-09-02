// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ReclaimRepairDroneActor.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Interfaces/ReclaimTargetContracts.h"
#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

AReclaimRepairDroneActor::AReclaimRepairDroneActor()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AReclaimRepairDroneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepairTickTimerHandle);
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AReclaimRepairDroneActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimRepairDroneActor, SourceCharacter);
	DOREPLIFETIME(AReclaimRepairDroneActor, TargetActor);
	DOREPLIFETIME(AReclaimRepairDroneActor, EndServerTime);
}

bool AReclaimRepairDroneActor::InitializeRepairDrone_Server(AReclaimPlayerCharacter* NewSourceCharacter, AActor* NewTargetActor, const UReclaimAbilityDefinition* AbilityDefinition)
{
	if (!HasAuthority() || !NewSourceCharacter || !AbilityDefinition)
	{
		return false;
	}

	SourceCharacter = NewSourceCharacter;
	TargetActor = NewTargetActor ? NewTargetActor : NewSourceCharacter;
	SetOwner(NewSourceCharacter);

	MaxRepairRange = AbilityDefinition->Range > 0.0f ? AbilityDefinition->Range : DefaultMaxRange;
	HealthRepairPerTick = AbilityDefinition->Value > 0.0f ? AbilityDefinition->Value : DefaultHealthRepairPerTick;
	ShieldRepairPerTick = AbilityDefinition->SecondaryValue > 0.0f ? AbilityDefinition->SecondaryValue : DefaultShieldRepairPerTick;

	if (!IsRepairStillValid_Server())
	{
		return false;
	}

	AttachToActor(TargetActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector(0.0f, 0.0f, 120.0f));

	UWorld* World = GetWorld();
	const float Duration = AbilityDefinition->Duration > 0.0f ? AbilityDefinition->Duration : DefaultDurationSeconds;
	const float TickInterval = AbilityDefinition->TickInterval > 0.0f ? AbilityDefinition->TickInterval : DefaultTickInterval;
	EndServerTime = World ? World->GetTimeSeconds() + Duration : Duration;

	if (World)
	{
		World->GetTimerManager().SetTimer(RepairTickTimerHandle, this, &AReclaimRepairDroneActor::ApplyRepairTick_Server, FMath::Max(0.05f, TickInterval), true, 0.0f);
		if (Duration > 0.0f)
		{
			World->GetTimerManager().SetTimer(DurationTimerHandle, this, &AReclaimRepairDroneActor::EndRepairDrone_Server, Duration, false);
		}
	}

	return true;
}

void AReclaimRepairDroneActor::EndRepairDrone_Server()
{
	if (HasAuthority())
	{
		Destroy();
	}
}

bool AReclaimRepairDroneActor::IsRepairStillValid_Server() const
{
	if (!HasAuthority() || !SourceCharacter || !TargetActor || TargetActor->IsPendingKillPending())
	{
		return false;
	}

	const AReclaimPlayerState* SourcePlayerState = SourceCharacter->GetReclaimPlayerState();
	if (!SourcePlayerState || SourcePlayerState->GetLifeState() != EReclaimPlayerLifeState::Active)
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UReclaimRepairableInterface::StaticClass()))
	{
		return false;
	}

	if (MaxRepairRange > 0.0f && FVector::DistSquared(SourceCharacter->GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(MaxRepairRange))
	{
		return false;
	}

	return IReclaimRepairableInterface::Execute_CanReceiveRepairFrom(TargetActor, SourceCharacter);
}

void AReclaimRepairDroneActor::ApplyRepairTick_Server()
{
	if (!IsRepairStillValid_Server())
	{
		EndRepairDrone_Server();
		return;
	}

	IReclaimRepairableInterface::Execute_ApplyRepair_Server(TargetActor, SourceCharacter, HealthRepairPerTick, ShieldRepairPerTick);
}
