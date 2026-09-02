// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rewards/ReclaimRewardService.h"

#include "Core/ReclaimDeterministicRandom.h"
#include "Misc/Crc.h"

TArray<FReclaimRewardCandidate> UReclaimRewardService::GenerateCandidates(int32 RunSeed, int32 PlayerSlot, FName TerminalId, const TArray<FReclaimRewardCandidate>& Pool, int32 CandidateCount)
{
	TArray<FReclaimRewardCandidate> LegalPool;
	for (const FReclaimRewardCandidate& Candidate : Pool)
	{
		if (!Candidate.CandidateId.IsNone() && Candidate.Weight > 0)
		{
			LegalPool.Add(Candidate);
		}
	}

	TArray<FReclaimRewardCandidate> Results;
	if (LegalPool.IsEmpty() || CandidateCount <= 0)
	{
		return Results;
	}

	const int32 TerminalHash = static_cast<int32>(FCrc::StrCrc32(*TerminalId.ToString()));
	FRandomStream Stream = UReclaimDeterministicRandom::MakeStream(RunSeed, EReclaimRandomDomain::RewardTerminal, PlayerSlot, TerminalHash);

	while (Results.Num() < CandidateCount && !LegalPool.IsEmpty())
	{
		int32 TotalWeight = 0;
		for (const FReclaimRewardCandidate& Candidate : LegalPool)
		{
			TotalWeight += FMath::Max(0, Candidate.Weight);
		}

		if (TotalWeight <= 0)
		{
			break;
		}

		int32 Roll = Stream.RandRange(1, TotalWeight);
		for (int32 Index = 0; Index < LegalPool.Num(); ++Index)
		{
			Roll -= FMath::Max(0, LegalPool[Index].Weight);
			if (Roll <= 0)
			{
				Results.Add(LegalPool[Index]);
				LegalPool.RemoveAtSwap(Index);
				break;
			}
		}
	}

	return Results;
}

bool UReclaimRewardService::IsCandidateLegal(FName CandidateId, const TArray<FReclaimRewardCandidate>& Candidates)
{
	for (const FReclaimRewardCandidate& Candidate : Candidates)
	{
		if (Candidate.CandidateId == CandidateId)
		{
			return true;
		}
	}

	return false;
}
