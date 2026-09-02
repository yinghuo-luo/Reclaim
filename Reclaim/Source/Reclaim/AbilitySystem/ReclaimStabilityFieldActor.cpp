// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ReclaimStabilityFieldActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Core/ReclaimGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

AReclaimStabilityFieldActor::AReclaimStabilityFieldActor()
{
	bReplicates = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = false;

	FieldCollision = CreateDefaultSubobject<USphereComponent>(TEXT("FieldCollision"));
	SetRootComponent(FieldCollision);
	FieldCollision->SetSphereRadius(DefaultRadius);
	FieldCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FieldCollision->SetCollisionObjectType(ECC_WorldDynamic);
	FieldCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	FieldCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	FieldCollision->OnComponentBeginOverlap.AddDynamic(this, &AReclaimStabilityFieldActor::OnFieldBeginOverlap);
	FieldCollision->OnComponentEndOverlap.AddDynamic(this, &AReclaimStabilityFieldActor::OnFieldEndOverlap);
}

void AReclaimStabilityFieldActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
		World->GetTimerManager().ClearTimer(TargetRefreshTimerHandle);
	}

	if (HasAuthority())
	{
		ClearAllFieldEffects_Server();
	}

	Super::EndPlay(EndPlayReason);
}

void AReclaimStabilityFieldActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimStabilityFieldActor, SourceCharacter);
	DOREPLIFETIME(AReclaimStabilityFieldActor, FieldRadius);
	DOREPLIFETIME(AReclaimStabilityFieldActor, EndServerTime);
}

bool AReclaimStabilityFieldActor::InitializeStabilityField_Server(AReclaimPlayerCharacter* NewSourceCharacter, const UReclaimAbilityDefinition* AbilityDefinition)
{
	if (!HasAuthority() || !NewSourceCharacter || !AbilityDefinition)
	{
		return false;
	}

	const AReclaimPlayerState* SourcePlayerState = NewSourceCharacter->GetReclaimPlayerState();
	if (!SourcePlayerState || SourcePlayerState->GetLifeState() != EReclaimPlayerLifeState::Active)
	{
		return false;
	}

	SourceCharacter = NewSourceCharacter;
	SetOwner(NewSourceCharacter);
	SetActorLocation(NewSourceCharacter->GetActorLocation());
	FieldRadius = AbilityDefinition->Radius > 0.0f ? AbilityDefinition->Radius : DefaultRadius;
	if (FieldCollision)
	{
		FieldCollision->SetSphereRadius(FieldRadius);
		FieldCollision->UpdateOverlaps();
	}

	UWorld* World = GetWorld();
	const float Duration = AbilityDefinition->Duration > 0.0f ? AbilityDefinition->Duration : DefaultDurationSeconds;
	EndServerTime = World ? World->GetTimeSeconds() + Duration : Duration;

	RefreshFieldTargets_Server();
	if (World)
	{
		World->GetTimerManager().SetTimer(TargetRefreshTimerHandle, this, &AReclaimStabilityFieldActor::RefreshFieldTargets_Server, FMath::Max(0.05f, TargetRefreshInterval), true);
		if (Duration > 0.0f)
		{
			World->GetTimerManager().SetTimer(DurationTimerHandle, this, &AReclaimStabilityFieldActor::EndStabilityField_Server, Duration, false);
		}
	}

	return true;
}

void AReclaimStabilityFieldActor::EndStabilityField_Server()
{
	if (HasAuthority())
	{
		Destroy();
	}
}

void AReclaimStabilityFieldActor::OnFieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		TryApplyFieldToActor_Server(OtherActor);
	}
}

void AReclaimStabilityFieldActor::OnFieldEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		RemoveFieldFromAbilitySystem_Server(AbilitySystem);
	}
}

void AReclaimStabilityFieldActor::RefreshFieldTargets_Server()
{
	if (!HasAuthority() || !FieldCollision)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	FieldCollision->GetOverlappingActors(OverlappingActors, AReclaimPlayerCharacter::StaticClass());
	TSet<TWeakObjectPtr<UAbilitySystemComponent>> CurrentOverlappingAbilitySystems;
	for (AActor* Actor : OverlappingActors)
	{
		UAbilitySystemComponent* AbilitySystem = nullptr;
		if (IsActorEligibleForField_Server(Actor, AbilitySystem))
		{
			CurrentOverlappingAbilitySystems.Add(AbilitySystem);
			TryApplyFieldToActor_Server(Actor);
		}
	}

	TArray<TWeakObjectPtr<UAbilitySystemComponent>> ExistingAbilitySystems = AffectedAbilitySystems.Array();
	for (const TWeakObjectPtr<UAbilitySystemComponent>& AbilitySystemPtr : ExistingAbilitySystems)
	{
		if (!AbilitySystemPtr.IsValid() || !CurrentOverlappingAbilitySystems.Contains(AbilitySystemPtr))
		{
			RemoveFieldFromAbilitySystem_Server(AbilitySystemPtr.Get());
		}
	}
}

bool AReclaimStabilityFieldActor::TryApplyFieldToActor_Server(AActor* Actor)
{
	UAbilitySystemComponent* AbilitySystem = nullptr;
	if (!IsActorEligibleForField_Server(Actor, AbilitySystem) || !AbilitySystem || AffectedAbilitySystems.Contains(AbilitySystem))
	{
		return false;
	}

	AffectedAbilitySystems.Add(AbilitySystem);
	const int32 ExistingCount = AbilitySystem->GetGameplayTagCount(ReclaimGameplayTags::State_StabilityField);
	AbilitySystem->SetLooseGameplayTagCount(ReclaimGameplayTags::State_StabilityField, ExistingCount + 1);
	return true;
}

void AReclaimStabilityFieldActor::RemoveFieldFromAbilitySystem_Server(UAbilitySystemComponent* AbilitySystem)
{
	if (!AbilitySystem || !AffectedAbilitySystems.Remove(AbilitySystem))
	{
		return;
	}

	const int32 ExistingCount = AbilitySystem->GetGameplayTagCount(ReclaimGameplayTags::State_StabilityField);
	AbilitySystem->SetLooseGameplayTagCount(ReclaimGameplayTags::State_StabilityField, FMath::Max(0, ExistingCount - 1));
}

void AReclaimStabilityFieldActor::ClearAllFieldEffects_Server()
{
	TArray<TWeakObjectPtr<UAbilitySystemComponent>> ExistingAbilitySystems = AffectedAbilitySystems.Array();
	for (const TWeakObjectPtr<UAbilitySystemComponent>& AbilitySystemPtr : ExistingAbilitySystems)
	{
		RemoveFieldFromAbilitySystem_Server(AbilitySystemPtr.Get());
	}
	AffectedAbilitySystems.Reset();
}

bool AReclaimStabilityFieldActor::IsActorEligibleForField_Server(AActor* Actor, UAbilitySystemComponent*& OutAbilitySystem) const
{
	OutAbilitySystem = nullptr;
	AReclaimPlayerCharacter* PlayerCharacter = Cast<AReclaimPlayerCharacter>(Actor);
	const AReclaimPlayerState* ReclaimPlayerState = PlayerCharacter ? PlayerCharacter->GetReclaimPlayerState() : nullptr;
	if (!PlayerCharacter || !ReclaimPlayerState || ReclaimPlayerState->GetLifeState() != EReclaimPlayerLifeState::Active)
	{
		return false;
	}

	if (FieldRadius > 0.0f && FVector::DistSquared(GetActorLocation(), PlayerCharacter->GetActorLocation()) > FMath::Square(FieldRadius))
	{
		return false;
	}

	OutAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter);
	return OutAbilitySystem != nullptr;
}
