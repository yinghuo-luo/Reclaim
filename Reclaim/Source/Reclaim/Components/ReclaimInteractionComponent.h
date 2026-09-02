// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReclaimTypes.h"
#include "TimerManager.h"
#include "ReclaimInteractionComponent.generated.h"

UCLASS(ClassGroup=(Reclaim), meta=(BlueprintSpawnableComponent))
class RECLAIM_API UReclaimInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReclaimInteractionComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Interaction")
	void RequestBeginRevive(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Interaction")
	void RequestCancelRevive();

	UFUNCTION(Server, Reliable)
	void Server_RequestBeginRevive(AActor* TargetActor);

	UFUNCTION(Server, Reliable)
	void Server_RequestCancelRevive();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Interaction")
	void CancelRevive_Server();

protected:
	bool ValidateReviveTarget_Server(AActor* TargetActor, EReclaimReviveValidationResult& OutResult) const;
	void ValidatePendingRevive_Server();
	void CompleteRevive_Server();
	void ClearReviveState_Server();
	void SetRevivingTag_Server(bool bReviving) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Interaction|Revive", meta=(ClampMin="0"))
	float ReviveDurationSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Interaction|Revive", meta=(ClampMin="0"))
	float ReviveMaxDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Interaction|Revive", meta=(ClampMin="0.05"))
	float ReviveValidationInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Interaction|Revive", meta=(ClampMin="0.01", ClampMax="1"))
	float ReviveHealthFraction = 0.5f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Interaction|Revive")
	TObjectPtr<AActor> PendingReviveTarget = nullptr;

	FTimerHandle ReviveTimerHandle;
	FTimerHandle ReviveValidationTimerHandle;
};
