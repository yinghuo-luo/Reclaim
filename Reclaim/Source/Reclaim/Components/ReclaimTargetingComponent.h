// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ReclaimTargetingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReclaimMarkStateChangedSignature);

UCLASS(ClassGroup=(Reclaim), meta=(BlueprintSpawnableComponent))
class RECLAIM_API UReclaimTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReclaimTargetingComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Reclaim|Target")
	const FGameplayTagContainer& GetTargetTags() const { return TargetTags; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Target")
	bool IsMarked() const { return bMarked; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Target")
	FGameplayTag GetMarkTag() const { return MarkTag; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Target")
	float GetMarkEndServerTime() const { return MarkEndServerTime; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Target")
	void SetTargetTags_Server(const FGameplayTagContainer& NewTargetTags);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Target")
	void ApplyMark_Server(AActor* SourceActor, float Duration, FGameplayTag InMarkTag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Target")
	void ClearMark_Server();

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Target")
	FReclaimMarkStateChangedSignature OnMarkStateChangedDelegate;

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnMarkStateChanged();

protected:
	UFUNCTION()
	void OnRep_Marked();

	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category="Reclaim|Target")
	FGameplayTagContainer TargetTags;

	UPROPERTY(ReplicatedUsing=OnRep_Marked, BlueprintReadOnly, Category="Reclaim|Target")
	bool bMarked = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Target")
	FGameplayTag MarkTag;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Target")
	float MarkEndServerTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Target")
	TObjectPtr<AActor> MarkSourceActor = nullptr;

	FTimerHandle MarkTimerHandle;
};
