// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimSaveGame.generated.h"

UCLASS()
class RECLAIM_API UReclaimSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Save")
	int32 SchemaVersion = 1;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Save")
	FReclaimResourceBundle PermanentResources;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Save")
	FReclaimLoadoutSummary NextRunExperimentalLoadout;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Save")
	int32 HostCampaignPurification = 0;
};
