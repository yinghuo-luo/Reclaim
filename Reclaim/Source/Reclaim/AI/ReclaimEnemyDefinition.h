// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReclaimEnemyDefinition.generated.h"

class AReclaimEnemyCharacter;
class AReclaimProjectile;
class UReclaimAIConfig;

UENUM(BlueprintType)
enum class EReclaimEnemyEliteAffix : uint8
{
	None,
	Shield
};

UCLASS(BlueprintType)
class RECLAIM_API UReclaimEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy")
	FName EnemyId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy")
	FGameplayTagContainer EnemyTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy", meta=(ClampMin="1"))
	int32 ThreatCost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy")
	TObjectPtr<UReclaimEnemyDefinition> BaseEnemyDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy")
	TObjectPtr<UReclaimAIConfig> AIConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy")
	TSubclassOf<AReclaimEnemyCharacter> PawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Vitals", meta=(ClampMin="1"))
	float MaxHealth = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Vitals", meta=(ClampMin="0"))
	float MaxShield = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Vitals", meta=(ClampMin="0"))
	float ShieldRegenRate = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Defense", meta=(ClampMin="0"))
	float Armor = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Defense", meta=(ClampMin="0", ClampMax="1"))
	float ArmorDamageReduction = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Defense")
	bool bHasWeakPoint = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Defense", meta=(ClampMin="1"))
	float WeakPointDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Movement", meta=(ClampMin="0"))
	float MaxWalkSpeed = 430.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Movement", meta=(ClampMin="0"))
	float MaxAcceleration = 2048.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float PreferredDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float AttackDistance = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float RetreatDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float MeleeDamage = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float HeavyMeleeDamage = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float ProjectileDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float ProjectileSpeed = 1400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat", meta=(ClampMin="0"))
	float ProjectileLifetimeSeconds = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat")
	TSubclassOf<AReclaimProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Combat")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Cooldowns", meta=(ClampMin="0"))
	float MeleeCooldownSeconds = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Cooldowns", meta=(ClampMin="0"))
	float RangedCooldownSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Cooldowns", meta=(ClampMin="0"))
	float LeapCooldownSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Cooldowns", meta=(ClampMin="0"))
	float HeavyCooldownSeconds = 3.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Cooldowns", meta=(ClampMin="0"))
	float ChargeCooldownSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanMeleeAttack = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanRangedAttack = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanLeapAttack = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanHeavyAttack = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanCharge = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanReposition = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanBePushed = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions")
	bool bCanBeStaggered = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions", meta=(ClampMin="0"))
	float LeapMinDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions", meta=(ClampMin="0"))
	float LeapMaxDistance = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions", meta=(ClampMin="0"))
	float LeapImpulse = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions", meta=(ClampMin="0"))
	float ChargeMinDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions", meta=(ClampMin="0"))
	float ChargeMaxDistance = 1100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Actions", meta=(ClampMin="0"))
	float ChargeImpulse = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Director")
	bool bCountsTowardSpecialUnitCap = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Director")
	bool bEligibleForElite = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Director")
	bool bHighPriorityTarget = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Director", meta=(ClampMin="0"))
	int32 SpawnWeight = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Elite")
	EReclaimEnemyEliteAffix EliteAffix = EReclaimEnemyEliteAffix::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy|Elite", meta=(ClampMin="0"))
	float EliteShieldBonus = 0.0f;
};
