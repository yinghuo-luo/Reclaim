// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "AI/ReclaimAIConfig.h"
#include "ReclaimEnemySpawnPoint.generated.h"

UCLASS()
class RECLAIM_API AReclaimEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AReclaimEnemySpawnPoint();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Spawn")
	bool IsSpawnPointValid_Server(const UReclaimAIConfig* Config) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Spawn")
	bool PassesHardSpawnRules_Server(const UReclaimAIConfig* Config) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Spawn")
	float ComputeSpawnPreferenceScore_Server(const UReclaimAIConfig* Config) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Spawn")
	bool IsOccludedFromActivePlayers_Server(const UReclaimAIConfig* Config) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Spawn")
	FGameplayTag GetSpawnGroupTag() const { return SpawnGroupTag; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Spawn")
	FName GetStableSpawnId() const;

protected:
	bool PassesDistanceRule_Server(const UReclaimAIConfig* Config) const;
	bool PassesVisibilityRule_Server(const UReclaimAIConfig* Config) const;
	bool PassesNavigationRule_Server(const UReclaimAIConfig* Config) const;
	EReclaimSpawnVisibilityRule ResolveVisibilityRule(const UReclaimAIConfig* Config) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Spawn")
	FGameplayTag SpawnGroupTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Spawn")
	FName StableSpawnId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Spawn")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Spawn")
	bool bOverrideMinimumDistance = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Spawn", meta=(EditCondition="bOverrideMinimumDistance", ClampMin="0"))
	float MinimumDistanceFromActivePlayer = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Spawn")
	bool bOverrideVisibilityRule = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Spawn", meta=(EditCondition="bOverrideVisibilityRule"))
	EReclaimSpawnVisibilityRule VisibilityRuleOverride = EReclaimSpawnVisibilityRule::RequireOccluded;
};
