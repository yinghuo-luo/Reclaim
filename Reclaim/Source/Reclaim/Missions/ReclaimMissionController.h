// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimMissionController.generated.h"

UCLASS()
class RECLAIM_API AReclaimMissionController : public AActor
{
	GENERATED_BODY()

public:
	AReclaimMissionController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Mission")
	bool AdvanceObjectiveOnce(FName ObjectiveId);

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Mission")
	TArray<FName> CompletedObjectiveIds;
};
