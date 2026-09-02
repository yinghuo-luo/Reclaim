// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/ReclaimHealthShieldComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ReclaimAttributeSet.h"
#include "Characters/ReclaimEnemyCharacter.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Core/ReclaimCombatMath.h"
#include "Core/ReclaimGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameModes/ReclaimMissionGameMode.h"
#include "TimerManager.h"
#include "Player/ReclaimPlayerState.h"

UReclaimHealthShieldComponent::UReclaimHealthShieldComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UReclaimHealthShieldComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeConfiguredAttributes_Server();
}

void UReclaimHealthShieldComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShieldRechargeDelayTimerHandle);
		World->GetTimerManager().ClearTimer(ShieldRechargeTimerHandle);
		World->GetTimerManager().ClearTimer(DownedTimeoutTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

float UReclaimHealthShieldComponent::GetHealth() const
{
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	return Attributes ? Attributes->GetHealth() : 0.0f;
}

float UReclaimHealthShieldComponent::GetMaxHealth() const
{
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	return Attributes ? Attributes->GetMaxHealth() : 0.0f;
}

float UReclaimHealthShieldComponent::GetShield() const
{
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	return Attributes ? Attributes->GetShield() : 0.0f;
}

float UReclaimHealthShieldComponent::GetMaxShield() const
{
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	return Attributes ? Attributes->GetMaxShield() : 0.0f;
}

float UReclaimHealthShieldComponent::GetShieldRegenRate() const
{
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	return Attributes ? Attributes->GetShieldRegenRate() : 0.0f;
}

EReclaimPlayerLifeState UReclaimHealthShieldComponent::GetLifeState() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AReclaimPlayerState* ReclaimPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (ReclaimPlayerState)
	{
		return ReclaimPlayerState->GetLifeState();
	}

	return EReclaimPlayerLifeState::Active;
}

bool UReclaimHealthShieldComponent::IsDefeated() const
{
	return GetHealth() <= 0.0f;
}

void UReclaimHealthShieldComponent::InitializeAttributes_Server(float InitialMaxHealth, float InitialMaxShield, float InitialShieldRegenRate)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	if (!AbilitySystem)
	{
		return;
	}

	const float MaxHealth = FMath::Max(1.0f, InitialMaxHealth);
	const float MaxShield = FMath::Max(0.0f, InitialMaxShield);
	const float ShieldRegenRate = FMath::Max(0.0f, InitialShieldRegenRate);

	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetHealthAttribute(), MaxHealth);
	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetMaxShieldAttribute(), MaxShield);
	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetShieldAttribute(), MaxShield);
	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetShieldRegenRateAttribute(), ShieldRegenRate);
	bZeroHealthReported = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DownedTimeoutTimerHandle);
	}
	StopShieldRecharge_Server(false);
}

void UReclaimHealthShieldComponent::ResetAttributesToConfiguredDefaults_Server()
{
	InitializeConfiguredAttributes_Server();
}

bool UReclaimHealthShieldComponent::ApplyDamage_Server(float DamageAmount, AActor* DamageInstigator, FGameplayTag DamageTypeTag, FReclaimDamageApplicationResult& OutResult)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	if (!AbilitySystem || !Attributes)
	{
		return false;
	}

	const float ModifiedDamage = ModifyIncomingDamageByOwnerState_Server(DamageAmount, DamageInstigator, DamageTypeTag);

	const FReclaimShieldDamageMathResult MathResult = FReclaimCombatMath::ApplyShieldHealthDamage(
		Attributes->GetHealth(),
		Attributes->GetShield(),
		Attributes->GetMaxHealth(),
		Attributes->GetMaxShield(),
		ModifiedDamage);

	if (MathResult.IncomingDamage <= 0.0f)
	{
		return false;
	}

	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetShieldAttribute(), MathResult.NewShield);
	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetHealthAttribute(), MathResult.NewHealth);

	OutResult.TargetActor = OwnerActor;
	OutResult.DamageInstigator = DamageInstigator;
	OutResult.DamageTypeTag = DamageTypeTag;
	OutResult.IncomingDamage = MathResult.IncomingDamage;
	OutResult.ShieldDamage = MathResult.ShieldDamage;
	OutResult.HealthDamage = MathResult.HealthDamage;
	OutResult.NewHealth = MathResult.NewHealth;
	OutResult.NewShield = MathResult.NewShield;
	OutResult.bShieldBroken = MathResult.bShieldBroken;
	OutResult.bTargetDefeated = MathResult.bTargetDefeated;

	StopShieldRecharge_Server(true);
	if (MathResult.bTargetDefeated)
	{
		HandleZeroHealth_Server(OutResult);
	}
	else if (MathResult.NewShield < Attributes->GetMaxShield())
	{
		RestartShieldRechargeDelay_Server();
	}

	Multicast_DamageApplied(OutResult);
	return true;
}

bool UReclaimHealthShieldComponent::ApplyRepair_Server(float HealthAmount, float ShieldAmount, AActor* RepairInstigator, FReclaimRepairApplicationResult& OutResult)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	if (!AbilitySystem || !Attributes)
	{
		return false;
	}

	const float IncomingHealthRepair = FMath::Max(0.0f, HealthAmount);
	const float IncomingShieldRepair = FMath::Max(0.0f, ShieldAmount);
	if (IncomingHealthRepair <= 0.0f && IncomingShieldRepair <= 0.0f)
	{
		return false;
	}

	const float StartingHealth = Attributes->GetHealth();
	const float StartingShield = Attributes->GetShield();
	const float NewHealth = FMath::Clamp(StartingHealth + IncomingHealthRepair, 0.0f, Attributes->GetMaxHealth());
	const float NewShield = FMath::Clamp(StartingShield + IncomingShieldRepair, 0.0f, Attributes->GetMaxShield());

	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetHealthAttribute(), NewHealth);
	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetShieldAttribute(), NewShield);

	OutResult.TargetActor = OwnerActor;
	OutResult.RepairInstigator = RepairInstigator;
	OutResult.IncomingHealthRepair = IncomingHealthRepair;
	OutResult.IncomingShieldRepair = IncomingShieldRepair;
	OutResult.HealthRestored = FMath::Max(0.0f, NewHealth - StartingHealth);
	OutResult.ShieldRestored = FMath::Max(0.0f, NewShield - StartingShield);
	OutResult.NewHealth = NewHealth;
	OutResult.NewShield = NewShield;

	if (NewHealth > 0.0f)
	{
		bZeroHealthReported = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DownedTimeoutTimerHandle);
		}
	}

	OnRepairAppliedDelegate.Broadcast(OutResult);
	OnRepairApplied(OutResult);
	return OutResult.HealthRestored > 0.0f || OutResult.ShieldRestored > 0.0f;
}

bool UReclaimHealthShieldComponent::ReviveFromDowned_Server(AActor* ReviveInstigator, float HealthFraction)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	AReclaimPlayerState* ReclaimPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (!ReclaimPlayerState || ReclaimPlayerState->GetLifeState() != EReclaimPlayerLifeState::Downed)
	{
		return false;
	}

	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	if (!Attributes)
	{
		return false;
	}

	const float TargetHealth = FMath::Max(1.0f, Attributes->GetMaxHealth() * FMath::Clamp(HealthFraction, 0.01f, 1.0f));
	FReclaimRepairApplicationResult RepairResult;
	ApplyRepair_Server(TargetHealth, 0.0f, ReviveInstigator, RepairResult);
	ReclaimPlayerState->SetLifeState_Server(EReclaimPlayerLifeState::Active);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DownedTimeoutTimerHandle);
	}
	bZeroHealthReported = false;
	return true;
}

void UReclaimHealthShieldComponent::Multicast_DamageApplied_Implementation(const FReclaimDamageApplicationResult& Result)
{
	ApplyDamagePresentation(Result);
}

void UReclaimHealthShieldComponent::Multicast_ShieldRechargeStarted_Implementation()
{
	OnShieldRechargeStarted();
}

void UReclaimHealthShieldComponent::Multicast_ShieldRechargeStopped_Implementation()
{
	OnShieldRechargeStopped();
}

void UReclaimHealthShieldComponent::ApplyDamagePresentation(const FReclaimDamageApplicationResult& Result)
{
	OnDamageAppliedDelegate.Broadcast(Result);
	OnDamageApplied(Result);

	if (Result.ShieldDamage > 0.0f)
	{
		OnShieldHit(Result);
	}

	if (Result.bShieldBroken)
	{
		OnShieldBroken(Result);
	}

	if (Result.HealthDamage > 0.0f)
	{
		OnHealthHit(Result);
	}

	if (Result.bTargetDefeated)
	{
		OnTargetDefeated(Result);
	}
}

void UReclaimHealthShieldComponent::InitializeConfiguredAttributes_Server()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		InitializeAttributes_Server(ConfiguredMaxHealth, ConfiguredMaxShield, ConfiguredShieldRegenRate);
	}
}

void UReclaimHealthShieldComponent::RestartShieldRechargeDelay_Server()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(ShieldRechargeDelayTimerHandle);
	if (ShieldRechargeDelay <= 0.0f)
	{
		BeginShieldRecharge_Server();
		return;
	}

	World->GetTimerManager().SetTimer(ShieldRechargeDelayTimerHandle, this, &UReclaimHealthShieldComponent::BeginShieldRecharge_Server, ShieldRechargeDelay, false);
}

void UReclaimHealthShieldComponent::BeginShieldRecharge_Server()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !World || !Attributes || Attributes->GetShieldRegenRate() <= 0.0f || Attributes->GetShield() >= Attributes->GetMaxShield() || Attributes->GetHealth() <= 0.0f)
	{
		return;
	}

	if (!World->GetTimerManager().IsTimerActive(ShieldRechargeTimerHandle))
	{
		World->GetTimerManager().SetTimer(ShieldRechargeTimerHandle, this, &UReclaimHealthShieldComponent::RechargeShieldStep_Server, FMath::Max(0.05f, ShieldRechargeInterval), true);
		Multicast_ShieldRechargeStarted();
	}
}

void UReclaimHealthShieldComponent::RechargeShieldStep_Server()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	const UReclaimAttributeSet* Attributes = FindAttributeSet();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !World || !AbilitySystem || !Attributes)
	{
		return;
	}

	if (Attributes->GetHealth() <= 0.0f || Attributes->GetShield() >= Attributes->GetMaxShield() || Attributes->GetShieldRegenRate() <= 0.0f)
	{
		StopShieldRecharge_Server(true);
		return;
	}

	const float Interval = FMath::Max(0.05f, ShieldRechargeInterval);
	const float NewShield = FMath::Min(Attributes->GetMaxShield(), Attributes->GetShield() + Attributes->GetShieldRegenRate() * Interval);
	AbilitySystem->SetNumericAttributeBase(UReclaimAttributeSet::GetShieldAttribute(), NewShield);

	if (NewShield >= Attributes->GetMaxShield())
	{
		StopShieldRecharge_Server(true);
	}
}

void UReclaimHealthShieldComponent::StopShieldRecharge_Server(bool bBroadcast)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const bool bWasRecharging = World->GetTimerManager().IsTimerActive(ShieldRechargeTimerHandle);
	World->GetTimerManager().ClearTimer(ShieldRechargeDelayTimerHandle);
	World->GetTimerManager().ClearTimer(ShieldRechargeTimerHandle);

	if (bBroadcast && bWasRecharging)
	{
		Multicast_ShieldRechargeStopped();
	}
}

void UReclaimHealthShieldComponent::HandleZeroHealth_Server(const FReclaimDamageApplicationResult& Result)
{
	if (bZeroHealthReported)
	{
		return;
	}

	bZeroHealthReported = true;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AReclaimPlayerState* ReclaimPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (ReclaimPlayerState && ReclaimPlayerState->GetLifeState() == EReclaimPlayerLifeState::Active)
	{
		ReclaimPlayerState->SetLifeState_Server(EReclaimPlayerLifeState::Downed);
		if (UWorld* World = GetWorld(); World && DownedTimeoutSeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(DownedTimeoutTimerHandle, this, &UReclaimHealthShieldComponent::HandleDownedTimeout_Server, DownedTimeoutSeconds, false);
		}
	}
}

void UReclaimHealthShieldComponent::HandleDownedTimeout_Server()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	AReclaimPlayerState* ReclaimPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (ReclaimPlayerState && ReclaimPlayerState->GetLifeState() == EReclaimPlayerLifeState::Downed)
	{
		ReclaimPlayerState->SetLifeState_Server(EReclaimPlayerLifeState::Destroyed);
		if (UWorld* World = GetWorld())
		{
			if (AReclaimMissionGameMode* MissionGameMode = World->GetAuthGameMode<AReclaimMissionGameMode>())
			{
				MissionGameMode->HandlePlayerDestroyed(ReclaimPlayerState);
			}
		}
	}
}

float UReclaimHealthShieldComponent::ModifyIncomingDamageByOwnerState_Server(float DamageAmount, AActor* DamageInstigator, FGameplayTag DamageTypeTag) const
{
	float ModifiedDamage = FMath::Max(0.0f, DamageAmount);
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || ModifiedDamage <= 0.0f)
	{
		return ModifiedDamage;
	}

	if (const AReclaimEnemyCharacter* EnemyCharacter = Cast<AReclaimEnemyCharacter>(OwnerActor))
	{
		ModifiedDamage = EnemyCharacter->ModifyIncomingDamage_Server(ModifiedDamage, DamageInstigator, DamageTypeTag);
	}

	const UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	if (!AbilitySystem)
	{
		return ModifiedDamage;
	}

	if (AbilitySystem->HasMatchingGameplayTag(ReclaimGameplayTags::State_StabilityField))
	{
		ModifiedDamage *= FMath::Clamp(StabilityFieldDamageMultiplier, 0.0f, 1.0f);
	}

	if (AReclaimPlayerCharacter* PlayerCharacter = Cast<AReclaimPlayerCharacter>(OwnerActor))
	{
		float BarrierRemainingDamage = ModifiedDamage;
		if (PlayerCharacter->TryAbsorbDamageWithKineticBarrier_Server(DamageInstigator, DamageTypeTag, ModifiedDamage, BarrierRemainingDamage))
		{
			ModifiedDamage = BarrierRemainingDamage;
		}
	}

	return ModifiedDamage;
}

UAbilitySystemComponent* UReclaimHealthShieldComponent::FindAbilitySystem() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

const UReclaimAttributeSet* UReclaimHealthShieldComponent::FindAttributeSet() const
{
	const UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	return AbilitySystem ? AbilitySystem->GetSet<UReclaimAttributeSet>() : nullptr;
}

void UReclaimHealthShieldComponent::OnDamageApplied_Implementation(const FReclaimDamageApplicationResult& Result)
{
}

void UReclaimHealthShieldComponent::OnRepairApplied_Implementation(const FReclaimRepairApplicationResult& Result)
{
}

void UReclaimHealthShieldComponent::OnShieldHit_Implementation(const FReclaimDamageApplicationResult& Result)
{
}

void UReclaimHealthShieldComponent::OnShieldBroken_Implementation(const FReclaimDamageApplicationResult& Result)
{
}

void UReclaimHealthShieldComponent::OnHealthHit_Implementation(const FReclaimDamageApplicationResult& Result)
{
}

void UReclaimHealthShieldComponent::OnTargetDefeated_Implementation(const FReclaimDamageApplicationResult& Result)
{
}

void UReclaimHealthShieldComponent::OnShieldRechargeStarted_Implementation()
{
}

void UReclaimHealthShieldComponent::OnShieldRechargeStopped_Implementation()
{
}
