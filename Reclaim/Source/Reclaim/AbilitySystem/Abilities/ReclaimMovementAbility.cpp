// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/ReclaimMovementAbility.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "Interfaces/ReclaimTargetContracts.h"

namespace
{
	bool IsTargetIntentWithinMovementRange(const AReclaimPlayerCharacter* Character, const FReclaimAbilityActivationPayload& Payload, const FVector& Direction, float Range)
	{
		if (!Character || !Payload.bHasTargetLocation || Range <= 0.0f)
		{
			return true;
		}

		const FVector ToTarget = FVector(Payload.TargetLocation) - Character->GetActorLocation();
		const FVector HorizontalToTarget = FVector(ToTarget.X, ToTarget.Y, 0.0f);
		if (HorizontalToTarget.SizeSquared() > FMath::Square(Range + 100.0f))
		{
			return false;
		}

		const FVector TargetDirection = HorizontalToTarget.GetSafeNormal();
		return TargetDirection.IsNearlyZero() || FVector::DotProduct(TargetDirection, Direction) >= 0.35f;
	}

	bool IsMovementSweepFeasible_Server(AReclaimPlayerCharacter* Character, const FVector& Direction, float Range)
	{
		UWorld* World = Character ? Character->GetWorld() : nullptr;
		UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (!World || !Character || !Capsule || Range <= 0.0f)
		{
			return false;
		}

		const FVector Start = Character->GetActorLocation();
		const FVector End = Start + Direction * Range;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimMovementAbilitySweep), false, Character);
		QueryParams.AddIgnoredActor(Character);

		FHitResult BlockingHit;
		const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(
			FMath::Max(1.0f, Capsule->GetScaledCapsuleRadius() * 0.9f),
			FMath::Max(1.0f, Capsule->GetScaledCapsuleHalfHeight() * 0.9f));
		return !World->SweepSingleByChannel(BlockingHit, Start, End, FQuat::Identity, ECC_WorldStatic, CollisionShape, QueryParams);
	}
}

void UReclaimMovementAbility::ActivateAbility(
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
	if (!ConsumeActivationPayload(Payload) || !ValidateCommonPayload(Payload, 250.0f, AbilityDefinition->MaxAngleDegrees))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector Direction = FVector(Payload.AimDirection).GetSafeNormal2D();
	const float Range = FMath::Max(0.0f, AbilityDefinition->Range);
	if (Direction.IsNearlyZero() || Range <= 0.0f || !IsTargetIntentWithinMovementRange(Character, Payload, Direction, Range) || !IsMovementSweepFeasible_Server(Character, Direction, Range))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitReclaimAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float Duration = FMath::Max(0.05f, AbilityDefinition->Duration);
	const float Impulse = Range / Duration;
	Character->LaunchCharacter(Direction * Impulse, true, true);

	if (AbilityDefinition->MovementExecutionMode == EReclaimMovementExecutionMode::Dash)
	{
		UWorld* World = Character->GetWorld();
		if (World)
		{
			const FVector Start = Character->GetActorLocation();
			const FVector End = Start + Direction * Range;
			const float Radius = AbilityDefinition->Radius > 0.0f ? AbilityDefinition->Radius : 160.0f;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimDashPush), true, Character);
			QueryParams.AddIgnoredActor(Character);

			TArray<FHitResult> Hits;
			World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), QueryParams);
			for (const FHitResult& Hit : Hits)
			{
				AActor* HitActor = Hit.GetActor();
				if (!HitActor || !HitActor->GetClass()->ImplementsInterface(UReclaimPushableInterface::StaticClass()))
				{
					continue;
				}

				if (IReclaimPushableInterface::Execute_CanBePushedBy(HitActor, Character, AbilityDefinition->AbilityTags))
				{
					const float Strength = AbilityDefinition->KnockbackStrength > 0.0f ? AbilityDefinition->KnockbackStrength : 900.0f;
					IReclaimPushableInterface::Execute_ApplyPush_Server(HitActor, Character, Direction, Strength, FMath::Max(0.2f, AbilityDefinition->Duration));
				}
			}
		}
	}

	ConfirmAbilityPresentation(AbilityDefinition->GetPrimaryAbilityTag(), Character->GetActorLocation());
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
