// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/ReclaimAIConfig.h"
#include "ReclaimEnemyDirector.generated.h"

class AReclaimEnemyCharacter;
class AReclaimEnemySpawnPoint;
class AReclaimPlayerState;
class UReclaimEnemyDefinition;

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimEnemyBudgetEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Director")
	FName EnemyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Director")
	TObjectPtr<UReclaimEnemyDefinition> EnemyDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Director", meta=(ClampMin="1"))
	int32 ThreatCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Director", meta=(ClampMin="0"))
	int32 SpawnWeight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Director")
	bool bSpecialUnit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Director")
	bool bLegal = true;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimEnemySpawnRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	FName EnemyId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	TObjectPtr<UReclaimEnemyDefinition> EnemyDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	int32 ThreatCost = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	bool bSpecialUnit = false;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimThreatBudgetSolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	int32 Budget = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	int32 ThreatSpent = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	int32 SpecialUnits = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	TArray<FReclaimEnemySpawnRequest> Requests;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Director")
	FString CompositionSummary;
};

UCLASS()
class RECLAIM_API AReclaimEnemyDirector : public AActor
{
	GENERATED_BODY()

public:
	AReclaimEnemyDirector();

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	static FReclaimDirectorScalingResult ResolveScaling(const UReclaimAIConfig* Config, int32 EffectivePlayerCount);

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	static int32 ComputeThreatBudget(int32 BaseBudget, float ThreatBudgetScalar);

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	static FReclaimThreatBudgetSolution SolveThreatBudget(int32 Budget, int32 SpecialUnitCap, int32 CompositionSeed, const TArray<FReclaimEnemyBudgetEntry>& Entries);

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	static int32 CompareSpawnCandidatesDeterministic(const FVector& LeftLocation, FName LeftStableId, const FVector& RightLocation, FName RightStableId);

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	static float ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule Rule, bool bOccludedFromActivePlayers);

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	static bool DoesPlayerStateCountForEffectivePlayers(const AReclaimPlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Director")
	void RefreshThreatBudget(int32 InEffectivePlayerCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Director")
	bool SpawnNextWave_Server();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Director")
	void RegisterEnemy(AReclaimEnemyCharacter* EnemyCharacter);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Director")
	void UnregisterEnemy(AReclaimEnemyCharacter* EnemyCharacter);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Director")
	void NotifyEnemyDied(AReclaimEnemyCharacter* EnemyCharacter);

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	int32 GetEffectivePlayerCount() const { return EffectivePlayerCount; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	float GetThreatBudgetScalar() const { return ThreatBudgetScalar; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	float GetEnemyHealthScalar() const { return EnemyHealthScalar; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	int32 GetSpecialUnitCap() const { return SpecialUnitCap; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	int32 GetCurrentBudget() const { return CurrentBudget; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	int32 GetCurrentThreat() const { return CurrentThreat; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Director")
	FString GetLastSpawnComposition() const { return LastSpawnComposition; }

	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetThreatDebugString() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	TArray<FReclaimEnemyBudgetEntry> BuildBudgetEntries() const;
	AReclaimEnemySpawnPoint* ChooseSpawnPoint_Server(int32 RequestIndex, int32 CompositionSeed) const;
	bool IsCurrentIntensityTooHigh_Server() const;
	void RefreshDebugState_Server();
	int32 CalculateEffectivePlayerCount_Server() const;
	int32 CalculateAliveThreat_Server() const;
	int32 CalculateAliveSpecialUnits_Server() const;
	int32 GetRunSeed_Server() const;
	void ScheduleSpawnRetry_Server();
	void SpawnNextWaveTimer_Server();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Director")
	TObjectPtr<UReclaimAIConfig> AIConfig = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Director")
	TArray<TObjectPtr<UReclaimEnemyDefinition>> EnemyRoster;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Director")
	TSubclassOf<AReclaimEnemyCharacter> DefaultEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Director")
	TArray<TObjectPtr<AReclaimEnemySpawnPoint>> ExplicitSpawnPoints;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Director")
	bool bAutoSpawnCombatSandboxWave = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Director", meta=(ClampMin="0"))
	float AutoSpawnDelaySeconds = 1.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Director|Debug")
	int32 EffectivePlayerCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Director|Debug")
	float ThreatBudgetScalar = 1.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Director|Debug")
	float EnemyHealthScalar = 1.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Director|Debug")
	int32 SpecialUnitCap = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Director|Debug")
	int32 CurrentBudget = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Director|Debug")
	int32 CurrentThreat = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Director|Debug")
	FString LastSpawnComposition;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AReclaimEnemyCharacter>> AliveEnemies;

	int32 WaveIndex = 0;
	FTimerHandle SpawnRetryTimerHandle;
};
