// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "ReclaimAbilitySystemComponent.generated.h"

class UReclaimAbilityDefinition;

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimAbilityRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Ability")
	float CooldownEndServerTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Ability")
	int32 ChargesRemaining = 1;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Ability")
	float ChargeRecoveryEndServerTime = 0.0f;
};

UCLASS()
class RECLAIM_API UReclaimAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UReclaimAbilitySystemComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Reclaim|Ability")
	bool IsAbilityRuntimeReady(const UReclaimAbilityDefinition* Definition, FString& OutFailureReason) const;

	static bool IsRuntimeStateReadyAtTime(const UReclaimAbilityDefinition* Definition, const FReclaimAbilityRuntimeState* RuntimeState, float CurrentTime, FString& OutFailureReason);
	static void RefreshRuntimeStateAtTime(const UReclaimAbilityDefinition* Definition, FReclaimAbilityRuntimeState& RuntimeState, float CurrentTime);
	static void CommitRuntimeStateAtTime(const UReclaimAbilityDefinition* Definition, FReclaimAbilityRuntimeState& RuntimeState, float CurrentTime);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Ability")
	void RefreshAbilityRuntimeState_Server(const UReclaimAbilityDefinition* Definition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Ability")
	void CommitAbilityRuntimeState_Server(const UReclaimAbilityDefinition* Definition);

	UFUNCTION(BlueprintPure, Category="Reclaim|Ability")
	float GetAbilityCooldownRemaining(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Ability")
	int32 GetAbilityChargesRemaining(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Ability")
	const TArray<FReclaimAbilityRuntimeState>& GetAbilityRuntimeStates() const { return AbilityRuntimeStates; }

protected:
	UFUNCTION()
	void OnRep_AbilityRuntimeStates();

	FReclaimAbilityRuntimeState* FindMutableRuntimeState(FGameplayTag AbilityTag);
	const FReclaimAbilityRuntimeState* FindRuntimeState(FGameplayTag AbilityTag) const;
	FReclaimAbilityRuntimeState& FindOrAddRuntimeState_Server(const UReclaimAbilityDefinition* Definition);
	float GetServerTimeSeconds() const;

	UPROPERTY(ReplicatedUsing=OnRep_AbilityRuntimeStates, BlueprintReadOnly, Category="Reclaim|Ability")
	TArray<FReclaimAbilityRuntimeState> AbilityRuntimeStates;
};
