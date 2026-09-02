#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ReclaimDeployAbility.h"
#include "AbilitySystem/Abilities/ReclaimGenericCombatAbility.h"
#include "AbilitySystem/Abilities/ReclaimMovementAbility.h"
#include "AbilitySystem/Abilities/ReclaimScanAbility.h"
#include "ReclaimMutationAbilities.generated.h"

class UReclaimMutationLoadComponent;

/**
 * Shared mutation-load check/spend mixin behavior is implemented in the cpp helpers.
 * Each class deliberately inherits an existing validated execution family so the
 * M3 networking lifecycle remains intact.
 */

UCLASS()
class RECLAIM_API UReclaimMutationMovementAbility : public UReclaimMovementAbility
{
    GENERATED_BODY()

public:
    UReclaimMutationMovementAbility();

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Reclaim|Mutation")
    float MutationLoadCost = 18.0f;
};

UCLASS()
class RECLAIM_API UReclaimMutationDeployAbility : public UReclaimDeployAbility
{
    GENERATED_BODY()

public:
    UReclaimMutationDeployAbility();

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Reclaim|Mutation")
    float MutationLoadCost = 20.0f;
};

UCLASS()
class RECLAIM_API UReclaimMutationCombatAbility : public UReclaimGenericCombatAbility
{
    GENERATED_BODY()

public:
    UReclaimMutationCombatAbility();

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Reclaim|Mutation")
    float MutationLoadCost = 14.0f;
};

UCLASS()
class RECLAIM_API UReclaimMutationScanAbility : public UReclaimScanAbility
{
    GENERATED_BODY()

public:
    UReclaimMutationScanAbility();

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Reclaim|Mutation")
    float MutationLoadCost = 12.0f;
};

/**
 * Warden's new-canon Regeneration Pulse.
 *
 * It keeps the old generic area-combat execution family for server-side query/
 * stagger behavior, but adds authoritative ally healing and mutation-load spend.
 */
UCLASS()
class RECLAIM_API UReclaimRegenerationPulseAbility : public UReclaimGenericCombatAbility
{
    GENERATED_BODY()

public:
    UReclaimRegenerationPulseAbility();

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Reclaim|Mutation")
    float MutationLoadCost = 16.0f;

    UPROPERTY(EditDefaultsOnly, Category="Reclaim|Warden")
    float HealAmount = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category="Reclaim|Warden")
    float HealRadius = 800.0f;

private:
    void ApplyAuthoritativeAreaHeal(const FGameplayAbilityActorInfo* ActorInfo) const;
};
