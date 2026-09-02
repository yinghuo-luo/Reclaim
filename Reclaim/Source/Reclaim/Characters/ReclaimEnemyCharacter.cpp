// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ReclaimEnemyCharacter.h"

#include "AI/ReclaimAIConfig.h"
#include "AI/ReclaimEnemyAIController.h"
#include "AI/ReclaimEnemyDirector.h"
#include "AbilitySystem/ReclaimAbilitySystemComponent.h"
#include "AbilitySystem/ReclaimAttributeSet.h"
#include "Components/ReclaimTargetingComponent.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Core/ReclaimGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogReclaimEnemyCharacter, Log, All);

namespace
{
	bool IsEnemyDefinitionElite(const UReclaimEnemyDefinition* Definition)
	{
		return Definition
			&& (Definition->EnemyTags.HasTagExact(ReclaimGameplayTags::Enemy_Elite)
				|| Definition->EliteAffix != EReclaimEnemyEliteAffix::None);
	}

	FString IntentToDebugString(EReclaimEnemyIntentType IntentType)
	{
		if (const UEnum* IntentEnum = StaticEnum<EReclaimEnemyIntentType>())
		{
			return IntentEnum->GetNameStringByValue(static_cast<int64>(IntentType));
		}

		return FString::FromInt(static_cast<int32>(IntentType));
	}
}

AReclaimEnemyCharacter::AReclaimEnemyCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = false;
	AbilitySystemComponent = CreateDefaultSubobject<UReclaimAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UReclaimAttributeSet>(TEXT("AttributeSet"));
	HealthShieldComponent = CreateDefaultSubobject<UReclaimHealthShieldComponent>(TEXT("HealthShieldComponent"));
	TargetingComponent = CreateDefaultSubobject<UReclaimTargetingComponent>(TEXT("TargetingComponent"));
	AIControllerClass = AReclaimEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent* AReclaimEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AReclaimEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (HealthShieldComponent)
	{
		HealthShieldComponent->OnDamageAppliedDelegate.AddDynamic(this, &AReclaimEnemyCharacter::HandleDamageApplied);
	}

	if (HasAuthority())
	{
		ApplyDefinitionRuntime_Server(1.0f);
		TryAutoRegisterWithDirector_Server();
	}
}

void AReclaimEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AReclaimEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimEnemyCharacter, EnemyDefinition);
	DOREPLIFETIME(AReclaimEnemyCharacter, bDead);
	DOREPLIFETIME(AReclaimEnemyCharacter, bStaggered);
	DOREPLIFETIME(AReclaimEnemyCharacter, bArmorBroken);
	DOREPLIFETIME(AReclaimEnemyCharacter, bElite);
	DOREPLIFETIME(AReclaimEnemyCharacter, EliteAffix);
	DOREPLIFETIME(AReclaimEnemyCharacter, DecisionSeed);
	DOREPLIFETIME(AReclaimEnemyCharacter, CurrentAITargetActor);
	DOREPLIFETIME(AReclaimEnemyCharacter, CurrentAIIntent);
}

FGameplayTagContainer AReclaimEnemyCharacter::GetReclaimTargetTags_Implementation() const
{
	return TargetingComponent ? TargetingComponent->GetTargetTags() : FGameplayTagContainer();
}

bool AReclaimEnemyCharacter::IsHostileTo_Implementation(AActor* SourceActor) const
{
	return SourceActor != nullptr && SourceActor != this && !bDead;
}

bool AReclaimEnemyCharacter::CanBeMarkedBy_Implementation(AActor* SourceActor) const
{
	return SourceActor != nullptr && !bDead;
}

void AReclaimEnemyCharacter::ApplyMark_Server_Implementation(AActor* SourceActor, float Duration, FGameplayTag MarkTag)
{
	if (HasAuthority() && TargetingComponent && !bDead)
	{
		TargetingComponent->ApplyMark_Server(SourceActor, Duration, MarkTag);
	}
}

bool AReclaimEnemyCharacter::CanBePushedBy_Implementation(AActor* SourceActor, const FGameplayTagContainer& SourceTags) const
{
	return SourceActor != nullptr
		&& !bDead
		&& EnemyDefinition
		&& EnemyDefinition->bCanBePushed;
}

void AReclaimEnemyCharacter::ApplyPush_Server_Implementation(AActor* SourceActor, FVector Direction, float Strength, float Duration)
{
	if (!HasAuthority() || bDead || !EnemyDefinition || !EnemyDefinition->bCanBePushed)
	{
		return;
	}

	const FVector PushDirection = Direction.GetSafeNormal2D();
	if (!PushDirection.IsNearlyZero())
	{
		LaunchCharacter(PushDirection * FMath::Max(0.0f, Strength), true, true);
	}

	ApplyStagger_Server(SourceActor, Duration);
}

bool AReclaimEnemyCharacter::CanBePurifiedBy_Implementation(AActor* SourceActor) const
{
	return SourceActor != nullptr && !bDead;
}

void AReclaimEnemyCharacter::Purify_Server_Implementation(AActor* SourceActor, float PurificationMagnitude)
{
	if (!HasAuthority() || bDead || !HealthShieldComponent)
	{
		return;
	}

	FReclaimDamageApplicationResult Result;
	HealthShieldComponent->ApplyDamage_Server(FMath::Max(0.0f, PurificationMagnitude), SourceActor, ReclaimGameplayTags::Damage_Electric, Result);
	ApplyStagger_Server(SourceActor, 0.75f);
}

void AReclaimEnemyCharacter::InitializeEnemy_Server(UReclaimEnemyDefinition* NewEnemyDefinition, UReclaimAIConfig* NewAIConfig, float EnemyHealthScalar, int32 NewDecisionSeed, AReclaimEnemyDirector* NewOwningDirector)
{
	if (!HasAuthority())
	{
		return;
	}

	EnemyDefinition = NewEnemyDefinition;
	if (NewAIConfig)
	{
		RuntimeAIConfig = NewAIConfig;
	}
	else
	{
		RuntimeAIConfig = EnemyDefinition ? EnemyDefinition->AIConfig.Get() : nullptr;
	}
	DecisionSeed = NewDecisionSeed;
	OwningDirector = NewOwningDirector;
	ApplyDefinitionRuntime_Server(EnemyHealthScalar);
}

void AReclaimEnemyCharacter::SetEnemyDefinition_Server(UReclaimEnemyDefinition* NewEnemyDefinition)
{
	if (HasAuthority())
	{
		EnemyDefinition = NewEnemyDefinition;
		ApplyDefinitionRuntime_Server(1.0f);
	}
}

void AReclaimEnemyCharacter::SetAIConfig_Server(UReclaimAIConfig* NewAIConfig)
{
	if (HasAuthority())
	{
		RuntimeAIConfig = NewAIConfig;
	}
}

void AReclaimEnemyCharacter::SetCurrentAITarget_Server(AActor* NewTargetActor)
{
	if (HasAuthority())
	{
		CurrentAITargetActor = NewTargetActor;
	}
}

void AReclaimEnemyCharacter::SetCurrentAIIntent_Server(EReclaimEnemyIntentType NewIntent)
{
	if (HasAuthority())
	{
		CurrentAIIntent = NewIntent;
	}
}

bool AReclaimEnemyCharacter::TryCommitActionCooldown_Server(EReclaimEnemyIntentType IntentType)
{
	if (!HasAuthority() || bDead || bStaggered || !IsActionReady(IntentType))
	{
		return false;
	}

	SetLastActionTime(IntentType, GetServerTimeSeconds());
	return true;
}

bool AReclaimEnemyCharacter::IsActionReady(EReclaimEnemyIntentType IntentType) const
{
	const float CooldownSeconds = GetCooldownSecondsForIntent(IntentType);
	const float LastActionTime = GetLastActionTime(IntentType);
	return GetServerTimeSeconds() + UE_KINDA_SMALL_NUMBER >= LastActionTime + CooldownSeconds;
}

int32 AReclaimEnemyCharacter::GetThreatCost() const
{
	return EnemyDefinition ? FMath::Max(1, EnemyDefinition->ThreatCost) : 1;
}

bool AReclaimEnemyCharacter::CountsTowardSpecialUnitCap() const
{
	return EnemyDefinition && EnemyDefinition->bCountsTowardSpecialUnitCap;
}

float AReclaimEnemyCharacter::ModifyIncomingDamage_Server(float DamageAmount, AActor* DamageInstigator, FGameplayTag DamageTypeTag) const
{
	float ModifiedDamage = FMath::Max(0.0f, DamageAmount);
	if (!EnemyDefinition || ModifiedDamage <= 0.0f)
	{
		return ModifiedDamage;
	}

	if (EnemyDefinition->Armor > 0.0f && !bArmorBroken)
	{
		ModifiedDamage *= 1.0f - FMath::Clamp(EnemyDefinition->ArmorDamageReduction, 0.0f, 0.95f);
	}

	return ModifiedDamage;
}

void AReclaimEnemyCharacter::ApplyArmorBreak_Server(AActor* SourceActor)
{
	if (HasAuthority() && EnemyDefinition && EnemyDefinition->Armor > 0.0f)
	{
		bArmorBroken = true;
	}
}

void AReclaimEnemyCharacter::ApplyStagger_Server(AActor* SourceActor, float Duration)
{
	if (!HasAuthority() || bDead || !EnemyDefinition || !EnemyDefinition->bCanBeStaggered)
	{
		return;
	}

	bStaggered = true;
	OnEnemyStaggered(true);

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
		if (Duration > 0.0f)
		{
			World->GetTimerManager().SetTimer(StaggerTimerHandle, this, &AReclaimEnemyCharacter::ClearStagger_Server, Duration, false);
		}
	}
}

FString AReclaimEnemyCharacter::GetEnemyDebugString() const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	const FName EnemyId = EnemyDefinition ? EnemyDefinition->EnemyId : NAME_None;
	return FString::Printf(
		TEXT("Enemy=%s Authority=%s Dead=%s Elite=%s Affix=%d Threat=%d Target=%s Intent=%s ArmorBroken=%s Staggered=%s"),
		*EnemyId.ToString(),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		bDead ? TEXT("true") : TEXT("false"),
		bElite ? TEXT("true") : TEXT("false"),
		static_cast<int32>(EliteAffix),
		GetThreatCost(),
		*GetNameSafe(CurrentAITargetActor),
		*IntentToDebugString(CurrentAIIntent),
		bArmorBroken ? TEXT("true") : TEXT("false"),
		bStaggered ? TEXT("true") : TEXT("false"));
#endif
}

void AReclaimEnemyCharacter::HandleDamageApplied(const FReclaimDamageApplicationResult& Result)
{
	if (HasAuthority() && Result.bTargetDefeated)
	{
		HandleDeath_Server();
	}
}

void AReclaimEnemyCharacter::OnRep_Dead()
{
	if (bDead)
	{
		ApplyDeathState();
		OnEnemyDeath();
	}
}

void AReclaimEnemyCharacter::OnRep_Staggered()
{
	OnEnemyStaggered(bStaggered);
}

void AReclaimEnemyCharacter::ApplyDefinitionRuntime_Server(float EnemyHealthScalar)
{
	if (!HasAuthority() || !EnemyDefinition)
	{
		return;
	}

	RuntimeAIConfig = RuntimeAIConfig ? RuntimeAIConfig : EnemyDefinition->AIConfig;
	bElite = IsEnemyDefinitionElite(EnemyDefinition);
	EliteAffix = EnemyDefinition->EliteAffix;
	bArmorBroken = false;
	bDead = false;
	bStaggered = false;

	FGameplayTagContainer RuntimeTags = EnemyDefinition->EnemyTags;
	RuntimeTags.AddTag(ReclaimGameplayTags::Target_Hostile);
	RuntimeTags.AddTag(ReclaimGameplayTags::Target_Purifiable);
	if (bElite)
	{
		RuntimeTags.AddTag(ReclaimGameplayTags::Enemy_Elite);
	}
	if (TargetingComponent)
	{
		TargetingComponent->SetTargetTags_Server(RuntimeTags);
	}

	if (HealthShieldComponent)
	{
		const float Health = FMath::Max(1.0f, EnemyDefinition->MaxHealth * FMath::Max(0.0f, EnemyHealthScalar));
		const float Shield = FMath::Max(0.0f, EnemyDefinition->MaxShield + (EliteAffix == EReclaimEnemyEliteAffix::Shield ? EnemyDefinition->EliteShieldBonus : 0.0f));
		HealthShieldComponent->InitializeAttributes_Server(Health, Shield, EnemyDefinition->ShieldRegenRate);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = FMath::Max(0.0f, EnemyDefinition->MaxWalkSpeed);
		MovementComponent->MaxAcceleration = FMath::Max(0.0f, EnemyDefinition->MaxAcceleration);
	}
}

void AReclaimEnemyCharacter::HandleDeath_Server()
{
	if (!HasAuthority() || bDead)
	{
		return;
	}

	bDead = true;
	CurrentAIIntent = EReclaimEnemyIntentType::Dead;
	CurrentAITargetActor = nullptr;
	ApplyDeathState();
	OnEnemyDeath();

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
		if (AReclaimEnemyAIController* ReclaimAIController = Cast<AReclaimEnemyAIController>(AIController))
		{
			ReclaimAIController->StopAI_Server();
		}
	}

	SetLifeSpan(8.0f);
	NotifyDirectorDeath_Server();
}

void AReclaimEnemyCharacter::ApplyDeathState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
	}

	bStaggered = false;
	SetActorEnableCollision(false);
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->DisableMovement();
	}

	if (HasAuthority() && TargetingComponent)
	{
		TargetingComponent->ClearMark_Server();
		TargetingComponent->SetTargetTags_Server(FGameplayTagContainer());
	}
}

void AReclaimEnemyCharacter::ClearStagger_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	bStaggered = false;
	OnEnemyStaggered(false);
}

float AReclaimEnemyCharacter::GetCooldownSecondsForIntent(EReclaimEnemyIntentType IntentType) const
{
	if (!EnemyDefinition)
	{
		return 0.0f;
	}

	switch (IntentType)
	{
	case EReclaimEnemyIntentType::RangedAttack:
		return EnemyDefinition->RangedCooldownSeconds;
	case EReclaimEnemyIntentType::LeapAttack:
		return EnemyDefinition->LeapCooldownSeconds;
	case EReclaimEnemyIntentType::HeavyAttack:
		return EnemyDefinition->HeavyCooldownSeconds;
	case EReclaimEnemyIntentType::Charge:
		return EnemyDefinition->ChargeCooldownSeconds;
	case EReclaimEnemyIntentType::MeleeAttack:
	default:
		return EnemyDefinition->MeleeCooldownSeconds;
	}
}

float AReclaimEnemyCharacter::GetLastActionTime(EReclaimEnemyIntentType IntentType) const
{
	switch (IntentType)
	{
	case EReclaimEnemyIntentType::RangedAttack:
		return LastRangedTime;
	case EReclaimEnemyIntentType::LeapAttack:
		return LastLeapTime;
	case EReclaimEnemyIntentType::HeavyAttack:
		return LastHeavyTime;
	case EReclaimEnemyIntentType::Charge:
		return LastChargeTime;
	case EReclaimEnemyIntentType::MeleeAttack:
	default:
		return LastMeleeTime;
	}
}

void AReclaimEnemyCharacter::SetLastActionTime(EReclaimEnemyIntentType IntentType, float NewTime)
{
	switch (IntentType)
	{
	case EReclaimEnemyIntentType::RangedAttack:
		LastRangedTime = NewTime;
		break;
	case EReclaimEnemyIntentType::LeapAttack:
		LastLeapTime = NewTime;
		break;
	case EReclaimEnemyIntentType::HeavyAttack:
		LastHeavyTime = NewTime;
		break;
	case EReclaimEnemyIntentType::Charge:
		LastChargeTime = NewTime;
		break;
	case EReclaimEnemyIntentType::MeleeAttack:
	default:
		LastMeleeTime = NewTime;
		break;
	}
}

float AReclaimEnemyCharacter::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0f;
}

void AReclaimEnemyCharacter::NotifyDirectorDeath_Server()
{
	if (OwningDirector)
	{
		OwningDirector->NotifyEnemyDied(this);
	}
}

void AReclaimEnemyCharacter::TryAutoRegisterWithDirector_Server()
{
	if (!HasAuthority() || OwningDirector || !EnemyDefinition)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (AReclaimEnemyDirector* OwnerDirector = Cast<AReclaimEnemyDirector>(GetOwner()))
	{
		OwningDirector = OwnerDirector;
		OwningDirector->RegisterEnemy(this);
		return;
	}

	for (TActorIterator<AReclaimEnemyDirector> It(World); It; ++It)
	{
		OwningDirector = *It;
		OwningDirector->RegisterEnemy(this);
		return;
	}
}

void AReclaimEnemyCharacter::OnEnemyDeath_Implementation()
{
}

void AReclaimEnemyCharacter::OnEnemyStaggered_Implementation(bool bNewStaggered)
{
}
