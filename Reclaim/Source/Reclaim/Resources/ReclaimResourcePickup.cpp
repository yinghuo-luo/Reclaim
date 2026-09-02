// Copyright Epic Games, Inc. All Rights Reserved.

#include "Resources/ReclaimResourcePickup.h"

#include "Components/SphereComponent.h"
#include "GameModes/ReclaimMissionGameState.h"

AReclaimResourcePickup::AReclaimResourcePickup()
{
	bReplicates = true;
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
}

void AReclaimResourcePickup::Collect()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AReclaimMissionGameState* MissionGameState = GetWorld() ? GetWorld()->GetGameState<AReclaimMissionGameState>() : nullptr)
	{
		MissionGameState->AddTeamResources_Server(ResourceValue);
		Destroy();
	}
}
