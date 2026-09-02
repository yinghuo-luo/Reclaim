// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReclaimPythonAssetSetupLibrary.generated.h"

UCLASS()
class RECLAIM_API UReclaimPythonAssetSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Reclaim|Editor")
	static FGameplayTag MakeGameplayTagForAssetSetup(FName TagName, bool bErrorIfNotFound = false);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Editor")
	static FGameplayTagContainer MakeGameplayTagContainerForAssetSetup(const TArray<FName>& TagNames, bool bErrorIfNotFound = false);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Editor")
	static bool EnsureWidgetBlueprintControls(UObject* WidgetBlueprintAsset, const TArray<FName>& ButtonNames, const TArray<FName>& TextBlockNames, const TArray<FName>& ProgressBarNames);
};
