// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ReclaimAIConfig.h"
#include "AI/ReclaimEnemyActionRules.h"
#include "AI/ReclaimEnemyCombatBrain.h"
#include "AI/ReclaimEnemyDefinition.h"
#include "AI/ReclaimEnemyDirector.h"
#include "Core/ReclaimGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM4ThreatScalingTest, "Reclaim.M4AI.ThreatScalingBaseline", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM4ThreatScalingTest::RunTest(const FString& Parameters)
{
	const UReclaimAIConfig* Config = GetDefault<UReclaimAIConfig>();

	const FReclaimDirectorScalingResult OnePlayer = AReclaimEnemyDirector::ResolveScaling(Config, 1);
	TestEqual(TEXT("1P threat scalar"), OnePlayer.ThreatBudgetScalar, 1.00f);
	TestEqual(TEXT("1P health scalar"), OnePlayer.EnemyHealthScalar, 1.00f);
	TestEqual(TEXT("1P special cap"), OnePlayer.SpecialUnitCap, 1);

	const FReclaimDirectorScalingResult TwoPlayers = AReclaimEnemyDirector::ResolveScaling(Config, 2);
	TestEqual(TEXT("2P threat scalar"), TwoPlayers.ThreatBudgetScalar, 1.65f);
	TestEqual(TEXT("2P health scalar"), TwoPlayers.EnemyHealthScalar, 1.05f);
	TestEqual(TEXT("2P special cap"), TwoPlayers.SpecialUnitCap, 2);

	const FReclaimDirectorScalingResult ThreePlayers = AReclaimEnemyDirector::ResolveScaling(Config, 3);
	TestEqual(TEXT("3P threat scalar"), ThreePlayers.ThreatBudgetScalar, 2.25f);
	TestEqual(TEXT("3P health scalar"), ThreePlayers.EnemyHealthScalar, 1.10f);
	TestEqual(TEXT("3P special cap"), ThreePlayers.SpecialUnitCap, 2);

	const FReclaimDirectorScalingResult FourPlayers = AReclaimEnemyDirector::ResolveScaling(Config, 4);
	TestEqual(TEXT("4P threat scalar"), FourPlayers.ThreatBudgetScalar, 2.80f);
	TestEqual(TEXT("4P health scalar"), FourPlayers.EnemyHealthScalar, 1.15f);
	TestEqual(TEXT("4P special cap"), FourPlayers.SpecialUnitCap, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM4BudgetSolverTest, "Reclaim.M4AI.BudgetSolverLegalDeterministic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM4BudgetSolverTest::RunTest(const FString& Parameters)
{
	TArray<FReclaimEnemyBudgetEntry> Entries;

	FReclaimEnemyBudgetEntry Crawler;
	Crawler.EnemyId = TEXT("Crawler");
	Crawler.ThreatCost = 1;
	Crawler.SpawnWeight = 4;
	Crawler.bSpecialUnit = false;
	Entries.Add(Crawler);

	FReclaimEnemyBudgetEntry Spitter;
	Spitter.EnemyId = TEXT("Spitter");
	Spitter.ThreatCost = 2;
	Spitter.SpawnWeight = 3;
	Spitter.bSpecialUnit = false;
	Entries.Add(Spitter);

	FReclaimEnemyBudgetEntry Brute;
	Brute.EnemyId = TEXT("Brute");
	Brute.ThreatCost = 5;
	Brute.SpawnWeight = 2;
	Brute.bSpecialUnit = true;
	Entries.Add(Brute);

	FReclaimEnemyBudgetEntry Elite;
	Elite.EnemyId = TEXT("Elite");
	Elite.ThreatCost = 7;
	Elite.SpawnWeight = 1;
	Elite.bSpecialUnit = true;
	Entries.Add(Elite);

	const FReclaimThreatBudgetSolution First = AReclaimEnemyDirector::SolveThreatBudget(12, 1, 9001, Entries);
	const FReclaimThreatBudgetSolution Second = AReclaimEnemyDirector::SolveThreatBudget(12, 1, 9001, Entries);

	TestTrue(TEXT("Solver generated at least one legal request"), First.Requests.Num() > 0);
	TestTrue(TEXT("Solver never exceeds budget"), First.ThreatSpent <= First.Budget);
	TestTrue(TEXT("Solver respects special cap"), First.SpecialUnits <= 1);
	TestEqual(TEXT("Same seed reproduces request count"), First.Requests.Num(), Second.Requests.Num());
	TestEqual(TEXT("Same seed reproduces spent threat"), First.ThreatSpent, Second.ThreatSpent);
	TestEqual(TEXT("Same seed reproduces composition summary"), First.CompositionSummary, Second.CompositionSummary);

	for (const FReclaimEnemySpawnRequest& Request : First.Requests)
	{
		TestTrue(TEXT("Each request has valid threat cost"), Request.ThreatCost > 0);
		TestTrue(TEXT("Each request fits budget"), Request.ThreatCost <= First.Budget);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM4CombatBrainTest, "Reclaim.M4AI.CombatBrainFixedFacts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM4CombatBrainTest::RunTest(const FString& Parameters)
{
	UReclaimEnemyCombatBrain* Brain = NewObject<UReclaimEnemyCombatBrain>();
	UReclaimAIConfig* Config = NewObject<UReclaimAIConfig>();

	UReclaimEnemyDefinition* Crawler = NewObject<UReclaimEnemyDefinition>();
	Crawler->EnemyId = TEXT("Crawler");
	Crawler->EnemyTags.AddTag(ReclaimGameplayTags::Enemy_Crawler);
	Crawler->AttackDistance = 150.0f;
	Crawler->PreferredDistance = 175.0f;
	Crawler->bCanMeleeAttack = true;
	Crawler->bCanLeapAttack = true;
	Crawler->LeapMinDistance = 250.0f;
	Crawler->LeapMaxDistance = 800.0f;

	FReclaimEnemyFacts LeapFacts;
	LeapFacts.bHasValidTarget = true;
	LeapFacts.TargetLifeState = EReclaimPlayerLifeState::Active;
	LeapFacts.DistanceToTarget = 500.0f;
	LeapFacts.bHasLineOfSight = true;
	LeapFacts.bLeapReady = true;
	LeapFacts.bMeleeReady = true;
	TestEqual(TEXT("Crawler pounces in leap range"), Brain->DecideIntentForEnemy(LeapFacts, Crawler, Config).Type, EReclaimEnemyIntentType::LeapAttack);

	UReclaimEnemyDefinition* Spitter = NewObject<UReclaimEnemyDefinition>();
	Spitter->EnemyId = TEXT("Spitter");
	Spitter->EnemyTags.AddTag(ReclaimGameplayTags::Enemy_Spitter);
	Spitter->EnemyTags.AddTag(ReclaimGameplayTags::Enemy_Ranged);
	Spitter->PreferredDistance = 900.0f;
	Spitter->RetreatDistance = 350.0f;
	Spitter->bCanMeleeAttack = false;
	Spitter->bCanRangedAttack = true;
	Spitter->bCanReposition = true;

	FReclaimEnemyFacts SpitterFacts;
	SpitterFacts.bHasValidTarget = true;
	SpitterFacts.TargetLifeState = EReclaimPlayerLifeState::Active;
	SpitterFacts.DistanceToTarget = 700.0f;
	SpitterFacts.bHasLineOfSight = true;
	SpitterFacts.bRangedReady = true;
	TestEqual(TEXT("Spitter fires when in ranged window"), Brain->DecideIntentForEnemy(SpitterFacts, Spitter, Config).Type, EReclaimEnemyIntentType::RangedAttack);

	FReclaimEnemyFacts InvalidTargetFacts = SpitterFacts;
	InvalidTargetFacts.TargetLifeState = EReclaimPlayerLifeState::Downed;
	TestEqual(TEXT("Downed player is not a normal combat target"), Brain->DecideIntentForEnemy(InvalidTargetFacts, Spitter, Config).Type, EReclaimEnemyIntentType::Wait);

	UReclaimEnemyDefinition* Brute = NewObject<UReclaimEnemyDefinition>();
	Brute->EnemyId = TEXT("Brute");
	Brute->EnemyTags.AddTag(ReclaimGameplayTags::Enemy_Brute);
	Brute->bCanCharge = true;
	Brute->ChargeMinDistance = 300.0f;
	Brute->ChargeMaxDistance = 1200.0f;

	FReclaimEnemyFacts ChargeFacts;
	ChargeFacts.bHasValidTarget = true;
	ChargeFacts.TargetLifeState = EReclaimPlayerLifeState::Active;
	ChargeFacts.DistanceToTarget = 900.0f;
	ChargeFacts.bHasLineOfSight = true;
	ChargeFacts.bChargeReady = true;
	TestEqual(TEXT("Brute charges in charge range"), Brain->DecideIntentForEnemy(ChargeFacts, Brute, Config).Type, EReclaimEnemyIntentType::Charge);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM4SpawnOrderingTest, "Reclaim.M4AI.DeterministicSpawnOrdering", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM4SpawnOrderingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("X sorts before Y/Z/id"), AReclaimEnemyDirector::CompareSpawnCandidatesDeterministic(FVector(10.0f, 0.0f, 0.0f), TEXT("B"), FVector(20.0f, 0.0f, 0.0f), TEXT("A")), -1);
	TestEqual(TEXT("Y breaks X tie"), AReclaimEnemyDirector::CompareSpawnCandidatesDeterministic(FVector(10.0f, 2.0f, 0.0f), TEXT("A"), FVector(10.0f, 3.0f, 0.0f), TEXT("A")), -1);
	TestEqual(TEXT("Z breaks X/Y tie"), AReclaimEnemyDirector::CompareSpawnCandidatesDeterministic(FVector(10.0f, 2.0f, 1.0f), TEXT("A"), FVector(10.0f, 2.0f, 4.0f), TEXT("A")), -1);
	TestEqual(TEXT("Stable id breaks location tie"), AReclaimEnemyDirector::CompareSpawnCandidatesDeterministic(FVector(10.0f, 2.0f, 1.0f), TEXT("A"), FVector(10.0f, 2.0f, 1.0f), TEXT("B")), -1);
	TestEqual(TEXT("Same location and id compares equal"), AReclaimEnemyDirector::CompareSpawnCandidatesDeterministic(FVector(10.0f, 2.0f, 1.0f), TEXT("A"), FVector(10.0f, 2.0f, 1.0f), TEXT("A")), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM4OcclusionScoringTest, "Reclaim.M4AI.PreferOccludedScoring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM4OcclusionScoringTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("PreferOccluded scores hidden points higher than visible points"),
		AReclaimEnemyDirector::ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule::PreferOccluded, true)
		> AReclaimEnemyDirector::ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule::PreferOccluded, false));
	TestEqual(TEXT("Any ignores visibility preference"), AReclaimEnemyDirector::ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule::Any, true), AReclaimEnemyDirector::ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule::Any, false));
	TestEqual(TEXT("RequireOccluded is handled by validation, not scoring"), AReclaimEnemyDirector::ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule::RequireOccluded, true), AReclaimEnemyDirector::ComputeSpawnVisibilityPreferenceScore(EReclaimSpawnVisibilityRule::RequireOccluded, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM4CooldownValidationOrderTest, "Reclaim.M4AI.CooldownValidationOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM4CooldownValidationOrderTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Range failure wins before cooldown commit"),
		UReclaimEnemyActionRules::EvaluateActionCommitPreconditions(true, true, true, true, true, true, false, true, true, true, true),
		EReclaimEnemyActionValidationResult::OutOfRange);
	TestEqual(TEXT("LOS failure is checked before cooldown"),
		UReclaimEnemyActionRules::EvaluateActionCommitPreconditions(true, true, true, true, true, true, true, false, true, true, true),
		EReclaimEnemyActionValidationResult::LineOfSightBlocked);
	TestEqual(TEXT("Cooldown is only reached after target/range/LOS are valid"),
		UReclaimEnemyActionRules::EvaluateActionCommitPreconditions(true, true, true, true, true, true, true, true, false, true, true),
		EReclaimEnemyActionValidationResult::CooldownBlocked);
	TestEqual(TEXT("Valid preconditions allow commit"),
		UReclaimEnemyActionRules::EvaluateActionCommitPreconditions(true, true, true, true, true, true, true, true, true, true, true),
		EReclaimEnemyActionValidationResult::Success);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM4SpecialCapTest, "Reclaim.M4AI.ThreatBudgetSpecialCap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM4SpecialCapTest::RunTest(const FString& Parameters)
{
	FReclaimEnemyBudgetEntry Brute;
	Brute.EnemyId = TEXT("Brute");
	Brute.ThreatCost = 3;
	Brute.SpawnWeight = 1;
	Brute.bSpecialUnit = true;

	TArray<FReclaimEnemyBudgetEntry> Entries;
	Entries.Add(Brute);

	const FReclaimThreatBudgetSolution Solution = AReclaimEnemyDirector::SolveThreatBudget(12, 1, 12345, Entries);
	TestEqual(TEXT("One special requested when only special entries are legal"), Solution.SpecialUnits, 1);
	TestTrue(TEXT("Special cap is respected exactly"), Solution.SpecialUnits <= 1);
	return true;
}

#endif
