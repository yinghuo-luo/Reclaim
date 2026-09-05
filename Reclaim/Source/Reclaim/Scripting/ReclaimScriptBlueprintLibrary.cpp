#include "Scripting/ReclaimScriptBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Scripting/ReclaimScriptRuntimeSubsystem.h"

UReclaimScriptRuntimeSubsystem* UReclaimScriptBlueprintLibrary::GetReclaimScriptRuntime(
    const UObject* WorldContextObject)
{
    if (!GEngine || !WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UReclaimScriptRuntimeSubsystem>() : nullptr;
}

void UReclaimScriptBlueprintLibrary::DispatchReclaimGlobalScriptEvent(
    const UObject* WorldContextObject,
    const FString& EventName,
    UObject* ContextObject,
    const FString& PayloadJson)
{
    if (UReclaimScriptRuntimeSubsystem* Runtime = GetReclaimScriptRuntime(WorldContextObject))
    {
        Runtime->DispatchGlobalEvent(EventName, ContextObject, PayloadJson);
    }
}
