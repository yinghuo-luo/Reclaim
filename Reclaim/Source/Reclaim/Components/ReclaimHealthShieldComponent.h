// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReclaimTypes.h"
#include "GameplayTagContainer.h"
#include "ReclaimHealthShieldComponent.generated.h"

class AActor;
class UAbilitySystemComponent;
class UReclaimAttributeSet;

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimDamageApplicationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	TObjectPtr<AActor> DamageInstigator = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	float IncomingDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	float ShieldDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	float HealthDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	float NewHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	float NewShield = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	bool bShieldBroken = false;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Damage")
	bool bTargetDefeated = false;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimRepairApplicationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	TObjectPtr<AActor> RepairInstigator = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	float IncomingHealthRepair = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	float IncomingShieldRepair = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	float HealthRestored = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	float ShieldRestored = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	float NewHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Repair")
	float NewShield = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReclaimDamageAppliedSignature, const FReclaimDamageApplicationResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReclaimRepairAppliedSignature, const FReclaimRepairApplicationResult&, Result);

UCLASS(ClassGroup=(Reclaim), meta=(BlueprintSpawnableComponent))
class RECLAIM_API UReclaimHealthShieldComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReclaimHealthShieldComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category="Reclaim|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Health")
	float GetShield() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Health")
	float GetMaxShield() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Health")
	float GetShieldRegenRate() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Health")
	EReclaimPlayerLifeState GetLifeState() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Health")
	bool IsDefeated() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Health")
	void InitializeAttributes_Server(float InitialMaxHealth, float InitialMaxShield, float InitialShieldRegenRate);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Health")
	void ResetAttributesToConfiguredDefaults_Server();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Health")
	bool ApplyDamage_Server(float DamageAmount, AActor* DamageInstigator, FGameplayTag DamageTypeTag, FReclaimDamageApplicationResult& OutResult);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Health")
	bool ApplyRepair_Server(float HealthAmount, float ShieldAmount, AActor* RepairInstigator, FReclaimRepairApplicationResult& OutResult);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Health")
	bool ReviveFromDowned_Server(AActor* ReviveInstigator, float HealthFraction = 0.5f);

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Health")
	FReclaimDamageAppliedSignature OnDamageAppliedDelegate;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Health")
	FReclaimRepairAppliedSignature OnRepairAppliedDelegate;

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnDamageApplied(const FReclaimDamageApplicationResult& Result);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnRepairApplied(const FReclaimRepairApplicationResult& Result);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnShieldHit(const FReclaimDamageApplicationResult& Result);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnShieldBroken(const FReclaimDamageApplicationResult& Result);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnHealthHit(const FReclaimDamageApplicationResult& Result);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnTargetDefeated(const FReclaimDamageApplicationResult& Result);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnShieldRechargeStarted();

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnShieldRechargeStopped();

protected:
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_DamageApplied(const FReclaimDamageApplicationResult& Result);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ShieldRechargeStarted();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ShieldRechargeStopped();

	void ApplyDamagePresentation(const FReclaimDamageApplicationResult& Result);
	void InitializeConfiguredAttributes_Server();
	void RestartShieldRechargeDelay_Server();
	void BeginShieldRecharge_Server();
	void RechargeShieldStep_Server();
	void StopShieldRecharge_Server(bool bBroadcast);
	void HandleZeroHealth_Server(const FReclaimDamageApplicationResult& Result);
	void HandleDownedTimeout_Server();
	float ModifyIncomingDamageByOwnerState_Server(float DamageAmount, AActor* DamageInstigator, FGameplayTag DamageTypeTag) const;

	UAbilitySystemComponent* FindAbilitySystem() const;
	const UReclaimAttributeSet* FindAttributeSet() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Defaults", meta=(ClampMin="1"))
	float ConfiguredMaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Defaults", meta=(ClampMin="0"))
	float ConfiguredMaxShield = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Defaults", meta=(ClampMin="0"))
	float ConfiguredShieldRegenRate = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Shield", meta=(ClampMin="0"))
	float ShieldRechargeDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Shield", meta=(ClampMin="0.05"))
	float ShieldRechargeInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Life", meta=(ClampMin="0"))
	float DownedTimeoutSeconds = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Defense", meta=(ClampMin="0", ClampMax="1"))
	float ShieldedFrontalDamageMultiplier = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Defense", meta=(ClampMin="-1", ClampMax="1"))
	float ShieldedFrontalMinimumDot = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Health|Defense", meta=(ClampMin="0", ClampMax="1"))
	float StabilityFieldDamageMultiplier = 0.8f;

	FTimerHandle ShieldRechargeDelayTimerHandle;
	FTimerHandle ShieldRechargeTimerHandle;
	FTimerHandle DownedTimeoutTimerHandle;

	bool bZeroHealthReported = false;
};
