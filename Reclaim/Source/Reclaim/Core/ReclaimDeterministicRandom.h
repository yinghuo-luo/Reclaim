// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReclaimTypes.h"
#include "ReclaimDeterministicRandom.generated.h"

UCLASS()
class RECLAIM_API UReclaimDeterministicRandom : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|Random")
	static int32 MakeSubSeed(int32 RunSeed, EReclaimRandomDomain Domain, int32 StableA = 0, int32 StableB = 0, int32 StableC = 0);

	UFUNCTION(BlueprintPure, Category="Reclaim|Random")
	static FRandomStream MakeStream(int32 RunSeed, EReclaimRandomDomain Domain, int32 StableA = 0, int32 StableB = 0, int32 StableC = 0);
};
