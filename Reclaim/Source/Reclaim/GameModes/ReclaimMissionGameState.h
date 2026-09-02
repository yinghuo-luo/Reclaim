// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimMissionGameState.generated.h"

UCLASS()
class RECLAIM_API AReclaimMissionGameState : public AGameState
{
	GENERATED_BODY()

public:
	AReclaimMissionGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetRunSeed() const { return RunSeed; }
	EReclaimMissionPhase GetMissionPhase() const { return MissionPhase; }
	const FReclaimResourceBundle& GetTeamResources() const { return TeamResources; }

	void SetRunSeed_Server(int32 NewRunSeed);
	void SetMissionPhase_Server(EReclaimMissionPhase NewPhase);
	void AddTeamResources_Server(const FReclaimResourceBundle& Delta);
	bool SpendTeamResources_Server(const FReclaimResourceBundle& Cost);

protected:
	UFUNCTION()
	void OnRep_MissionPhase();

	UFUNCTION()
	void OnRep_TeamResources();

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Mission")
	int32 RunSeed = 1337;

	UPROPERTY(ReplicatedUsing=OnRep_MissionPhase, BlueprintReadOnly, Category="Reclaim|Mission")
	EReclaimMissionPhase MissionPhase = EReclaimMissionPhase::None;

	UPROPERTY(ReplicatedUsing=OnRep_TeamResources, BlueprintReadOnly, Category="Reclaim|Mission")
	FReclaimResourceBundle TeamResources;
};
