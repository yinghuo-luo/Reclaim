// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReclaimRepairDroneActor.generated.h"

class AReclaimPlayerCharacter;
class USceneComponent;
class UReclaimAbilityDefinition;

UCLASS()
class RECLAIM_API AReclaimRepairDroneActor : public AActor
{
	GENERATED_BODY()

public:
	AReclaimRepairDroneActor();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|RepairDrone")
	bool InitializeRepairDrone_Server(AReclaimPlayerCharacter* NewSourceCharacter, AActor* NewTargetActor, const UReclaimAbilityDefinition* AbilityDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|RepairDrone")
	void EndRepairDrone_Server();

protected:
	bool IsRepairStillValid_Server() const;
	void ApplyRepairTick_Server();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|RepairDrone")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|RepairDrone")
	TObjectPtr<AReclaimPlayerCharacter> SourceCharacter = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|RepairDrone")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|RepairDrone")
	float EndServerTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|RepairDrone", meta=(ClampMin="0"))
	float DefaultDurationSeconds = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|RepairDrone", meta=(ClampMin="0.05"))
	float DefaultTickInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|RepairDrone", meta=(ClampMin="0"))
	float DefaultMaxRange = 1400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|RepairDrone", meta=(ClampMin="0"))
	float DefaultHealthRepairPerTick = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|RepairDrone", meta=(ClampMin="0"))
	float DefaultShieldRepairPerTick = 8.0f;

	float MaxRepairRange = 1400.0f;
	float HealthRepairPerTick = 8.0f;
	float ShieldRepairPerTick = 8.0f;
	FTimerHandle RepairTickTimerHandle;
	FTimerHandle DurationTimerHandle;
};
