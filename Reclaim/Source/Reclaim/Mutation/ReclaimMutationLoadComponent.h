#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReclaimMutationLoadComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReclaimMutationLoadChanged, float, NewLoad, bool, bIsOverloaded);

/**
 * Server-authoritative mutation-load state used by Adapter role abilities.
 *
 * This component intentionally does not replace the existing ASC/Health/weapon
 * network ownership. It only owns the 0..100 Mutation Load state, recovery policy
 * and ability permission query.
 */
UCLASS(ClassGroup=(Reclaim), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RECLAIM_API UReclaimMutationLoadComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UReclaimMutationLoadComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category="Reclaim|Mutation")
    float GetMutationLoad() const { return CurrentMutationLoad; }

    UFUNCTION(BlueprintPure, Category="Reclaim|Mutation")
    float GetMutationLoadNormalized() const;

    UFUNCTION(BlueprintPure, Category="Reclaim|Mutation")
    bool IsHighMutationLoad() const { return CurrentMutationLoad >= HighLoadThreshold; }

    UFUNCTION(BlueprintPure, Category="Reclaim|Mutation")
    bool IsOverloaded() const { return bOverloaded; }

    UFUNCTION(BlueprintPure, Category="Reclaim|Mutation")
    bool CanActivateMutationAbility() const;

    /**
     * Server-only spend. Returns false when the owner is overloaded or Cost is invalid.
     * Effective cost is reduced while the owner has State.StabilityField.
     */
    UFUNCTION(BlueprintCallable, Category="Reclaim|Mutation")
    bool TryConsumeAbilityLoad(float Cost);

    UFUNCTION(BlueprintCallable, Category="Reclaim|Mutation")
    void ReduceMutationLoad(float Amount);

    UFUNCTION(BlueprintCallable, Category="Reclaim|Mutation")
    void ResetMutationLoad(float NewValue = 0.0f);

    UPROPERTY(BlueprintAssignable, Category="Reclaim|Mutation")
    FReclaimMutationLoadChanged OnMutationLoadChanged;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0", ClampMax="100.0"))
    float MaxMutationLoad = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0", ClampMax="100.0"))
    float HighLoadThreshold = 70.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0", ClampMax="100.0"))
    float OverloadUnlockThreshold = 65.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0"))
    float RecoveryPerSecond = 12.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0"))
    float OverloadRecoveryMultiplier = 1.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0"))
    float RecoveryDelayAfterAbility = 1.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.05"))
    float RecoveryTickInterval = 0.20f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0", ClampMax="1.0"))
    float StabilizerFieldCostMultiplier = 0.60f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="1.0"))
    float StabilizerFieldRecoveryMultiplier = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mutation", meta=(ClampMin="0.0"))
    float StabilizerProtectionRecoveryPerSecond = 6.0f;

    UPROPERTY(ReplicatedUsing=OnRep_MutationLoad, BlueprintReadOnly, Category="Mutation")
    float CurrentMutationLoad = 0.0f;

    UPROPERTY(ReplicatedUsing=OnRep_Overloaded, BlueprintReadOnly, Category="Mutation")
    bool bOverloaded = false;

private:
    FTimerHandle RecoveryTimerHandle;
    double LastAbilityUseServerTime = -DBL_MAX;

    UFUNCTION()
    void OnRep_MutationLoad();

    UFUNCTION()
    void OnRep_Overloaded();

    void RecoveryTick();
    void StartRecoveryTimer();
    void StopRecoveryTimerIfIdle();
    void RefreshOverloadState();
    bool HasRuntimeTag(FName TagName) const;
    void RecoverProtection(float DeltaSeconds);
    void BroadcastCurrentState();
};
