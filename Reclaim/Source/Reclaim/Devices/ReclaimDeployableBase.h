// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ReclaimTargetContracts.h"
#include "ReclaimDeployableBase.generated.h"

UCLASS(Abstract)
class RECLAIM_API AReclaimDeployableBase : public AActor, public IReclaimOverclockableInterface, public IReclaimRepairableInterface
{
	GENERATED_BODY()

public:
	AReclaimDeployableBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual AController* GetReclaimOwningController_Implementation() const override;
	virtual bool CanBeOverclockedBy_Implementation(AController* SourceController) const override;
	virtual void ApplyOverclock_Server_Implementation(AController* SourceController, float Duration, float PerformanceMultiplier, float DurabilityEfficiencyMultiplier, float CooldownRecoveryMultiplier) override;
	virtual void ClearOverclock_Server_Implementation(AController* SourceController) override;
	virtual bool CanReceiveRepairFrom_Implementation(AActor* SourceActor) const override;
	virtual float ApplyRepair_Server_Implementation(AActor* SourceActor, float HealthAmount, float ShieldAmount) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Deployable")
	void InitializeDeployable(AController* NewOwningController);

	UFUNCTION(BlueprintPure, Category="Reclaim|Deployable")
	bool IsOverclocked() const { return bOverclocked; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Deployable")
	bool IsDeployableDestroyed() const { return bDestroyed; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Deployable")
	float GetCurrentDurability() const { return CurrentDurability; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Deployable")
	float GetMaxDurability() const { return MaxDurability; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Deployable")
	float GetOverclockEndServerTime() const { return OverclockEndServerTime; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Deployable")
	float ApplyDurabilityDamage_Server(float DamageAmount, AActor* DamageInstigator);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnOverclockStateChanged(bool bNewOverclocked);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnDurabilityChanged(float NewDurability, float NewMaxDurability);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnDeployableDestroyed();

protected:
	UFUNCTION()
	void OnRep_Overclocked();

	UFUNCTION()
	void OnRep_Durability();

	UFUNCTION()
	void OnRep_Destroyed();

	void ClearOverclockInternal_Server();
	void ResetDurability_Server();
	void ApplyDestroyedState();
	bool IsRepairSourceAllowed(AActor* SourceActor) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Deployable")
	TObjectPtr<class UBoxComponent> DeployableCollision;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Deployable")
	TObjectPtr<AController> OwningController = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Deployable|Durability", meta=(ClampMin="1"))
	float MaxDurability = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Deployable|Durability")
	bool bRepairable = true;

	UPROPERTY(ReplicatedUsing=OnRep_Durability, BlueprintReadOnly, Category="Reclaim|Deployable|Durability")
	float CurrentDurability = 100.0f;

	UPROPERTY(ReplicatedUsing=OnRep_Destroyed, BlueprintReadOnly, Category="Reclaim|Deployable|Durability")
	bool bDestroyed = false;

	UPROPERTY(ReplicatedUsing=OnRep_Overclocked, BlueprintReadOnly, Category="Reclaim|Deployable")
	bool bOverclocked = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Deployable")
	float OverclockEndServerTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Deployable")
	float OverclockPerformanceMultiplier = 1.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Deployable")
	float OverclockDurabilityEfficiencyMultiplier = 1.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Deployable")
	float OverclockCooldownRecoveryMultiplier = 1.0f;

	FTimerHandle OverclockTimerHandle;
};
