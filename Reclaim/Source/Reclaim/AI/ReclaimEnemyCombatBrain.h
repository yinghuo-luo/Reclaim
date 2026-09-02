// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AI/ReclaimAIConfig.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimEnemyCombatBrain.generated.h"

class UReclaimEnemyDefinition;

UENUM(BlueprintType)
enum class EReclaimEnemyIntentType : uint8
{
	Hold,
	Wait,
	Approach,
	Retreat,
	MaintainRange,
	Reposition,
	StrafeLeft,
	StrafeRight,
	UseSkill,
	MeleeAttack,
	RangedAttack,
	LeapAttack,
	HeavyAttack,
	Charge,
	Dead
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimEnemyFacts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceToTarget = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSkillReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasValidTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimPlayerLifeState TargetLifeState = EReclaimPlayerLifeState::Destroyed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OwnHealthRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMeleeReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRangedReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLeapReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHeavyReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bChargeReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimEnemyIntentType PreviousIntent = EReclaimEnemyIntentType::Hold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DecisionSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDead = false;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimEnemyIntent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimEnemyIntentType Type = EReclaimEnemyIntentType::Hold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SkillTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DesiredRange = 0.0f;
};

UCLASS(BlueprintType)
class RECLAIM_API UReclaimEnemyCombatBrain : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|AI")
	FReclaimEnemyIntent DecideIntent(const FReclaimEnemyFacts& Facts, const UReclaimAIConfig* Config) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|AI")
	FReclaimEnemyIntent DecideIntentForEnemy(const FReclaimEnemyFacts& Facts, const UReclaimEnemyDefinition* EnemyDefinition, const UReclaimAIConfig* Config) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|AI")
	EReclaimEnemyIntentType GetLastIntentType() const { return LastIntent.Type; }

protected:
	mutable FReclaimEnemyIntent LastIntent;
};
