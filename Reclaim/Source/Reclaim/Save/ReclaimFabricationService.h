// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Save/ReclaimFabricationPool.h"
#include "ReclaimFabricationService.generated.h"

UCLASS()
class RECLAIM_API UReclaimFabricationService : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|Fabrication")
	static bool CanSpend(const FReclaimResourceBundle& Resources, const FReclaimResourceBundle& Cost);

	UFUNCTION(BlueprintPure, Category="Reclaim|Fabrication")
	static FName RollFabricationItem(int32 Seed, const TArray<FReclaimFabricationEntry>& Entries);
};
