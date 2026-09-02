// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReclaimTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReclaimAbilityDefinition.generated.h"

class AActor;
class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;

UCLASS(BlueprintType)
class RECLAIM_API UReclaimAbilityDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|Ability")
	FGameplayTag GetPrimaryAbilityTag() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	FName AbilityId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	FGameplayTagContainer AbilityTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	TSubclassOf<UGameplayEffect> CooldownEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	TSubclassOf<UGameplayEffect> CostEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	TSubclassOf<AActor> ProjectileOrDeployableClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	EReclaimMovementExecutionMode MovementExecutionMode = EReclaimMovementExecutionMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	EReclaimDeployExecutionMode DeployExecutionMode = EReclaimDeployExecutionMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	EReclaimCombatExecutionMode CombatExecutionMode = EReclaimCombatExecutionMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float CooldownSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float ActivationCost = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="1"))
	int32 MaxCharges = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float ChargeRecoverySeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float Range = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float Radius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float Duration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	float Value = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	float SecondaryValue = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0.05"))
	float TickInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float KnockbackStrength = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0", ClampMax="180"))
	float MaxAngleDegrees = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	FGameplayTagContainer PresentationCueTags;
};
