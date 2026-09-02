// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReclaimObjectiveBase.generated.h"

class AReclaimMissionController;

UCLASS(Abstract)
class RECLAIM_API AReclaimObjectiveBase : public AActor
{
	GENERATED_BODY()

public:
	AReclaimObjectiveBase();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Objective")
	bool CompleteObjective(AReclaimMissionController* MissionController);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Objective")
	FName ObjectiveId = NAME_None;
};
