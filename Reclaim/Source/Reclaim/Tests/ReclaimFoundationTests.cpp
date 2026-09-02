// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ReclaimEnemyDirector.h"
#include "Core/ReclaimDeterministicRandom.h"
#include "Core/ReclaimRoleReservationRules.h"
#include "Rewards/ReclaimRewardService.h"
#include "Save/ReclaimFabricationService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimRoleRulesTest, "Reclaim.Foundation.RoleRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimRoleRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Free role succeeds"), UReclaimRoleReservationRules::EvaluateRoleRequest(EReclaimRole::None, EReclaimRole::Vanguard, false, true, true, false), EReclaimRoleRequestResult::Success);
	TestEqual(TEXT("Ready player cannot switch"), UReclaimRoleReservationRules::EvaluateRoleRequest(EReclaimRole::Vanguard, EReclaimRole::Ranger, true, true, true, false), EReclaimRoleRequestResult::ReadyLocked);
	TestEqual(TEXT("Occupied role is rejected"), UReclaimRoleReservationRules::EvaluateRoleRequest(EReclaimRole::Vanguard, EReclaimRole::Ranger, false, true, true, true), EReclaimRoleRequestResult::RoleOccupied);
	TestEqual(TEXT("Same role is idempotent"), UReclaimRoleReservationRules::EvaluateRoleRequest(EReclaimRole::Engineer, EReclaimRole::Engineer, false, true, true, false), EReclaimRoleRequestResult::Success);
	TestEqual(TEXT("None is invalid"), UReclaimRoleReservationRules::EvaluateRoleRequest(EReclaimRole::Engineer, EReclaimRole::None, false, true, true, false), EReclaimRoleRequestResult::InvalidRole);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimDeterministicRandomTest, "Reclaim.Foundation.DeterministicRandom", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimDeterministicRandomTest::RunTest(const FString& Parameters)
{
	const int32 SeedA = UReclaimDeterministicRandom::MakeSubSeed(1234, EReclaimRandomDomain::RewardTerminal, 1, 2, 3);
	const int32 SeedB = UReclaimDeterministicRandom::MakeSubSeed(1234, EReclaimRandomDomain::RewardTerminal, 1, 2, 3);
	const int32 SeedC = UReclaimDeterministicRandom::MakeSubSeed(1234, EReclaimRandomDomain::RewardTerminal, 2, 2, 3);
	const int32 SpreadSeedA = UReclaimDeterministicRandom::MakeSubSeed(1234, EReclaimRandomDomain::WeaponSpread, 0, 100, 1);
	const int32 SpreadSeedB = UReclaimDeterministicRandom::MakeSubSeed(1234, EReclaimRandomDomain::WeaponSpread, 0, 100, 2);

	TestEqual(TEXT("Same inputs reproduce sub-seed"), SeedA, SeedB);
	TestNotEqual(TEXT("Changing player slot changes sub-seed"), SeedA, SeedC);
	TestNotEqual(TEXT("Shot sequence changes weapon spread seed"), SpreadSeedA, SpreadSeedB);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimResourceBundleTest, "Reclaim.Foundation.ResourceBundle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimResourceBundleTest::RunTest(const FString& Parameters)
{
	FReclaimResourceBundle Resources;
	Resources.Alloy = 10;
	Resources.Crystal = 3;

	FReclaimResourceBundle Cost;
	Cost.Alloy = 4;
	Cost.Crystal = 4;
	TestFalse(TEXT("Cannot overspend crystal"), Resources.CanSpend(Cost));

	Cost.Crystal = 2;
	TestTrue(TEXT("Can spend available resources"), Resources.SpendIfPossible(Cost));
	TestEqual(TEXT("Alloy was decremented"), Resources.Alloy, 6);
	TestEqual(TEXT("Crystal was decremented"), Resources.Crystal, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimRewardServiceTest, "Reclaim.Foundation.Rewards", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimRewardServiceTest::RunTest(const FString& Parameters)
{
	TArray<FReclaimRewardCandidate> Pool;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FReclaimRewardCandidate Candidate;
		Candidate.CandidateId = FName(*FString::Printf(TEXT("Candidate_%d"), Index));
		Candidate.Weight = 1;
		Pool.Add(Candidate);
	}

	const TArray<FReclaimRewardCandidate> First = UReclaimRewardService::GenerateCandidates(9001, 0, TEXT("Node1"), Pool);
	const TArray<FReclaimRewardCandidate> Second = UReclaimRewardService::GenerateCandidates(9001, 0, TEXT("Node1"), Pool);
	const TArray<FReclaimRewardCandidate> OtherPlayer = UReclaimRewardService::GenerateCandidates(9001, 1, TEXT("Node1"), Pool);

	TestEqual(TEXT("Three candidates generated"), First.Num(), 3);
	TestEqual(TEXT("Same inputs reproduce first candidate"), First[0].CandidateId, Second[0].CandidateId);
	TestNotEqual(TEXT("Player slot changes candidate stream"), First[0].CandidateId, OtherPlayer[0].CandidateId);

	TSet<FName> UniqueIds;
	for (const FReclaimRewardCandidate& Candidate : First)
	{
		UniqueIds.Add(Candidate.CandidateId);
		TestTrue(TEXT("Candidate belongs to generated set"), UReclaimRewardService::IsCandidateLegal(Candidate.CandidateId, First));
	}
	TestEqual(TEXT("Candidates are unique when pool is large enough"), UniqueIds.Num(), First.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimThreatBudgetTest, "Reclaim.Foundation.ThreatBudget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimThreatBudgetTest::RunTest(const FString& Parameters)
{
	const UReclaimAIConfig* DefaultConfig = GetDefault<UReclaimAIConfig>();
	TestEqual(TEXT("1P budget uses base"), AReclaimEnemyDirector::ComputeThreatBudget(12, AReclaimEnemyDirector::ResolveScaling(DefaultConfig, 1).ThreatBudgetScalar), 12);
	TestEqual(TEXT("2P budget uses configured scalar"), AReclaimEnemyDirector::ComputeThreatBudget(12, AReclaimEnemyDirector::ResolveScaling(DefaultConfig, 2).ThreatBudgetScalar), 20);
	TestEqual(TEXT("4P budget uses configured scalar"), AReclaimEnemyDirector::ComputeThreatBudget(12, AReclaimEnemyDirector::ResolveScaling(DefaultConfig, 4).ThreatBudgetScalar), 34);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimFabricationTest, "Reclaim.Foundation.Fabrication", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimFabricationTest::RunTest(const FString& Parameters)
{
	FReclaimResourceBundle Resources;
	Resources.Alloy = 5;
	FReclaimResourceBundle Cost;
	Cost.Alloy = 4;
	TestTrue(TEXT("Fabrication can spend valid resource cost"), UReclaimFabricationService::CanSpend(Resources, Cost));

	TArray<FReclaimFabricationEntry> Entries;
	FReclaimFabricationEntry Entry;
	Entry.ItemId = TEXT("ArcGunPrototype");
	Entry.Weight = 1;
	Entries.Add(Entry);
	TestEqual(TEXT("Single valid fabrication entry is selected"), UReclaimFabricationService::RollFabricationItem(42, Entries), FName(TEXT("ArcGunPrototype")));
	return true;
}

#endif
