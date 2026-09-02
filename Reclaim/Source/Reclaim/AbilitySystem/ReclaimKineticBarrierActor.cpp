// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ReclaimKineticBarrierActor.h"

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

AReclaimKineticBarrierActor::AReclaimKineticBarrierActor()
{
	bReplicates = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = false;

	BarrierCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BarrierCollision"));
	SetRootComponent(BarrierCollision);
	BarrierCollision->SetBoxExtent(CollisionExtent);
	BarrierCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BarrierCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BarrierCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BarrierCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	// Reclaim projectiles currently use ECC_WorldDynamic. Blocking that channel makes
	// the server-owned barrier a real interception surface instead of a damage-only tag.
	BarrierCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
}

void AReclaimKineticBarrierActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	if (HasAuthority() && OwnerCharacter)
	{
		OwnerCharacter->UnregisterKineticBarrier_Server(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AReclaimKineticBarrierActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimKineticBarrierActor, OwnerCharacter);
	DOREPLIFETIME(AReclaimKineticBarrierActor, MaxAbsorption);
	DOREPLIFETIME(AReclaimKineticBarrierActor, RemainingAbsorption);
	DOREPLIFETIME(AReclaimKineticBarrierActor, EndServerTime);
}

bool AReclaimKineticBarrierActor::InitializeBarrier_Server(AReclaimPlayerCharacter* NewOwnerCharacter, const UReclaimAbilityDefinition* AbilityDefinition)
{
	if (!HasAuthority() || !NewOwnerCharacter || !AbilityDefinition)
	{
		return false;
	}

	const AReclaimPlayerState* ReclaimPlayerState = NewOwnerCharacter->GetReclaimPlayerState();
	if (!ReclaimPlayerState || ReclaimPlayerState->GetLifeState() != EReclaimPlayerLifeState::Active)
	{
		return false;
	}

	OwnerCharacter = NewOwnerCharacter;
	SetOwner(NewOwnerCharacter);
	AttachToActor(NewOwnerCharacter, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector(ForwardOffset, 0.0f, 0.0f));
	SetActorRelativeRotation(FRotator::ZeroRotator);

	if (BarrierCollision)
	{
		const float Radius = AbilityDefinition->Radius > 0.0f ? AbilityDefinition->Radius : CollisionExtent.Y;
		BarrierCollision->SetBoxExtent(FVector(CollisionExtent.X, Radius, CollisionExtent.Z));
		BarrierCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BarrierCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		BarrierCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	}

	MaxAbsorption = AbilityDefinition->Value > 0.0f ? AbilityDefinition->Value : DefaultAbsorption;
	RemainingAbsorption = MaxAbsorption;

	UWorld* World = GetWorld();
	const float Duration = AbilityDefinition->Duration > 0.0f ? AbilityDefinition->Duration : DefaultDurationSeconds;
	EndServerTime = World ? World->GetTimeSeconds() + Duration : Duration;
	NewOwnerCharacter->RegisterKineticBarrier_Server(this);

	if (World && Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(DurationTimerHandle, this, &AReclaimKineticBarrierActor::EndBarrier_Server, Duration, false);
	}

	return true;
}

bool AReclaimKineticBarrierActor::TryAbsorbDamage_Server(AActor* DamageInstigator, FGameplayTag DamageTypeTag, float IncomingDamage, float& OutRemainingDamage)
{
	OutRemainingDamage = FMath::Max(0.0f, IncomingDamage);
	if (!HasAuthority() || !OwnerCharacter || RemainingAbsorption <= 0.0f || OutRemainingDamage <= 0.0f || !IsDamageFromProtectedArc(DamageInstigator))
	{
		return false;
	}

	const float AbsorbedDamage = FMath::Min(RemainingAbsorption, OutRemainingDamage);
	RemainingAbsorption = FMath::Max(0.0f, RemainingAbsorption - AbsorbedDamage);
	OutRemainingDamage = FMath::Max(0.0f, OutRemainingDamage - AbsorbedDamage);

	if (RemainingAbsorption <= 0.0f)
	{
		EndBarrier_Server();
	}

	return AbsorbedDamage > 0.0f;
}

void AReclaimKineticBarrierActor::EndBarrier_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	if (OwnerCharacter)
	{
		OwnerCharacter->UnregisterKineticBarrier_Server(this);
	}

	Destroy();
}

bool AReclaimKineticBarrierActor::IsDamageFromProtectedArc(AActor* DamageInstigator) const
{
	if (!OwnerCharacter || !DamageInstigator)
	{
		return false;
	}

	const FVector DirectionToSource = (DamageInstigator->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
	const FVector OwnerForward = OwnerCharacter->GetActorForwardVector().GetSafeNormal();
	return !DirectionToSource.IsNearlyZero()
		&& !OwnerForward.IsNearlyZero()
		&& FVector::DotProduct(OwnerForward, DirectionToSource) >= ProtectedArcMinimumDot;
}
