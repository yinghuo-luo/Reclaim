// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReclaimDeviceDefinition.generated.h"

class AReclaimDeployableBase;

UCLASS(BlueprintType)
class RECLAIM_API UReclaimDeviceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Device")
	FName DeviceId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Device")
	FGameplayTagContainer DeviceTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Device")
	TSubclassOf<AReclaimDeployableBase> DeployableClass;
};
