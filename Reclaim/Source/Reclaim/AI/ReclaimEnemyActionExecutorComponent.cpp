// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ReclaimEnemyActionExecutorComponent.h"

#include "AI/ReclaimAIConfig.h"
#include "AI/ReclaimEnemyActionRules.h"
#include "AI/ReclaimEnemyDefinition.h"
#include "Characters/ReclaimEnemyCharacter.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Core/ReclaimGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Player/ReclaimPlayerState.h"
#include "Weapons/ReclaimProjectile.h"

namespace
{
	bool IsActivePlayerTarget(AActor* TargetActor)
	{
		const APawn* TargetPawn = Cast<APawn>(TargetActor);
		const AReclaimPlayerState* TargetPlayerState = TargetPawn ? TargetPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
		return TargetPawn
			&& TargetPlayerState
			&& TargetPlayerState->GetLifeState() == EReclaimPlayerLifeState::Active
			&& !TargetPawn->IsPendingKillPending();
	}

	bool IsWithinDistance(const AActor* SourceActor, const AActor* TargetActor, float MaxDistance)
	{
		if (!SourceActor || !TargetActor)
		{
			return false;
		}

		return MaxDistance <= 0.0f || FVector::DistSquared(SourceActor->GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(MaxDistance);
	}

	bool IsBetweenDistances(const AActor* SourceActor, const AActor* TargetActor, float MinDistance, float MaxDistance)
	{
		if (!SourceActor || !TargetActor)
		{
			return false;
		}

		const float DistanceSquared = FVector::DistSquared(SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
		return DistanceSquared >= FMath::Square(FMath::Max(0.0f, MinDistance))
			&& (MaxDistance <= 0.0f || DistanceSquared <= FMath::Square(MaxDistance));
	}

	bool HasLineOfSightToTarget(AReclaimEnemyCharacter* Enemy, AActor* TargetActor)
	{
		UWorld* World = Enemy ? Enemy->GetWorld() : nullptr;
		if (!World || !Enemy || !TargetActor)
		{
			return false;
		}

		const UReclaimAIConfig* Config = Enemy->GetAIConfig();
		const ECollisionChannel VisibilityChannel = Config ? Config->LineOfSightChannel.GetValue() : ECC_Visibility;
		const FVector Start = Enemy->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
		const FVector End = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);

		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimEnemyActionLOS), true, Enemy);
		QueryParams.AddIgnoredActor(Enemy);
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, VisibilityChannel, QueryParams);
		return !bBlocked || Hit.GetActor() == TargetActor;
	}

	bool ValidateAndCommitAction(AReclaimEnemyCharacter* Enemy, AActor* TargetActor, const UReclaimEnemyDefinition* Definition, EReclaimEnemyIntentType CooldownIntent, bool bHasDamageReceiver, bool bInRange, bool bRequiresDamageReceiver, bool bRequiresLineOfSight)
	{
		const bool bEnemyCanAct = Enemy && !Enemy->IsDead();
		const bool bTargetActive = IsActivePlayerTarget(TargetActor);
		const bool bHasLineOfSight = !bRequiresLineOfSight || HasLineOfSightToTarget(Enemy, TargetActor);
		const bool bCooldownReady = Enemy && Enemy->IsActionReady(CooldownIntent);

		const EReclaimEnemyActionValidationResult ValidationResult = UReclaimEnemyActionRules::EvaluateActionCommitPreconditions(
			Enemy != nullptr,
			bEnemyCanAct,
			Definition != nullptr,
			TargetActor != nullptr,
			bTargetActive,
			bHasDamageReceiver,
			bInRange,
			bHasLineOfSight,
			bCooldownReady,
			bRequiresDamageReceiver,
			bRequiresLineOfSight);

		return ValidationResult == EReclaimEnemyActionValidationResult::Success
			&& Enemy->TryCommitActionCooldown_Server(CooldownIntent);
	}

	void ApplyDirectEnemyDamage(AReclaimEnemyCharacter* Enemy, AActor* TargetActor, float DamageAmount, const UReclaimEnemyDefinition* Definition)
	{
		UReclaimHealthShieldComponent* HealthShield = TargetActor ? TargetActor->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
		if (!Enemy || !HealthShield)
		{
			return;
		}

		FReclaimDamageApplicationResult Result;
		const FGameplayTag DamageTag = Definition && Definition->DamageTypeTag.IsValid() ? Definition->DamageTypeTag : ReclaimGameplayTags::Damage_Kinetic;
		HealthShield->ApplyDamage_Server(DamageAmount, Enemy, DamageTag, Result);
	}
}

UReclaimEnemyActionExecutorComponent::UReclaimEnemyActionExecutorComponent()
{
	SetIsReplicatedByDefault(false);
	PrimaryComponentTick.bCanEverTick = false;
}

void UReclaimEnemyActionExecutorComponent::ExecuteIntent(const FReclaimEnemyIntent& Intent)
{
	ExecuteIntentAgainstTarget(Intent, nullptr);
}

void UReclaimEnemyActionExecutorComponent::ExecuteIntentAgainstTarget(const FReclaimEnemyIntent& Intent, AActor* TargetActor)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	LastExecutedIntent = Intent;

	switch (Intent.Type)
	{
	case EReclaimEnemyIntentType::Approach:
		MoveToActor_Server(TargetActor);
		break;
	case EReclaimEnemyIntentType::Retreat:
		MoveToLocation_Server(ProjectRepositionLocation_Server(TargetActor, Intent.DesiredRange, true));
		break;
	case EReclaimEnemyIntentType::MaintainRange:
		StopMovement_Server();
		FaceTarget_Server(TargetActor);
		break;
	case EReclaimEnemyIntentType::Reposition:
	case EReclaimEnemyIntentType::StrafeLeft:
	case EReclaimEnemyIntentType::StrafeRight:
		MoveToLocation_Server(ProjectRepositionLocation_Server(TargetActor, Intent.DesiredRange, false));
		FaceTarget_Server(TargetActor);
		break;
	case EReclaimEnemyIntentType::MeleeAttack:
		StopMovement_Server();
		FaceTarget_Server(TargetActor);
		if (const AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(OwnerActor))
		{
			const UReclaimEnemyDefinition* Definition = Enemy->GetEnemyDefinition();
			ExecuteMeleeDamage_Server(TargetActor, Definition ? Definition->MeleeDamage : 0.0f, EReclaimEnemyIntentType::MeleeAttack);
		}
		break;
	case EReclaimEnemyIntentType::HeavyAttack:
		StopMovement_Server();
		FaceTarget_Server(TargetActor);
		if (const AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(OwnerActor))
		{
			const UReclaimEnemyDefinition* Definition = Enemy->GetEnemyDefinition();
			ExecuteMeleeDamage_Server(TargetActor, Definition ? Definition->HeavyMeleeDamage : 0.0f, EReclaimEnemyIntentType::HeavyAttack);
		}
		break;
	case EReclaimEnemyIntentType::RangedAttack:
		StopMovement_Server();
		FaceTarget_Server(TargetActor);
		ExecuteProjectileAttack_Server(TargetActor);
		break;
	case EReclaimEnemyIntentType::LeapAttack:
		FaceTarget_Server(TargetActor);
		ExecuteLeapAttack_Server(TargetActor);
		break;
	case EReclaimEnemyIntentType::Charge:
		FaceTarget_Server(TargetActor);
		ExecuteCharge_Server(TargetActor);
		break;
	case EReclaimEnemyIntentType::Dead:
	case EReclaimEnemyIntentType::Wait:
	case EReclaimEnemyIntentType::Hold:
	case EReclaimEnemyIntentType::UseSkill:
	default:
		StopMovement_Server();
		break;
	}
}

void UReclaimEnemyActionExecutorComponent::StopMovement_Server() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AAIController* AIController = OwnerPawn ? Cast<AAIController>(OwnerPawn->GetController()) : nullptr;
	if (AIController)
	{
		AIController->StopMovement();
	}
}

void UReclaimEnemyActionExecutorComponent::MoveToActor_Server(AActor* TargetActor) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AAIController* AIController = OwnerPawn ? Cast<AAIController>(OwnerPawn->GetController()) : nullptr;
	if (AIController && TargetActor)
	{
		AIController->MoveToActor(TargetActor, 120.0f, true, true, true, nullptr, true);
	}
}

void UReclaimEnemyActionExecutorComponent::MoveToLocation_Server(const FVector& Destination) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AAIController* AIController = OwnerPawn ? Cast<AAIController>(OwnerPawn->GetController()) : nullptr;
	if (AIController && !Destination.ContainsNaN())
	{
		AIController->MoveToLocation(Destination, 120.0f, true, true, true, true, nullptr, true);
	}
}

void UReclaimEnemyActionExecutorComponent::FaceTarget_Server(AActor* TargetActor) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !TargetActor)
	{
		return;
	}

	const FVector ToTarget = (TargetActor->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		OwnerActor->SetActorRotation(ToTarget.Rotation());
	}
}

void UReclaimEnemyActionExecutorComponent::ExecuteMeleeDamage_Server(AActor* TargetActor, float DamageAmount, EReclaimEnemyIntentType CooldownIntent) const
{
	AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(GetOwner());
	const UReclaimEnemyDefinition* Definition = Enemy ? Enemy->GetEnemyDefinition() : nullptr;
	UReclaimHealthShieldComponent* HealthShield = TargetActor ? TargetActor->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
	const float AttackDistance = Definition ? FMath::Max(0.0f, Definition->AttackDistance) : 0.0f;
	const bool bInRange = IsWithinDistance(Enemy, TargetActor, AttackDistance + 75.0f);
	if (!ValidateAndCommitAction(Enemy, TargetActor, Definition, CooldownIntent, HealthShield != nullptr, bInRange, true, false))
	{
		return;
	}

	ApplyDirectEnemyDamage(Enemy, TargetActor, DamageAmount, Definition);
}

void UReclaimEnemyActionExecutorComponent::ExecuteProjectileAttack_Server(AActor* TargetActor) const
{
	AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(GetOwner());
	const UReclaimEnemyDefinition* Definition = Enemy ? Enemy->GetEnemyDefinition() : nullptr;
	UWorld* World = GetWorld();
	UReclaimHealthShieldComponent* HealthShield = TargetActor ? TargetActor->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
	const float RangedMaxDistance = Definition ? FMath::Max(Definition->PreferredDistance * 1.5f, Definition->AttackDistance) : 0.0f;
	const FVector SpawnLocation = Enemy ? Enemy->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f) : FVector::ZeroVector;
	const FVector AimDirection = TargetActor
		? (TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f) - SpawnLocation).GetSafeNormal()
		: FVector::ZeroVector;

	// Complete execution-specific validation before consuming the cooldown.
	if (!World || AimDirection.IsNearlyZero()
		|| !ValidateAndCommitAction(Enemy, TargetActor, Definition, EReclaimEnemyIntentType::RangedAttack, HealthShield != nullptr, IsWithinDistance(Enemy, TargetActor, RangedMaxDistance), true, true))
	{
		return;
	}

	TSubclassOf<AReclaimProjectile> ProjectileClass = Definition->ProjectileClass;
	if (!ProjectileClass)
	{
		ProjectileClass = AReclaimProjectile::StaticClass();
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Enemy;
	SpawnParameters.Instigator = Enemy;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AReclaimProjectile* Projectile = World->SpawnActor<AReclaimProjectile>(ProjectileClass, SpawnLocation, AimDirection.Rotation(), SpawnParameters);
	if (Projectile)
	{
		const FGameplayTag DamageTag = Definition->DamageTypeTag.IsValid() ? Definition->DamageTypeTag : ReclaimGameplayTags::Damage_Kinetic;
		Projectile->InitializeDamageProjectile_Server(Definition->ProjectileDamage, DamageTag, Enemy, Definition->ProjectileSpeed, Definition->ProjectileLifetimeSeconds);
	}
}

void UReclaimEnemyActionExecutorComponent::ExecuteLeapAttack_Server(AActor* TargetActor) const
{
	AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(GetOwner());
	const UReclaimEnemyDefinition* Definition = Enemy ? Enemy->GetEnemyDefinition() : nullptr;
	UReclaimHealthShieldComponent* HealthShield = TargetActor ? TargetActor->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
	const bool bInRange = Definition && IsBetweenDistances(Enemy, TargetActor, Definition->LeapMinDistance, Definition->LeapMaxDistance);
	const FVector Direction = Enemy && TargetActor
		? (TargetActor->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D()
		: FVector::ZeroVector;

	if (Direction.IsNearlyZero()
		|| !ValidateAndCommitAction(Enemy, TargetActor, Definition, EReclaimEnemyIntentType::LeapAttack, HealthShield != nullptr, bInRange, true, true))
	{
		return;
	}

	Enemy->LaunchCharacter(Direction * Definition->LeapImpulse + FVector(0.0f, 0.0f, 180.0f), true, true);
	ApplyDirectEnemyDamage(Enemy, TargetActor, Definition->MeleeDamage, Definition);
}

void UReclaimEnemyActionExecutorComponent::ExecuteCharge_Server(AActor* TargetActor) const
{
	AReclaimEnemyCharacter* Enemy = Cast<AReclaimEnemyCharacter>(GetOwner());
	const UReclaimEnemyDefinition* Definition = Enemy ? Enemy->GetEnemyDefinition() : nullptr;
	UReclaimHealthShieldComponent* HealthShield = TargetActor ? TargetActor->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
	const bool bInRange = Definition && IsBetweenDistances(Enemy, TargetActor, Definition->ChargeMinDistance, Definition->ChargeMaxDistance);
	const FVector Direction = Enemy && TargetActor
		? (TargetActor->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D()
		: FVector::ZeroVector;

	if (Direction.IsNearlyZero()
		|| !ValidateAndCommitAction(Enemy, TargetActor, Definition, EReclaimEnemyIntentType::Charge, HealthShield != nullptr, bInRange, true, true))
	{
		return;
	}

	Enemy->LaunchCharacter(Direction * Definition->ChargeImpulse, true, true);
	ApplyDirectEnemyDamage(Enemy, TargetActor, Definition->HeavyMeleeDamage, Definition);
}

FVector UReclaimEnemyActionExecutorComponent::ProjectRepositionLocation_Server(AActor* TargetActor, float DesiredDistance, bool bRetreat) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !TargetActor)
	{
		return OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
	}

	FVector AwayFromTarget = (OwnerActor->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal2D();
	if (AwayFromTarget.IsNearlyZero())
	{
		AwayFromTarget = OwnerActor->GetActorRightVector().GetSafeNormal2D();
	}

	const FVector Strafe = FVector::CrossProduct(FVector::UpVector, AwayFromTarget).GetSafeNormal();
	const float Distance = FMath::Max(250.0f, DesiredDistance);
	const FVector RawDestination = bRetreat
		? OwnerActor->GetActorLocation() + AwayFromTarget * Distance
		: TargetActor->GetActorLocation() + AwayFromTarget * Distance + Strafe * 300.0f;

	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (NavigationSystem)
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(RawDestination, ProjectedLocation, FVector(300.0f, 300.0f, 300.0f)))
		{
			return ProjectedLocation.Location;
		}
	}

	return RawDestination;
}
