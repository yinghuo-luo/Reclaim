// Copyright Epic Games, Inc. All Rights Reserved.

#include "Devices/ReclaimDeployableBase.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AReclaimDeployableBase::AReclaimDeployableBase()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = false;

	DeployableCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DeployableCollision"));
	SetRootComponent(DeployableCollision);
	DeployableCollision->SetBoxExtent(FVector(60.0f, 60.0f, 60.0f));
	DeployableCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DeployableCollision->SetCollisionObjectType(ECC_WorldDynamic);
	DeployableCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	DeployableCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DeployableCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void AReclaimDeployableBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ResetDurability_Server();
	}
}

void AReclaimDeployableBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OverclockTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AReclaimDeployableBase::InitializeDeployable(AController* NewOwningController)
{
	if (HasAuthority())
	{
		OwningController = NewOwningController;
		ResetDurability_Server();
	}
}

AController* AReclaimDeployableBase::GetReclaimOwningController_Implementation() const
{
	return OwningController;
}

bool AReclaimDeployableBase::CanBeOverclockedBy_Implementation(AController* SourceController) const
{
	return SourceController != nullptr
		&& OwningController != nullptr
		&& SourceController == OwningController
		&& !bDestroyed
		&& !IsPendingKillPending();
}

void AReclaimDeployableBase::ApplyOverclock_Server_Implementation(AController* SourceController, float Duration, float PerformanceMultiplier, float DurabilityEfficiencyMultiplier, float CooldownRecoveryMultiplier)
{
	if (!HasAuthority() || !CanBeOverclockedBy_Implementation(SourceController))
	{
		return;
	}

	bOverclocked = true;
	OverclockPerformanceMultiplier = FMath::Max(0.01f, PerformanceMultiplier);
	OverclockDurabilityEfficiencyMultiplier = FMath::Max(0.01f, DurabilityEfficiencyMultiplier);
	OverclockCooldownRecoveryMultiplier = FMath::Max(0.01f, CooldownRecoveryMultiplier);

	UWorld* World = GetWorld();
	const float ClampedDuration = FMath::Max(0.0f, Duration);
	OverclockEndServerTime = World ? World->GetTimeSeconds() + ClampedDuration : ClampedDuration;

	if (World)
	{
		World->GetTimerManager().ClearTimer(OverclockTimerHandle);
		if (ClampedDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(OverclockTimerHandle, this, &AReclaimDeployableBase::ClearOverclockInternal_Server, ClampedDuration, false);
		}
	}

	OnOverclockStateChanged(true);
}

void AReclaimDeployableBase::ClearOverclock_Server_Implementation(AController* SourceController)
{
	if (HasAuthority() && (!SourceController || SourceController == OwningController))
	{
		ClearOverclockInternal_Server();
	}
}

bool AReclaimDeployableBase::CanReceiveRepairFrom_Implementation(AActor* SourceActor) const
{
	return HasAuthority()
		&& bRepairable
		&& !bDestroyed
		&& !IsPendingKillPending()
		&& IsRepairSourceAllowed(SourceActor)
		&& CurrentDurability < MaxDurability;
}

float AReclaimDeployableBase::ApplyRepair_Server_Implementation(AActor* SourceActor, float HealthAmount, float ShieldAmount)
{
	if (!HasAuthority() || !CanReceiveRepairFrom_Implementation(SourceActor))
	{
		return 0.0f;
	}

	const float RepairAmount = FMath::Max(0.0f, HealthAmount) + FMath::Max(0.0f, ShieldAmount);
	if (RepairAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float StartingDurability = CurrentDurability;
	const float Efficiency = bOverclocked ? FMath::Max(0.01f, OverclockDurabilityEfficiencyMultiplier) : 1.0f;
	CurrentDurability = FMath::Clamp(CurrentDurability + RepairAmount * Efficiency, 0.0f, MaxDurability);
	OnDurabilityChanged(CurrentDurability, MaxDurability);
	return FMath::Max(0.0f, CurrentDurability - StartingDurability);
}

float AReclaimDeployableBase::ApplyDurabilityDamage_Server(float DamageAmount, AActor* DamageInstigator)
{
	if (!HasAuthority() || bDestroyed)
	{
		return 0.0f;
	}

	const float IncomingDamage = FMath::Max(0.0f, DamageAmount);
	if (IncomingDamage <= 0.0f)
	{
		return 0.0f;
	}

	const float StartingDurability = CurrentDurability;
	const float Efficiency = bOverclocked ? FMath::Max(0.01f, OverclockDurabilityEfficiencyMultiplier) : 1.0f;
	CurrentDurability = FMath::Clamp(CurrentDurability - IncomingDamage / Efficiency, 0.0f, MaxDurability);
	OnDurabilityChanged(CurrentDurability, MaxDurability);

	if (CurrentDurability <= 0.0f)
	{
		bDestroyed = true;
		ClearOverclockInternal_Server();
		ApplyDestroyedState();
		OnDeployableDestroyed();
	}

	return FMath::Max(0.0f, StartingDurability - CurrentDurability);
}

void AReclaimDeployableBase::OnRep_Overclocked()
{
	OnOverclockStateChanged(bOverclocked);
}

void AReclaimDeployableBase::OnRep_Durability()
{
	OnDurabilityChanged(CurrentDurability, MaxDurability);
}

void AReclaimDeployableBase::OnRep_Destroyed()
{
	if (bDestroyed)
	{
		ApplyDestroyedState();
		OnDeployableDestroyed();
	}
}

void AReclaimDeployableBase::ClearOverclockInternal_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OverclockTimerHandle);
	}

	bOverclocked = false;
	OverclockEndServerTime = 0.0f;
	OverclockPerformanceMultiplier = 1.0f;
	OverclockDurabilityEfficiencyMultiplier = 1.0f;
	OverclockCooldownRecoveryMultiplier = 1.0f;
	OnOverclockStateChanged(false);
}

void AReclaimDeployableBase::ResetDurability_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	MaxDurability = FMath::Max(1.0f, MaxDurability);
	CurrentDurability = MaxDurability;
	bDestroyed = false;
	if (DeployableCollision)
	{
		DeployableCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	OnDurabilityChanged(CurrentDurability, MaxDurability);
}

void AReclaimDeployableBase::ApplyDestroyedState()
{
	SetActorEnableCollision(false);
	if (DeployableCollision)
	{
		DeployableCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DeployableCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

bool AReclaimDeployableBase::IsRepairSourceAllowed(AActor* SourceActor) const
{
	if (!SourceActor)
	{
		return false;
	}

	if (!OwningController)
	{
		return true;
	}

	if (const AController* SourceController = Cast<AController>(SourceActor))
	{
		return SourceController == OwningController;
	}

	const APawn* SourcePawn = Cast<APawn>(SourceActor);
	return SourcePawn && SourcePawn->GetController() == OwningController;
}

void AReclaimDeployableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AReclaimDeployableBase, OwningController);
	DOREPLIFETIME(AReclaimDeployableBase, CurrentDurability);
	DOREPLIFETIME(AReclaimDeployableBase, bDestroyed);
	DOREPLIFETIME(AReclaimDeployableBase, bOverclocked);
	DOREPLIFETIME(AReclaimDeployableBase, OverclockEndServerTime);
	DOREPLIFETIME(AReclaimDeployableBase, OverclockPerformanceMultiplier);
	DOREPLIFETIME(AReclaimDeployableBase, OverclockDurabilityEfficiencyMultiplier);
	DOREPLIFETIME(AReclaimDeployableBase, OverclockCooldownRecoveryMultiplier);
}

void AReclaimDeployableBase::OnOverclockStateChanged_Implementation(bool bNewOverclocked)
{
}

void AReclaimDeployableBase::OnDurabilityChanged_Implementation(float NewDurability, float NewMaxDurability)
{
}

void AReclaimDeployableBase::OnDeployableDestroyed_Implementation()
{
}
