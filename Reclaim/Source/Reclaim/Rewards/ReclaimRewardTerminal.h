// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimRewardTerminal.generated.h"

class AReclaimPlayerState;
class UReclaimRunInventoryComponent;

UCLASS()
class RECLAIM_API AReclaimRewardTerminal : public AActor
{
	GENERATED_BODY()

public:
	AReclaimRewardTerminal();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Rewards")
	TArray<FReclaimRewardCandidate> GenerateCandidatesForPlayer(AReclaimPlayerState* PlayerState, int32 RunSeed);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Rewards")
	bool ConfirmCandidateForPlayer(AReclaimPlayerState* PlayerState, FName CandidateId);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Rewards")
	FName TerminalId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Rewards")
	TArray<FReclaimRewardCandidate> CandidatePool;

	TMap<TWeakObjectPtr<AReclaimPlayerState>, TArray<FReclaimRewardCandidate>> ServerCandidatesByPlayer;
};
