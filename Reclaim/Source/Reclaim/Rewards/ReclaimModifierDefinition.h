// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReclaimModifierDefinition.generated.h"

UCLASS(BlueprintType)
class RECLAIM_API UReclaimModifierDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Modifier")
	FName ModifierId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Modifier")
	FGameplayTag BuildTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Modifier", meta=(ClampMin="0"))
	int32 Weight = 1;
};
