// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rewards/ReclaimRewardTerminal.h"

#include "Components/ReclaimRunInventoryComponent.h"
#include "Player/ReclaimPlayerState.h"
#include "Rewards/ReclaimRewardService.h"

AReclaimRewardTerminal::AReclaimRewardTerminal()
{
	bReplicates = true;
}

TArray<FReclaimRewardCandidate> AReclaimRewardTerminal::GenerateCandidatesForPlayer(AReclaimPlayerState* PlayerState, int32 RunSeed)
{
	if (!HasAuthority() || !PlayerState)
	{
		return TArray<FReclaimRewardCandidate>();
	}

	TArray<FReclaimRewardCandidate> Candidates = UReclaimRewardService::GenerateCandidates(RunSeed, PlayerState->GetPlayerSlot(), TerminalId, CandidatePool);
	const TWeakObjectPtr<AReclaimPlayerState> PlayerKey(PlayerState);
	ServerCandidatesByPlayer.FindOrAdd(PlayerKey) = Candidates;

	if (UReclaimRunInventoryComponent* RunInventory = PlayerState->GetRunInventoryComponent())
	{
		RunInventory->SetPendingRewardCandidates_Server(Candidates);
	}

	return Candidates;
}

bool AReclaimRewardTerminal::ConfirmCandidateForPlayer(AReclaimPlayerState* PlayerState, FName CandidateId)
{
	if (!HasAuthority() || !PlayerState)
	{
		return false;
	}

	const TWeakObjectPtr<AReclaimPlayerState> PlayerKey(PlayerState);
	const TArray<FReclaimRewardCandidate>* Candidates = ServerCandidatesByPlayer.Find(PlayerKey);
	if (!Candidates || !UReclaimRewardService::IsCandidateLegal(CandidateId, *Candidates))
	{
		return false;
	}

	UReclaimRunInventoryComponent* RunInventory = PlayerState->GetRunInventoryComponent();
	if (!RunInventory)
	{
		return false;
	}

	RunInventory->AddModifier_Server(CandidateId);
	ServerCandidatesByPlayer.Remove(PlayerKey);
	RunInventory->SetPendingRewardCandidates_Server(TArray<FReclaimRewardCandidate>());
	return true;
}
