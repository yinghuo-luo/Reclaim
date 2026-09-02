// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReclaimAnimConfig.generated.h"

UCLASS(BlueprintType)
class RECLAIM_API UReclaimAnimConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation", meta=(ClampMin="0"))
	float StopSpeedThreshold = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation", meta=(ClampMin="0"))
	float TurnInPlaceYawThreshold = 90.0f;
};
