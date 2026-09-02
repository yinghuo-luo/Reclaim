// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimRoleDefinition.generated.h"

class AReclaimPlayerCharacter;
class UReclaimAbilityDefinition;

UCLASS(BlueprintType)
class RECLAIM_API UReclaimRoleDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|Role")
	UReclaimAbilityDefinition* GetCoreAbilityBySlot(int32 AbilitySlot) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Role")
	EReclaimRole Role = EReclaimRole::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Role")
	FGameplayTag RoleTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Role")
	TSubclassOf<AReclaimPlayerCharacter> PawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Role")
	TArray<TObjectPtr<UReclaimAbilityDefinition>> CoreAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Role")
	int32 SortOrder = 0;
};
