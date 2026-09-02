// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimRunInventoryComponent.generated.h"

UCLASS(ClassGroup=(Reclaim), meta=(BlueprintSpawnableComponent))
class RECLAIM_API UReclaimRunInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReclaimRunInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|RunInventory")
	void AddModifier_Server(FName ModifierId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|RunInventory")
	void SetPendingRewardCandidates_Server(const TArray<FReclaimRewardCandidate>& Candidates);

	const TArray<FName>& GetActiveModifierIds() const { return ActiveModifierIds; }
	const TArray<FReclaimRewardCandidate>& GetPendingRewardCandidates() const { return PendingRewardCandidates; }

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|RunInventory")
	TArray<FName> ActiveModifierIds;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|RunInventory")
	TArray<FReclaimRewardCandidate> PendingRewardCandidates;
};
