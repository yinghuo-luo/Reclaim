// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReclaimMissionDefinition.generated.h"

UCLASS(BlueprintType)
class RECLAIM_API UReclaimMissionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission")
	FName MissionId = TEXT("ClearInfectedNest");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission", meta=(ClampMin="0"))
	int32 InfectionNodeCount = 3;
};
