#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReclaimM4CanonLibrary.generated.h"

/**
 * Small editor/runtime-safe reflection helper used by the M4 Python setup.
 *
 * UE5.6 Python cannot reliably assign EditDefaultsOnly fields on temporary
 * FReclaimDirectorScalingRow structs. This helper writes the existing AI config
 * array without introducing a second Director authority implementation.
 */
UCLASS()
class RECLAIM_API UReclaimM4CanonLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Reclaim|M4 Canon")
    static bool ApplyM4DirectorScaling(UObject* AIConfigObject);
};
