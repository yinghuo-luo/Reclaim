#include "AbilitySystem/Abilities/ReclaimMutationAbilities.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagContainer.h"
#include "Engine/OverlapResult.h"
#include "Mutation/ReclaimMutationLoadComponent.h"

namespace ReclaimMutationAbilityPrivate
{
    static UReclaimMutationLoadComponent* FindMutationLoad(const FGameplayAbilityActorInfo* ActorInfo)
    {
        AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
        return Avatar ? Avatar->FindComponentByClass<UReclaimMutationLoadComponent>() : nullptr;
    }

    static bool CanUse(const FGameplayAbilityActorInfo* ActorInfo)
    {
        UReclaimMutationLoadComponent* Mutation = FindMutationLoad(ActorInfo);
        // During migration/editor validation a missing component must not crash the
        // existing M3 ability path. The setup validator treats absence as a hard error.
        return !Mutation || Mutation->CanActivateMutationAbility();
    }

    static bool Spend(const FGameplayAbilityActorInfo* ActorInfo, float Cost)
    {
        UReclaimMutationLoadComponent* Mutation = FindMutationLoad(ActorInfo);
        if (!Mutation)
        {
            return true;
        }

        AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
        if (!Avatar)
        {
            return false;
        }

        // Local-predicted/client execution may play cosmetics, but only the server
        // mutates the replicated Mutation Load. The authoritative activation will
        // run the same check and can still reject the request.
        if (!Avatar->HasAuthority())
        {
            return true;
        }

        return Mutation->TryConsumeAbilityLoad(Cost);
    }

    template <typename TAbility>
    static bool BaseCanActivate(
        const TAbility* Ability,
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags,
        const FGameplayTagContainer* TargetTags,
        FGameplayTagContainer* OptionalRelevantTags)
    {
        return Ability && ReclaimMutationAbilityPrivate::CanUse(ActorInfo);
    }

    static FGameplayAttribute FindAttribute(UAbilitySystemComponent* ASC, FName PropertyName)
    {
        if (!ASC)
        {
            return FGameplayAttribute();
        }

        for (UAttributeSet* Set : ASC->GetSpawnedAttributes())
        {
            if (!Set)
            {
                continue;
            }

            if (FProperty* Property = Set->GetClass()->FindPropertyByName(PropertyName))
            {
                return FGameplayAttribute(Property);
            }
        }

        return FGameplayAttribute();
    }
}

UReclaimMutationMovementAbility::UReclaimMutationMovementAbility() = default;

bool UReclaimMutationMovementAbility::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
        && ReclaimMutationAbilityPrivate::CanUse(ActorInfo);
}

void UReclaimMutationMovementAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!ReclaimMutationAbilityPrivate::Spend(ActorInfo, MutationLoadCost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

UReclaimMutationDeployAbility::UReclaimMutationDeployAbility() = default;

bool UReclaimMutationDeployAbility::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
        && ReclaimMutationAbilityPrivate::CanUse(ActorInfo);
}

void UReclaimMutationDeployAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!ReclaimMutationAbilityPrivate::Spend(ActorInfo, MutationLoadCost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

UReclaimMutationCombatAbility::UReclaimMutationCombatAbility() = default;

bool UReclaimMutationCombatAbility::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
        && ReclaimMutationAbilityPrivate::CanUse(ActorInfo);
}

void UReclaimMutationCombatAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!ReclaimMutationAbilityPrivate::Spend(ActorInfo, MutationLoadCost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

UReclaimMutationScanAbility::UReclaimMutationScanAbility() = default;

bool UReclaimMutationScanAbility::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
        && ReclaimMutationAbilityPrivate::CanUse(ActorInfo);
}

void UReclaimMutationScanAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!ReclaimMutationAbilityPrivate::Spend(ActorInfo, MutationLoadCost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

UReclaimRegenerationPulseAbility::UReclaimRegenerationPulseAbility() = default;

bool UReclaimRegenerationPulseAbility::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
        && ReclaimMutationAbilityPrivate::CanUse(ActorInfo);
}

void UReclaimRegenerationPulseAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!ReclaimMutationAbilityPrivate::Spend(ActorInfo, MutationLoadCost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ApplyAuthoritativeAreaHeal(ActorInfo);
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UReclaimRegenerationPulseAbility::ApplyAuthoritativeAreaHeal(const FGameplayAbilityActorInfo* ActorInfo) const
{
    AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
    UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
    if (!Avatar || !World || !Avatar->HasAuthority())
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimRegenerationPulse), false, Avatar);
    const FCollisionShape Shape = FCollisionShape::MakeSphere(HealRadius);

    World->OverlapMultiByObjectType(
        Overlaps,
        Avatar->GetActorLocation(),
        FQuat::Identity,
        ObjectQuery,
        Shape,
        QueryParams);

    // Include self even if an unusual collision profile excluded the avatar.
    TSet<AActor*> Candidates;
    Candidates.Add(Avatar);
    for (const FOverlapResult& Result : Overlaps)
    {
        if (AActor* Target = Result.GetActor())
        {
            Candidates.Add(Target);
        }
    }

    for (AActor* Target : Candidates)
    {
        APawn* TargetPawn = Cast<APawn>(Target);
        if (!TargetPawn)
        {
            continue;
        }

        // In Demo co-op, player pawns are friendly. Reject AI/other non-player pawns
        // by requiring a PlayerState or local player controller ownership path.
        if (!TargetPawn->GetPlayerState() && Target != Avatar)
        {
            continue;
        }

        IAbilitySystemInterface* AbilityTarget = Cast<IAbilitySystemInterface>(Target);
        UAbilitySystemComponent* ASC = AbilityTarget ? AbilityTarget->GetAbilitySystemComponent() : nullptr;
        if (!ASC)
        {
            continue;
        }

        const FGameplayTag DownedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Downed"), false);
        if (DownedTag.IsValid() && ASC->HasMatchingGameplayTag(DownedTag))
        {
            // Downed/Revive remains owned by the existing cooperation state machine.
            // Regeneration Pulse must not bypass that flow by restoring Health directly.
            continue;
        }

        const FGameplayAttribute Health = ReclaimMutationAbilityPrivate::FindAttribute(ASC, TEXT("Health"));
        const FGameplayAttribute MaxHealth = ReclaimMutationAbilityPrivate::FindAttribute(ASC, TEXT("MaxHealth"));
        if (!Health.IsValid() || !MaxHealth.IsValid())
        {
            continue;
        }

        const float Current = ASC->GetNumericAttribute(Health);
        const float Maximum = ASC->GetNumericAttribute(MaxHealth);
        if (Current >= Maximum - KINDA_SMALL_NUMBER)
        {
            continue;
        }

        ASC->ApplyModToAttribute(Health, EGameplayModOp::Additive, FMath::Min(HealAmount, Maximum - Current));
    }
}
