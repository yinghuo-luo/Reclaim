// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimFabricationPool.generated.h"

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimFabricationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FReclaimResourceBundle Cost;
};

UCLASS(BlueprintType)
class RECLAIM_API UReclaimFabricationPool : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Fabrication")
	TArray<FReclaimFabricationEntry> Entries;
};
