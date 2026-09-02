// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ReclaimAIConfig.h"

UReclaimAIConfig::UReclaimAIConfig()
{
	PlayerScaling.Reset();

	FReclaimDirectorScalingRow OnePlayer;
	OnePlayer.EffectivePlayers = 1;
	OnePlayer.ThreatBudgetScalar = 1.0f;
	OnePlayer.EnemyHealthScalar = 1.0f;
	OnePlayer.SpecialUnitCap = 1;
	PlayerScaling.Add(OnePlayer);

	FReclaimDirectorScalingRow TwoPlayers;
	TwoPlayers.EffectivePlayers = 2;
	TwoPlayers.ThreatBudgetScalar = 1.65f;
	TwoPlayers.EnemyHealthScalar = 1.05f;
	TwoPlayers.SpecialUnitCap = 2;
	PlayerScaling.Add(TwoPlayers);

	FReclaimDirectorScalingRow ThreePlayers;
	ThreePlayers.EffectivePlayers = 3;
	ThreePlayers.ThreatBudgetScalar = 2.25f;
	ThreePlayers.EnemyHealthScalar = 1.10f;
	ThreePlayers.SpecialUnitCap = 2;
	PlayerScaling.Add(ThreePlayers);

	FReclaimDirectorScalingRow FourPlayers;
	FourPlayers.EffectivePlayers = 4;
	FourPlayers.ThreatBudgetScalar = 2.80f;
	FourPlayers.EnemyHealthScalar = 1.15f;
	FourPlayers.SpecialUnitCap = 3;
	PlayerScaling.Add(FourPlayers);
}

FReclaimDirectorScalingResult UReclaimAIConfig::ResolveScaling(int32 EffectivePlayerCount) const
{
	FReclaimDirectorScalingResult Result;
	const int32 ClampedPlayers = FMath::Clamp(EffectivePlayerCount, 1, 4);
	Result.EffectivePlayers = ClampedPlayers;

	const FReclaimDirectorScalingRow* MatchingRow = PlayerScaling.FindByPredicate([ClampedPlayers](const FReclaimDirectorScalingRow& Candidate)
	{
		return Candidate.EffectivePlayers == ClampedPlayers;
	});

	if (MatchingRow)
	{
		Result.ThreatBudgetScalar = FMath::Max(0.0f, MatchingRow->ThreatBudgetScalar);
		Result.EnemyHealthScalar = FMath::Max(0.0f, MatchingRow->EnemyHealthScalar);
		Result.SpecialUnitCap = FMath::Max(0, MatchingRow->SpecialUnitCap);
		return Result;
	}

	Result.ThreatBudgetScalar = 1.0f;
	Result.EnemyHealthScalar = 1.0f;
	Result.SpecialUnitCap = 1;
	return Result;
}
