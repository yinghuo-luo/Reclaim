// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ReclaimSaveSubsystem.generated.h"

class UReclaimSaveGame;

UCLASS()
class RECLAIM_API UReclaimSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Reclaim|Save")
	UReclaimSaveGame* LoadOrCreateLocalSave();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Save")
	bool SaveLocal();

	UFUNCTION(BlueprintPure, Category="Reclaim|Save")
	UReclaimSaveGame* GetCurrentSave() const { return CurrentSave; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<UReclaimSaveGame> CurrentSave = nullptr;

	UPROPERTY()
	FString SlotName = TEXT("ReclaimLocalSave");
};
