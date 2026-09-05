#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReclaimScriptBlueprintLibrary.generated.h"

class UReclaimScriptRuntimeSubsystem;

UCLASS()
class RECLAIM_API UReclaimScriptBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Reclaim|Script", meta = (WorldContext = "WorldContextObject"))
    static UReclaimScriptRuntimeSubsystem* GetReclaimScriptRuntime(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script", meta = (WorldContext = "WorldContextObject"))
    static void DispatchReclaimGlobalScriptEvent(
        const UObject* WorldContextObject,
        const FString& EventName,
        UObject* ContextObject,
        const FString& PayloadJson);
};
