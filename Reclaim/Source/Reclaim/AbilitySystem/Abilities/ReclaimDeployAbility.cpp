// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/ReclaimDeployAbility.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "AbilitySystem/ReclaimKineticBarrierActor.h"
#include "AbilitySystem/ReclaimRepairDroneActor.h"
#include "AbilitySystem/ReclaimStabilityFieldActor.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Interfaces/ReclaimTargetContracts.h"
#include "Player/ReclaimPlayerState.h"

namespace
{
	bool IsActiveSourceCharacter(const AReclaimPlayerCharacter* Character)
	{
		const AReclaimPlayerState* PlayerState = Character ? Character->GetReclaimPlayerState() : nullptr;
		return Character && Character->HasAuthority() && PlayerState && PlayerState->GetLifeState() == EReclaimPlayerLifeState::Active;
	}

	AActor* ResolveRepairTarget(AReclaimPlayerCharacter* Character, const FReclaimAbilityActivationPayload& Payload)
	{
		if (Payload.TargetActor)
		{
			return Payload.TargetActor.Get();
		}

		return Character;
	}

	bool IsRepairTargetValid(AReclaimPlayerCharacter* Character, AActor* RepairTarget, const UReclaimAbilityDefinition* AbilityDefinition)
	{
		if (!IsActiveSourceCharacter(Character) || !RepairTarget || RepairTarget->IsPendingKillPending() || !AbilityDefinition)
		{
			return false;
		}

		if (!RepairTarget->GetClass()->ImplementsInterface(UReclaimRepairableInterface::StaticClass()))
		{
			return false;
		}

		const float Range = AbilityDefinition->Range;
		if (Range > 0.0f && FVector::DistSquared(Character->GetActorLocation(), RepairTarget->GetActorLocation()) > FMath::Square(Range))
		{
			return false;
		}

		return IReclaimRepairableInterface::Execute_CanReceiveRepairFrom(RepairTarget, Character);
	}

	template <typename TActor>
	TSubclassOf<TActor> ResolveConfiguredActorClass(TSubclassOf<AActor> ConfiguredClass)
	{
		if (UClass* RawClass = ConfiguredClass.Get())
		{
			if (RawClass->IsChildOf(TActor::StaticClass()))
			{
				return RawClass;
			}
		}
		return TActor::StaticClass();
	}
}

void UReclaimDeployAbility::ActivateAbility(
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
	UWorld* World = Character->GetWorld();
	if (!World || !IsActiveSourceCharacter(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bool bValidationPassed = false;
	switch (AbilityDefinition->DeployExecutionMode)
	{
	case EReclaimDeployExecutionMode::KineticBarrier:
		bValidationPassed = true;
		break;
	case EReclaimDeployExecutionMode::RepairDrone:
	{
		AActor* RepairTarget = ResolveRepairTarget(Character, Payload);
		bValidationPassed = IsRepairTargetValid(Character, RepairTarget, AbilityDefinition);
		break;
	}
	case EReclaimDeployExecutionMode::StabilityField:
		bValidationPassed = true;
		break;
	case EReclaimDeployExecutionMode::None:
	default:
		break;
	}

	if (!bValidationPassed || !CommitReclaimAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	switch (AbilityDefinition->DeployExecutionMode)
	{
	case EReclaimDeployExecutionMode::KineticBarrier:
	{
		TSubclassOf<AReclaimKineticBarrierActor> BarrierClass = ResolveConfiguredActorClass<AReclaimKineticBarrierActor>(AbilityDefinition->ProjectileOrDeployableClass);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AReclaimKineticBarrierActor* Barrier = World->SpawnActor<AReclaimKineticBarrierActor>(BarrierClass, Character->GetActorLocation(), Character->GetActorRotation(), SpawnParameters))
		{
			if (!Barrier->InitializeBarrier_Server(Character, AbilityDefinition))
			{
				Barrier->Destroy();
			}
		}
		break;
	}
	case EReclaimDeployExecutionMode::RepairDrone:
	{
		AActor* RepairTarget = ResolveRepairTarget(Character, Payload);
		TSubclassOf<AReclaimRepairDroneActor> RepairDroneClass = ResolveConfiguredActorClass<AReclaimRepairDroneActor>(AbilityDefinition->ProjectileOrDeployableClass);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AReclaimRepairDroneActor* RepairDrone = World->SpawnActor<AReclaimRepairDroneActor>(RepairDroneClass, Character->GetActorLocation(), Character->GetActorRotation(), SpawnParameters))
		{
			if (!RepairDrone->InitializeRepairDrone_Server(Character, RepairTarget, AbilityDefinition))
			{
				RepairDrone->Destroy();
			}
		}
		break;
	}
	case EReclaimDeployExecutionMode::StabilityField:
	{
		TSubclassOf<AReclaimStabilityFieldActor> StabilityFieldClass = ResolveConfiguredActorClass<AReclaimStabilityFieldActor>(AbilityDefinition->ProjectileOrDeployableClass);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AReclaimStabilityFieldActor* StabilityField = World->SpawnActor<AReclaimStabilityFieldActor>(StabilityFieldClass, Character->GetActorLocation(), Character->GetActorRotation(), SpawnParameters))
		{
			if (!StabilityField->InitializeStabilityField_Server(Character, AbilityDefinition))
			{
				StabilityField->Destroy();
			}
		}
		break;
	}
	case EReclaimDeployExecutionMode::None:
	default:
		break;
	}

	ConfirmAbilityPresentation(AbilityDefinition->GetPrimaryAbilityTag(), Character->GetActorLocation());
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
