// Copyright Epic Games, Inc. All Rights Reserved.

#include "Missions/ReclaimMissionController.h"

#include "Net/UnrealNetwork.h"

AReclaimMissionController::AReclaimMissionController()
{
	bReplicates = true;
}

bool AReclaimMissionController::AdvanceObjectiveOnce(FName ObjectiveId)
{
	if (!HasAuthority() || ObjectiveId.IsNone() || CompletedObjectiveIds.Contains(ObjectiveId))
	{
		return false;
	}

	CompletedObjectiveIds.AddUnique(ObjectiveId);
	return true;
}

void AReclaimMissionController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AReclaimMissionController, CompletedObjectiveIds);
}
