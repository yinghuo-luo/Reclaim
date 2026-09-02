// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/ReclaimProjectile.h"

#include "AbilitySystem/ReclaimKineticBarrierActor.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Core/ReclaimGameplayTags.h"
#include "GameFramework/ProjectileMovementComponent.h"

AReclaimProjectile::AReclaimProjectile()
{
	bReplicates = true;
	SetReplicateMovement(true);
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(12.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = 1400.0f;
	ProjectileMovementComponent->MaxSpeed = 1400.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	PrimaryActorTick.bCanEverTick = false;
}

void AReclaimProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &AReclaimProjectile::HandleProjectileHit);
	}
}

void AReclaimProjectile::InitializeDamageProjectile_Server(float InDamageAmount, FGameplayTag InDamageTypeTag, AActor* InIgnoredActor, float InitialSpeed, float LifeSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	DamageAmount = FMath::Max(0.0f, InDamageAmount);
	DamageTypeTag = InDamageTypeTag.IsValid() ? InDamageTypeTag : ReclaimGameplayTags::Damage_Kinetic;
	IgnoredActor = InIgnoredActor;

	if (CollisionComponent)
	{
		if (InIgnoredActor)
		{
			CollisionComponent->IgnoreActorWhenMoving(InIgnoredActor, true);
		}

		if (const AReclaimPlayerCharacter* SourceCharacter = Cast<AReclaimPlayerCharacter>(InIgnoredActor))
		{
			if (AReclaimKineticBarrierActor* OwnedBarrier = SourceCharacter->GetActiveKineticBarrierActor())
			{
				CollisionComponent->IgnoreActorWhenMoving(OwnedBarrier, true);
			}
		}
	}

	if (ProjectileMovementComponent)
	{
		const float ClampedSpeed = FMath::Max(1.0f, InitialSpeed);
		ProjectileMovementComponent->InitialSpeed = ClampedSpeed;
		ProjectileMovementComponent->MaxSpeed = ClampedSpeed;
		ProjectileMovementComponent->Velocity = GetActorForwardVector().GetSafeNormal() * ClampedSpeed;
	}

	if (LifeSeconds > 0.0f)
	{
		SetLifeSpan(LifeSeconds);
	}
}

void AReclaimProjectile::HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!OtherActor || OtherActor == this || OtherActor == IgnoredActor || OtherActor == GetOwner())
	{
		return;
	}

	if (AReclaimKineticBarrierActor* Barrier = Cast<AReclaimKineticBarrierActor>(OtherActor))
	{
		float RemainingDamage = DamageAmount;
		if (Barrier->TryAbsorbDamage_Server(this, DamageTypeTag, DamageAmount, RemainingDamage))
		{
			if (RemainingDamage > 0.0f)
			{
				if (AActor* ProtectedActor = Barrier->GetOwner())
				{
					if (UReclaimHealthShieldComponent* ProtectedHealthShield = ProtectedActor->FindComponentByClass<UReclaimHealthShieldComponent>())
					{
						FReclaimDamageApplicationResult Result;
						ProtectedHealthShield->ApplyDamage_Server(RemainingDamage, this, DamageTypeTag, Result);
					}
				}
			}

			if (bDestroyOnImpact)
			{
				Destroy();
			}
			return;
		}
	}

	if (DamageAmount > 0.0f)
	{
		if (UReclaimHealthShieldComponent* HealthShield = OtherActor->FindComponentByClass<UReclaimHealthShieldComponent>())
		{
			FReclaimDamageApplicationResult Result;
			HealthShield->ApplyDamage_Server(DamageAmount, this, DamageTypeTag, Result);
		}
	}

	if (bDestroyOnImpact)
	{
		Destroy();
	}
}
