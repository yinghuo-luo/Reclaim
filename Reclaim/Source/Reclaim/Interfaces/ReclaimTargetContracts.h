// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "ReclaimTargetContracts.generated.h"

UINTERFACE(BlueprintType)
class UReclaimTargetableInterface : public UInterface
{
	GENERATED_BODY()
};

class RECLAIM_API IReclaimTargetableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Target")
	FGameplayTagContainer GetReclaimTargetTags() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Target")
	bool IsHostileTo(AActor* SourceActor) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Target")
	bool CanBeMarkedBy(AActor* SourceActor) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="Reclaim|Target")
	void ApplyMark_Server(AActor* SourceActor, float Duration, FGameplayTag MarkTag);
};

UINTERFACE(BlueprintType)
class UReclaimPushableInterface : public UInterface
{
	GENERATED_BODY()
};

class RECLAIM_API IReclaimPushableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Movement")
	bool CanBePushedBy(AActor* SourceActor, const FGameplayTagContainer& SourceTags) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="Reclaim|Movement")
	void ApplyPush_Server(AActor* SourceActor, FVector Direction, float Strength, float Duration);
};

UINTERFACE(BlueprintType)
class UReclaimRepairableInterface : public UInterface
{
	GENERATED_BODY()
};

class RECLAIM_API IReclaimRepairableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Repair")
	bool CanReceiveRepairFrom(AActor* SourceActor) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="Reclaim|Repair")
	float ApplyRepair_Server(AActor* SourceActor, float HealthAmount, float ShieldAmount);
};

UINTERFACE(BlueprintType)
class UReclaimOverclockableInterface : public UInterface
{
	GENERATED_BODY()
};

class RECLAIM_API IReclaimOverclockableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Overclock")
	AController* GetReclaimOwningController() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Overclock")
	bool CanBeOverclockedBy(AController* SourceController) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="Reclaim|Overclock")
	void ApplyOverclock_Server(AController* SourceController, float Duration, float PerformanceMultiplier, float DurabilityEfficiencyMultiplier, float CooldownRecoveryMultiplier);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="Reclaim|Overclock")
	void ClearOverclock_Server(AController* SourceController);
};

UINTERFACE(BlueprintType)
class UReclaimPurifiableInterface : public UInterface
{
	GENERATED_BODY()
};

class RECLAIM_API IReclaimPurifiableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Reclaim|Purification")
	bool CanBePurifiedBy(AActor* SourceActor) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="Reclaim|Purification")
	void Purify_Server(AActor* SourceActor, float PurificationMagnitude);
};
