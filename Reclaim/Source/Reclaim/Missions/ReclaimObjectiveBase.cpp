// Copyright Epic Games, Inc. All Rights Reserved.

#include "Missions/ReclaimObjectiveBase.h"

#include "Missions/ReclaimMissionController.h"

AReclaimObjectiveBase::AReclaimObjectiveBase()
{
	bReplicates = true;
}

bool AReclaimObjectiveBase::CompleteObjective(AReclaimMissionController* MissionController)
{
	return MissionController && MissionController->AdvanceObjectiveOnce(ObjectiveId);
}
