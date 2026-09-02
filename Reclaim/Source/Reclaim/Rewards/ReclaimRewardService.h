// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimRewardService.generated.h"

UCLASS()
class RECLAIM_API UReclaimRewardService : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|Rewards")
	static TArray<FReclaimRewardCandidate> GenerateCandidates(int32 RunSeed, int32 PlayerSlot, FName TerminalId, const TArray<FReclaimRewardCandidate>& Pool, int32 CandidateCount = 3);

	UFUNCTION(BlueprintPure, Category="Reclaim|Rewards")
	static bool IsCandidateLegal(FName CandidateId, const TArray<FReclaimRewardCandidate>& Candidates);
};
