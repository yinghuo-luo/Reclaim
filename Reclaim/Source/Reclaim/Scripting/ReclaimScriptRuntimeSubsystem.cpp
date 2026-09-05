#include "Scripting/ReclaimScriptRuntimeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Scripting/ReclaimScriptBindingComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogReclaimScript, Log, All);

void UReclaimScriptRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    StartScriptRuntime();
}

void UReclaimScriptRuntimeSubsystem::Deinitialize()
{
    if (bRuntimeReady)
    {
        DispatchGlobalEvent(TEXT("Runtime.Shutdown"), GetGameInstance(), TEXT("{}"));
    }

    StopScriptRuntime();
    ActiveBindings.Reset();

    Super::Deinitialize();
}

void UReclaimScriptRuntimeSubsystem::StartScriptRuntime()
{
    if (JsEnv.IsValid())
    {
        return;
    }

    JsEnv = MakeShared<puerts::FJsEnv>(TEXT("JavaScript"));

    TArray<TPair<FString, UObject*>> Arguments;
    Arguments.Emplace(TEXT("ScriptHost"), this);
    Arguments.Emplace(TEXT("GameInstance"), GetGameInstance());

    UE_LOG(LogReclaimScript, Log, TEXT("Starting Reclaim PuerTS runtime: Reclaim/Bootstrap.js"));
    JsEnv->Start(TEXT("Reclaim/Bootstrap.js"), Arguments);
    bRuntimeReady = true;
}

void UReclaimScriptRuntimeSubsystem::StopScriptRuntime()
{
    bRuntimeReady = false;
    OnBindingAdded.Clear();
    OnBindingRemoved.Clear();
    OnScriptEvent.Clear();
    JsEnv.Reset();
}

void UReclaimScriptRuntimeSubsystem::RegisterBinding(UReclaimScriptBindingComponent* Binding)
{
    if (!IsValid(Binding))
    {
        return;
    }

    const TWeakObjectPtr<UReclaimScriptBindingComponent> WeakBinding(Binding);
    if (ActiveBindings.Contains(WeakBinding))
    {
        return;
    }

    ActiveBindings.Add(WeakBinding);
    if (bRuntimeReady)
    {
        OnBindingAdded.Broadcast(Binding);
    }
}

void UReclaimScriptRuntimeSubsystem::UnregisterBinding(UReclaimScriptBindingComponent* Binding)
{
    if (!IsValid(Binding))
    {
        return;
    }

    const TWeakObjectPtr<UReclaimScriptBindingComponent> WeakBinding(Binding);
    if (!ActiveBindings.Contains(WeakBinding))
    {
        return;
    }

    if (bRuntimeReady)
    {
        OnBindingRemoved.Broadcast(Binding);
    }

    ActiveBindings.Remove(WeakBinding);
}

void UReclaimScriptRuntimeSubsystem::DispatchBindingEvent(
    UReclaimScriptBindingComponent* Binding,
    const FString& EventName,
    UObject* ContextObject,
    const FString& PayloadJson)
{
    if (!bRuntimeReady || EventName.IsEmpty())
    {
        return;
    }

    OnScriptEvent.Broadcast(Binding, EventName, ContextObject, PayloadJson);
}

void UReclaimScriptRuntimeSubsystem::DispatchGlobalEvent(
    const FString& EventName,
    UObject* ContextObject,
    const FString& PayloadJson)
{
    DispatchBindingEvent(nullptr, EventName, ContextObject, PayloadJson);
}

TArray<UReclaimScriptBindingComponent*> UReclaimScriptRuntimeSubsystem::GetActiveBindings() const
{
    TArray<UReclaimScriptBindingComponent*> Result;
    Result.Reserve(ActiveBindings.Num());

    for (const TWeakObjectPtr<UReclaimScriptBindingComponent>& WeakBinding : ActiveBindings)
    {
        if (UReclaimScriptBindingComponent* Binding = WeakBinding.Get())
        {
            Result.Add(Binding);
        }
    }

    return Result;
}
