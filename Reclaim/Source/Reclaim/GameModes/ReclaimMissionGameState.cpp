// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/ReclaimMissionGameState.h"

#include "Net/UnrealNetwork.h"

AReclaimMissionGameState::AReclaimMissionGameState()
{
	bReplicates = true;
}

void AReclaimMissionGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimMissionGameState, RunSeed);
	DOREPLIFETIME(AReclaimMissionGameState, MissionPhase);
	DOREPLIFETIME(AReclaimMissionGameState, TeamResources);
}

void AReclaimMissionGameState::SetRunSeed_Server(int32 NewRunSeed)
{
	check(HasAuthority());
	RunSeed = NewRunSeed;
}

void AReclaimMissionGameState::SetMissionPhase_Server(EReclaimMissionPhase NewPhase)
{
	check(HasAuthority());
	MissionPhase = NewPhase;
}

void AReclaimMissionGameState::AddTeamResources_Server(const FReclaimResourceBundle& Delta)
{
	check(HasAuthority());
	TeamResources.AddClamped(Delta);
}

bool AReclaimMissionGameState::SpendTeamResources_Server(const FReclaimResourceBundle& Cost)
{
	check(HasAuthority());
	return TeamResources.SpendIfPossible(Cost);
}

void AReclaimMissionGameState::OnRep_MissionPhase()
{
}

void AReclaimMissionGameState::OnRep_TeamResources()
{
}
