#include "Mutation/ReclaimMutationLoadComponent.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UReclaimMutationLoadComponent::UReclaimMutationLoadComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UReclaimMutationLoadComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        CurrentMutationLoad = FMath::Clamp(CurrentMutationLoad, 0.0f, MaxMutationLoad);
        RefreshOverloadState();
        StartRecoveryTimer();
    }
}

void UReclaimMutationLoadComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UReclaimMutationLoadComponent, CurrentMutationLoad);
    DOREPLIFETIME(UReclaimMutationLoadComponent, bOverloaded);
}

float UReclaimMutationLoadComponent::GetMutationLoadNormalized() const
{
    return MaxMutationLoad > KINDA_SMALL_NUMBER
        ? FMath::Clamp(CurrentMutationLoad / MaxMutationLoad, 0.0f, 1.0f)
        : 0.0f;
}

bool UReclaimMutationLoadComponent::CanActivateMutationAbility() const
{
    return !bOverloaded && CurrentMutationLoad < MaxMutationLoad - KINDA_SMALL_NUMBER;
}

bool UReclaimMutationLoadComponent::TryConsumeAbilityLoad(float Cost)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || Cost < 0.0f || !CanActivateMutationAbility())
    {
        return false;
    }

    float EffectiveCost = Cost;
    if (HasRuntimeTag(TEXT("State.StabilityField")))
    {
        EffectiveCost *= StabilizerFieldCostMultiplier;
    }

    CurrentMutationLoad = FMath::Clamp(CurrentMutationLoad + EffectiveCost, 0.0f, MaxMutationLoad);
    LastAbilityUseServerTime = Owner->GetWorld() ? Owner->GetWorld()->GetTimeSeconds() : 0.0;
    RefreshOverloadState();
    BroadcastCurrentState();
    StartRecoveryTimer();
    return true;
}

void UReclaimMutationLoadComponent::ReduceMutationLoad(float Amount)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || Amount <= 0.0f)
    {
        return;
    }

    CurrentMutationLoad = FMath::Clamp(CurrentMutationLoad - Amount, 0.0f, MaxMutationLoad);
    RefreshOverloadState();
    BroadcastCurrentState();
}

void UReclaimMutationLoadComponent::ResetMutationLoad(float NewValue)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        return;
    }

    CurrentMutationLoad = FMath::Clamp(NewValue, 0.0f, MaxMutationLoad);
    LastAbilityUseServerTime = -DBL_MAX;
    RefreshOverloadState();
    BroadcastCurrentState();
    StartRecoveryTimer();
}

void UReclaimMutationLoadComponent::OnRep_MutationLoad()
{
    BroadcastCurrentState();
}

void UReclaimMutationLoadComponent::OnRep_Overloaded()
{
    BroadcastCurrentState();
}

void UReclaimMutationLoadComponent::RecoveryTick()
{
    AActor* Owner = GetOwner();
    UWorld* World = Owner ? Owner->GetWorld() : nullptr;
    if (!Owner || !World || !Owner->HasAuthority())
    {
        return;
    }

    const double Now = World->GetTimeSeconds();
    if (Now - LastAbilityUseServerTime < RecoveryDelayAfterAbility)
    {
        return;
    }

    const bool bInsideStabilizerField = HasRuntimeTag(TEXT("State.StabilityField"));
    float EffectiveRecovery = RecoveryPerSecond * RecoveryTickInterval;
    if (bOverloaded)
    {
        EffectiveRecovery *= OverloadRecoveryMultiplier;
    }
    if (bInsideStabilizerField)
    {
        EffectiveRecovery *= StabilizerFieldRecoveryMultiplier;
    }

    const float Before = CurrentMutationLoad;
    CurrentMutationLoad = FMath::Clamp(CurrentMutationLoad - EffectiveRecovery, 0.0f, MaxMutationLoad);

    if (bInsideStabilizerField)
    {
        RecoverProtection(RecoveryTickInterval);
    }

    RefreshOverloadState();

    if (!FMath::IsNearlyEqual(Before, CurrentMutationLoad))
    {
        BroadcastCurrentState();
    }

    StopRecoveryTimerIfIdle();
}

void UReclaimMutationLoadComponent::StartRecoveryTimer()
{
    AActor* Owner = GetOwner();
    UWorld* World = Owner ? Owner->GetWorld() : nullptr;
    if (!Owner || !World || !Owner->HasAuthority())
    {
        return;
    }

    if (!World->GetTimerManager().IsTimerActive(RecoveryTimerHandle))
    {
        World->GetTimerManager().SetTimer(
            RecoveryTimerHandle,
            this,
            &UReclaimMutationLoadComponent::RecoveryTick,
            RecoveryTickInterval,
            true);
    }
}

void UReclaimMutationLoadComponent::StopRecoveryTimerIfIdle()
{
    AActor* Owner = GetOwner();
    UWorld* World = Owner ? Owner->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    // Keep the low-frequency timer alive while the stabilizer field can provide
    // Protection recovery. Otherwise, stop it at zero load to avoid idle work.
    if (CurrentMutationLoad <= KINDA_SMALL_NUMBER && !HasRuntimeTag(TEXT("State.StabilityField")))
    {
        World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
    }
}

void UReclaimMutationLoadComponent::RefreshOverloadState()
{
    if (CurrentMutationLoad >= MaxMutationLoad - KINDA_SMALL_NUMBER)
    {
        bOverloaded = true;
    }
    else if (bOverloaded && CurrentMutationLoad <= OverloadUnlockThreshold)
    {
        bOverloaded = false;
    }
}

bool UReclaimMutationLoadComponent::HasRuntimeTag(FName TagName) const
{
    const AActor* Owner = GetOwner();
    const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Owner);
    UAbilitySystemComponent* ASC = AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
    if (!ASC)
    {
        return false;
    }

    const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
    return Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag);
}

void UReclaimMutationLoadComponent::RecoverProtection(float DeltaSeconds)
{
    AActor* Owner = GetOwner();
    IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Owner);
    UAbilitySystemComponent* ASC = AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
    if (!ASC)
    {
        return;
    }

    FGameplayAttribute ProtectionAttribute;
    FGameplayAttribute MaxProtectionAttribute;

    for (UAttributeSet* Set : ASC->GetSpawnedAttributes())
    {
        if (!Set)
        {
            continue;
        }

        FProperty* ProtectionProp = Set->GetClass()->FindPropertyByName(TEXT("Shield"));
        FProperty* MaxProtectionProp = Set->GetClass()->FindPropertyByName(TEXT("MaxShield"));
        if (!ProtectionProp || !MaxProtectionProp)
        {
            continue;
        }

        ProtectionAttribute = FGameplayAttribute(ProtectionProp);
        MaxProtectionAttribute = FGameplayAttribute(MaxProtectionProp);
        break;
    }

    if (!ProtectionAttribute.IsValid() || !MaxProtectionAttribute.IsValid())
    {
        return;
    }

    const float Current = ASC->GetNumericAttribute(ProtectionAttribute);
    const float Maximum = ASC->GetNumericAttribute(MaxProtectionAttribute);
    if (Current >= Maximum - KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float Delta = StabilizerProtectionRecoveryPerSecond * DeltaSeconds;
    ASC->ApplyModToAttribute(ProtectionAttribute, EGameplayModOp::Additive, FMath::Min(Delta, Maximum - Current));
}

void UReclaimMutationLoadComponent::BroadcastCurrentState()
{
    OnMutationLoadChanged.Broadcast(CurrentMutationLoad, bOverloaded);
}
