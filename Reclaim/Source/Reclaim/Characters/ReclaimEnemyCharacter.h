// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "AI/ReclaimEnemyCombatBrain.h"
#include "AI/ReclaimEnemyDefinition.h"
#include "Interfaces/ReclaimTargetContracts.h"
#include "ReclaimEnemyCharacter.generated.h"

class AReclaimEnemyDirector;
class UReclaimAbilitySystemComponent;
class UReclaimAttributeSet;
class UReclaimHealthShieldComponent;
class UReclaimTargetingComponent;
class UReclaimAIConfig;
class UAbilitySystemComponent;

UCLASS()
class RECLAIM_API AReclaimEnemyCharacter : public ACharacter, public IAbilitySystemInterface, public IReclaimTargetableInterface, public IReclaimPushableInterface, public IReclaimPurifiableInterface
{
	GENERATED_BODY()

public:
	AReclaimEnemyCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FGameplayTagContainer GetReclaimTargetTags_Implementation() const override;
	virtual bool IsHostileTo_Implementation(AActor* SourceActor) const override;
	virtual bool CanBeMarkedBy_Implementation(AActor* SourceActor) const override;
	virtual void ApplyMark_Server_Implementation(AActor* SourceActor, float Duration, FGameplayTag MarkTag) override;
	virtual bool CanBePushedBy_Implementation(AActor* SourceActor, const FGameplayTagContainer& SourceTags) const override;
	virtual void ApplyPush_Server_Implementation(AActor* SourceActor, FVector Direction, float Strength, float Duration) override;
	virtual bool CanBePurifiedBy_Implementation(AActor* SourceActor) const override;
	virtual void Purify_Server_Implementation(AActor* SourceActor, float PurificationMagnitude) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	void InitializeEnemy_Server(UReclaimEnemyDefinition* NewEnemyDefinition, UReclaimAIConfig* NewAIConfig, float EnemyHealthScalar, int32 NewDecisionSeed, AReclaimEnemyDirector* NewOwningDirector);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	void SetEnemyDefinition_Server(UReclaimEnemyDefinition* NewEnemyDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	void SetAIConfig_Server(UReclaimAIConfig* NewAIConfig);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	void SetCurrentAITarget_Server(AActor* NewTargetActor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	void SetCurrentAIIntent_Server(EReclaimEnemyIntentType NewIntent);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	bool TryCommitActionCooldown_Server(EReclaimEnemyIntentType IntentType);

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	bool IsActionReady(EReclaimEnemyIntentType IntentType) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	UReclaimEnemyDefinition* GetEnemyDefinition() const { return EnemyDefinition; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	UReclaimAIConfig* GetAIConfig() const { return RuntimeAIConfig; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	UReclaimHealthShieldComponent* GetHealthShieldComponent() const { return HealthShieldComponent; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	UReclaimTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	int32 GetThreatCost() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	bool IsElite() const { return bElite; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	EReclaimEnemyEliteAffix GetEliteAffix() const { return EliteAffix; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	int32 GetDecisionSeed() const { return DecisionSeed; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	EReclaimEnemyIntentType GetCurrentAIIntent() const { return CurrentAIIntent; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	bool CountsTowardSpecialUnitCap() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	bool IsArmorBroken() const { return bArmorBroken; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	bool IsStaggered() const { return bStaggered; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Enemy")
	float ModifyIncomingDamage_Server(float DamageAmount, AActor* DamageInstigator, FGameplayTag DamageTypeTag) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	void ApplyArmorBreak_Server(AActor* SourceActor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Enemy")
	void ApplyStagger_Server(AActor* SourceActor, float Duration);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetEnemyDebugString() const;

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnEnemyDeath();

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnEnemyStaggered(bool bNewStaggered);

protected:
	UFUNCTION()
	void HandleDamageApplied(const FReclaimDamageApplicationResult& Result);

	UFUNCTION()
	void OnRep_Dead();

	UFUNCTION()
	void OnRep_Staggered();

	void ApplyDefinitionRuntime_Server(float EnemyHealthScalar);
	void HandleDeath_Server();
	void ApplyDeathState();
	void ClearStagger_Server();
	float GetCooldownSecondsForIntent(EReclaimEnemyIntentType IntentType) const;
	float GetLastActionTime(EReclaimEnemyIntentType IntentType) const;
	void SetLastActionTime(EReclaimEnemyIntentType IntentType, float NewTime);
	float GetServerTimeSeconds() const;
	void NotifyDirectorDeath_Server();
	void TryAutoRegisterWithDirector_Server();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimHealthShieldComponent> HealthShieldComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimTargetingComponent> TargetingComponent;

	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category="Reclaim|Enemy")
	TObjectPtr<UReclaimEnemyDefinition> EnemyDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Enemy")
	TObjectPtr<UReclaimAIConfig> RuntimeAIConfig = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_Dead, BlueprintReadOnly, Category="Reclaim|Enemy")
	bool bDead = false;

	UPROPERTY(ReplicatedUsing=OnRep_Staggered, BlueprintReadOnly, Category="Reclaim|Enemy")
	bool bStaggered = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Enemy")
	bool bArmorBroken = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Enemy")
	bool bElite = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Enemy")
	EReclaimEnemyEliteAffix EliteAffix = EReclaimEnemyEliteAffix::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Enemy")
	int32 DecisionSeed = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Enemy|Debug")
	TObjectPtr<AActor> CurrentAITargetActor = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Enemy|Debug")
	EReclaimEnemyIntentType CurrentAIIntent = EReclaimEnemyIntentType::Hold;

	UPROPERTY(Transient)
	TObjectPtr<AReclaimEnemyDirector> OwningDirector = nullptr;

	FTimerHandle StaggerTimerHandle;
	float LastMeleeTime = -FLT_MAX;
	float LastRangedTime = -FLT_MAX;
	float LastLeapTime = -FLT_MAX;
	float LastHeavyTime = -FLT_MAX;
	float LastChargeTime = -FLT_MAX;
};
