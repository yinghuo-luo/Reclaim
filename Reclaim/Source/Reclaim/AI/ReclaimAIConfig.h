// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReclaimAIConfig.generated.h"

UENUM(BlueprintType)
enum class EReclaimSpawnVisibilityRule : uint8
{
	Any,
	PreferOccluded,
	RequireOccluded
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimDirectorScalingRow
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="1", ClampMax="4"))
	int32 EffectivePlayers = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float ThreatBudgetScalar = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float EnemyHealthScalar = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	int32 SpecialUnitCap = 1;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimDirectorScalingResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|AI")
	int32 EffectivePlayers = 1;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|AI")
	float ThreatBudgetScalar = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|AI")
	float EnemyHealthScalar = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|AI")
	int32 SpecialUnitCap = 1;
};

UCLASS(BlueprintType)
class RECLAIM_API UReclaimAIConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UReclaimAIConfig();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float DefaultPreferredDistance = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float DefaultAttackDistance = 170.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float DefaultRetreatDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0.05"))
	float DecisionInterval = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float TargetAcquireRadius = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI")
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="1"))
	int32 BaseThreatBudget = 12;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI")
	TArray<FReclaimDirectorScalingRow> PlayerScaling;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float MaxAliveThreatRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float SpawnRetryDelaySeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float MinimumSpawnDistanceFromActivePlayer = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI")
	EReclaimSpawnVisibilityRule SpawnVisibilityRule = EReclaimSpawnVisibilityRule::RequireOccluded;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|AI", meta=(ClampMin="0"))
	float SpawnNavProjectionExtent = 300.0f;

	UFUNCTION(BlueprintPure, Category="Reclaim|AI")
	FReclaimDirectorScalingResult ResolveScaling(int32 EffectivePlayerCount) const;
};
