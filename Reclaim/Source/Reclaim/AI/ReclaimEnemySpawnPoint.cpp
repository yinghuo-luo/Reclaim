// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ReclaimEnemySpawnPoint.h"

#include "Core/ReclaimCooperationRules.h"
#include "GameFramework/GameStateBase.h"
#include "NavigationSystem.h"
#include "Player/ReclaimPlayerState.h"

AReclaimEnemySpawnPoint::AReclaimEnemySpawnPoint()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
}

bool AReclaimEnemySpawnPoint::IsSpawnPointValid_Server(const UReclaimAIConfig* Config) const
{
	return PassesHardSpawnRules_Server(Config);
}

bool AReclaimEnemySpawnPoint::PassesHardSpawnRules_Server(const UReclaimAIConfig* Config) const
{
	return HasAuthority()
		&& bEnabled
		&& PassesDistanceRule_Server(Config)
		&& PassesVisibilityRule_Server(Config)
		&& PassesNavigationRule_Server(Config);
}

float AReclaimEnemySpawnPoint::ComputeSpawnPreferenceScore_Server(const UReclaimAIConfig* Config) const
{
	if (!HasAuthority())
	{
		return 0.0f;
	}

	const EReclaimSpawnVisibilityRule Rule = ResolveVisibilityRule(Config);
	if (Rule == EReclaimSpawnVisibilityRule::PreferOccluded)
	{
		return IsOccludedFromActivePlayers_Server(Config) ? 3.0f : 1.0f;
	}

	return 1.0f;
}

bool AReclaimEnemySpawnPoint::IsOccludedFromActivePlayers_Server(const UReclaimAIConfig* Config) const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!World || !GameState)
	{
		return false;
	}

	const ECollisionChannel VisibilityChannel = Config ? Config->LineOfSightChannel.GetValue() : ECC_Visibility;
	for (APlayerState* CandidatePlayerState : GameState->PlayerArray)
	{
		const AReclaimPlayerState* ReclaimPlayerState = Cast<AReclaimPlayerState>(CandidatePlayerState);
		const APawn* Pawn = ReclaimPlayerState ? Cast<APawn>(ReclaimPlayerState->GetPawn()) : nullptr;
		if (!Pawn || !UReclaimCooperationRules::IsCombatCapable(ReclaimPlayerState->GetLifeState()))
		{
			continue;
		}

		FVector ViewLocation = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
		FRotator ViewRotation = FRotator::ZeroRotator;
		Pawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);

		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimSpawnVisibility), true, this);
		QueryParams.AddIgnoredActor(Pawn);
		QueryParams.AddIgnoredActor(this);
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, ViewLocation, GetActorLocation(), VisibilityChannel, QueryParams);
		if (!bBlocked)
		{
			return false;
		}
	}

	return true;
}

FName AReclaimEnemySpawnPoint::GetStableSpawnId() const
{
	return StableSpawnId.IsNone() ? GetFName() : StableSpawnId;
}

bool AReclaimEnemySpawnPoint::PassesDistanceRule_Server(const UReclaimAIConfig* Config) const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return false;
	}

	const float RequiredDistance = bOverrideMinimumDistance
		? MinimumDistanceFromActivePlayer
		: (Config ? Config->MinimumSpawnDistanceFromActivePlayer : 1200.0f);

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AReclaimPlayerState* ReclaimPlayerState = Cast<AReclaimPlayerState>(PlayerState);
		const APawn* Pawn = ReclaimPlayerState ? Cast<APawn>(ReclaimPlayerState->GetPawn()) : nullptr;
		if (!Pawn || !UReclaimCooperationRules::IsCombatCapable(ReclaimPlayerState->GetLifeState()))
		{
			continue;
		}

		if (RequiredDistance > 0.0f && FVector::DistSquared(GetActorLocation(), Pawn->GetActorLocation()) < FMath::Square(RequiredDistance))
		{
			return false;
		}
	}

	return true;
}

bool AReclaimEnemySpawnPoint::PassesVisibilityRule_Server(const UReclaimAIConfig* Config) const
{
	const EReclaimSpawnVisibilityRule Rule = ResolveVisibilityRule(Config);
	if (Rule != EReclaimSpawnVisibilityRule::RequireOccluded)
	{
		return true;
	}

	return IsOccludedFromActivePlayers_Server(Config);
}

bool AReclaimEnemySpawnPoint::PassesNavigationRule_Server(const UReclaimAIConfig* Config) const
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!NavigationSystem)
	{
		return false;
	}

	const float Extent = Config ? FMath::Max(0.0f, Config->SpawnNavProjectionExtent) : 300.0f;
	FNavLocation ProjectedLocation;
	return NavigationSystem->ProjectPointToNavigation(GetActorLocation(), ProjectedLocation, FVector(Extent, Extent, Extent));
}

EReclaimSpawnVisibilityRule AReclaimEnemySpawnPoint::ResolveVisibilityRule(const UReclaimAIConfig* Config) const
{
	return bOverrideVisibilityRule
		? VisibilityRuleOverride
		: (Config ? Config->SpawnVisibilityRule : EReclaimSpawnVisibilityRule::RequireOccluded);
}
