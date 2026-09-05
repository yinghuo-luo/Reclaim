#pragma once

#include "CoreMinimal.h"
#include "JsEnv.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ReclaimScriptRuntimeSubsystem.generated.h"

class UReclaimScriptBindingComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FReclaimScriptBindingDelegate,
    UReclaimScriptBindingComponent*, Binding);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FReclaimScriptEventDelegate,
    UReclaimScriptBindingComponent*, Binding,
    FString, EventName,
    UObject*, ContextObject,
    FString, PayloadJson);

/**
 * Owns the single project-level PuerTS VM for one GameInstance.
 *
 * Authority rule:
 * - This subsystem transports script events only.
 * - Replication, RPC validation, damage/GAS execution, inventory/mission state,
 *   and all other authoritative state remain in C++.
 * - Gameplay TS bindings should normally use AuthorityOnly execution policy.
 */
UCLASS()
class RECLAIM_API UReclaimScriptRuntimeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable, Category = "Reclaim|Script")
    FReclaimScriptBindingDelegate OnBindingAdded;

    UPROPERTY(BlueprintAssignable, Category = "Reclaim|Script")
    FReclaimScriptBindingDelegate OnBindingRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Reclaim|Script")
    FReclaimScriptEventDelegate OnScriptEvent;

    UFUNCTION(BlueprintPure, Category = "Reclaim|Script")
    bool IsScriptRuntimeReady() const { return bRuntimeReady; }

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void RegisterBinding(UReclaimScriptBindingComponent* Binding);

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void UnregisterBinding(UReclaimScriptBindingComponent* Binding);

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void DispatchBindingEvent(
        UReclaimScriptBindingComponent* Binding,
        const FString& EventName,
        UObject* ContextObject,
        const FString& PayloadJson);

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void DispatchGlobalEvent(
        const FString& EventName,
        UObject* ContextObject,
        const FString& PayloadJson);

    UFUNCTION(BlueprintPure, Category = "Reclaim|Script")
    TArray<UReclaimScriptBindingComponent*> GetActiveBindings() const;

private:
    void StartScriptRuntime();
    void StopScriptRuntime();

    TSharedPtr<puerts::FJsEnv> JsEnv;
    TSet<TWeakObjectPtr<UReclaimScriptBindingComponent>> ActiveBindings;
    bool bRuntimeReady = false;
};
