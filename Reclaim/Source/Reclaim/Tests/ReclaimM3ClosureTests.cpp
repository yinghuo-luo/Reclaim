// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "AbilitySystem/ReclaimAbilitySystemComponent.h"
#include "Core/ReclaimCooperationRules.h"
#include "Core/ReclaimGameplayTags.h"
#include "Roles/ReclaimRoleDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM3LifeStateRulesTest, "Reclaim.M3Cooperation.LifeStateRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM3LifeStateRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Active can fight"), UReclaimCooperationRules::IsCombatCapable(EReclaimPlayerLifeState::Active));
	TestFalse(TEXT("Downed cannot fight"), UReclaimCooperationRules::IsCombatCapable(EReclaimPlayerLifeState::Downed));
	TestFalse(TEXT("Destroyed cannot fight"), UReclaimCooperationRules::IsCombatCapable(EReclaimPlayerLifeState::Destroyed));
	TestFalse(TEXT("Redeploying cannot fight"), UReclaimCooperationRules::IsCombatCapable(EReclaimPlayerLifeState::Redeploying));

	TestTrue(TEXT("Active can fire without blocking tag"), UReclaimCooperationRules::CanFireWeapon(EReclaimPlayerLifeState::Active, false));
	TestFalse(TEXT("Active cannot fire with blocking tag"), UReclaimCooperationRules::CanFireWeapon(EReclaimPlayerLifeState::Active, true));
	TestFalse(TEXT("Downed cannot use normal ability"), UReclaimCooperationRules::CanUseNormalAbility(EReclaimPlayerLifeState::Downed, false));
	TestTrue(TEXT("Destroyed with redeploy remaining can redeploy"), UReclaimCooperationRules::CanRedeploy(EReclaimPlayerLifeState::Destroyed, 1));
	TestFalse(TEXT("Redeploying cannot spend another redeploy"), UReclaimCooperationRules::CanRedeploy(EReclaimPlayerLifeState::Redeploying, 1));

	TArray<FReclaimParticipantLifeSnapshot> Participants;
	FReclaimParticipantLifeSnapshot DestroyedNoRedeploy;
	DestroyedNoRedeploy.LifeState = EReclaimPlayerLifeState::Destroyed;
	DestroyedNoRedeploy.RedeploysRemaining = 0;
	Participants.Add(DestroyedNoRedeploy);
	TestTrue(TEXT("All destroyed with no redeploys wipes"), UReclaimCooperationRules::ShouldTriggerTeamWipe(Participants));

	Participants[0].RedeploysRemaining = 1;
	TestFalse(TEXT("Remaining redeploy prevents wipe"), UReclaimCooperationRules::ShouldTriggerTeamWipe(Participants));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM3ReviveValidationTest, "Reclaim.M3Cooperation.ReviveValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM3ReviveValidationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Active reviver can revive downed friendly in range"),
		UReclaimCooperationRules::EvaluateReviveStart(EReclaimPlayerLifeState::Active, EReclaimPlayerLifeState::Downed, 200.0f, 300.0f, false, true),
		EReclaimReviveValidationResult::Success);
	TestEqual(TEXT("Downed reviver rejected"),
		UReclaimCooperationRules::EvaluateReviveStart(EReclaimPlayerLifeState::Downed, EReclaimPlayerLifeState::Downed, 200.0f, 300.0f, false, true),
		EReclaimReviveValidationResult::ReviverUnavailable);
	TestEqual(TEXT("Active target rejected"),
		UReclaimCooperationRules::EvaluateReviveStart(EReclaimPlayerLifeState::Active, EReclaimPlayerLifeState::Active, 200.0f, 300.0f, false, true),
		EReclaimReviveValidationResult::TargetNotDowned);
	TestEqual(TEXT("Same player rejected"),
		UReclaimCooperationRules::EvaluateReviveStart(EReclaimPlayerLifeState::Active, EReclaimPlayerLifeState::Downed, 200.0f, 300.0f, true, true),
		EReclaimReviveValidationResult::SamePlayer);
	TestEqual(TEXT("Too far rejected"),
		UReclaimCooperationRules::EvaluateReviveStart(EReclaimPlayerLifeState::Active, EReclaimPlayerLifeState::Downed, 301.0f, 300.0f, false, true),
		EReclaimReviveValidationResult::TooFar);
	TestTrue(TEXT("Cancel interrupts revive"),
		UReclaimCooperationRules::ShouldInterruptRevive(EReclaimPlayerLifeState::Active, EReclaimPlayerLifeState::Downed, 100.0f, 300.0f, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM3RoleAbilitySlotMappingTest, "Reclaim.M3Ability.RoleAbilitySlotMapping", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM3RoleAbilitySlotMappingTest::RunTest(const FString& Parameters)
{
	struct FExpectedRoleSlots
	{
		EReclaimRole Role;
		FGameplayTag AbilityOne;
		FGameplayTag AbilityTwo;
	};

	const FExpectedRoleSlots ExpectedSlots[] =
	{
		{ EReclaimRole::Vanguard, ReclaimGameplayTags::Ability_Vanguard_Dash, ReclaimGameplayTags::Ability_Vanguard_KineticBarrier },
		{ EReclaimRole::Ranger, ReclaimGameplayTags::Ability_Ranger_TacticalScan, ReclaimGameplayTags::Ability_Ranger_MagRailLeap },
		{ EReclaimRole::Engineer, ReclaimGameplayTags::Ability_Engineer_RepairDrone, ReclaimGameplayTags::Ability_Engineer_Overclock },
		{ EReclaimRole::Warden, ReclaimGameplayTags::Ability_Warden_PurificationPulse, ReclaimGameplayTags::Ability_Warden_StabilityField }
	};

	for (const FExpectedRoleSlots& Expected : ExpectedSlots)
	{
		UReclaimRoleDefinition* RoleDefinition = NewObject<UReclaimRoleDefinition>();
		RoleDefinition->Role = Expected.Role;

		UReclaimAbilityDefinition* AbilityOne = NewObject<UReclaimAbilityDefinition>();
		AbilityOne->AbilityTags.AddTag(Expected.AbilityOne);
		UReclaimAbilityDefinition* AbilityTwo = NewObject<UReclaimAbilityDefinition>();
		AbilityTwo->AbilityTags.AddTag(Expected.AbilityTwo);
		RoleDefinition->CoreAbilities.Add(AbilityOne);
		RoleDefinition->CoreAbilities.Add(AbilityTwo);

		TestEqual(TEXT("Ability slot 1 maps to expected tag"), RoleDefinition->GetCoreAbilityBySlot(0)->GetPrimaryAbilityTag(), Expected.AbilityOne);
		TestEqual(TEXT("Ability slot 2 maps to expected tag"), RoleDefinition->GetCoreAbilityBySlot(1)->GetPrimaryAbilityTag(), Expected.AbilityTwo);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimM3RuntimeChargesTest, "Reclaim.M3Ability.RuntimeCharges", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimM3RuntimeChargesTest::RunTest(const FString& Parameters)
{
	UReclaimAbilityDefinition* AbilityDefinition = NewObject<UReclaimAbilityDefinition>();
	AbilityDefinition->AbilityTags.AddTag(ReclaimGameplayTags::Ability_Engineer_RepairDrone);
	AbilityDefinition->CooldownSeconds = 1.0f;
	AbilityDefinition->MaxCharges = 1;
	AbilityDefinition->ChargeRecoverySeconds = 2.0f;

	FReclaimAbilityRuntimeState RuntimeState;
	RuntimeState.AbilityTag = AbilityDefinition->GetPrimaryAbilityTag();
	RuntimeState.ChargesRemaining = 1;

	FString FailureReason;
	TestTrue(TEXT("Initial state is ready"), UReclaimAbilitySystemComponent::IsRuntimeStateReadyAtTime(AbilityDefinition, &RuntimeState, 10.0f, FailureReason));

	UReclaimAbilitySystemComponent::CommitRuntimeStateAtTime(AbilityDefinition, RuntimeState, 10.0f);
	TestEqual(TEXT("Commit consumes charge"), RuntimeState.ChargesRemaining, 0);
	TestFalse(TEXT("Cooldown blocks immediate reuse"), UReclaimAbilitySystemComponent::IsRuntimeStateReadyAtTime(AbilityDefinition, &RuntimeState, 10.5f, FailureReason));
	TestEqual(TEXT("Cooldown failure reason"), FailureReason, FString(TEXT("cooldown")));

	TestFalse(TEXT("No charge blocks after cooldown before recharge"), UReclaimAbilitySystemComponent::IsRuntimeStateReadyAtTime(AbilityDefinition, &RuntimeState, 11.5f, FailureReason));
	TestEqual(TEXT("Charge failure reason"), FailureReason, FString(TEXT("charges")));

	UReclaimAbilitySystemComponent::RefreshRuntimeStateAtTime(AbilityDefinition, RuntimeState, 12.1f);
	TestEqual(TEXT("Refresh restores recovered charge"), RuntimeState.ChargesRemaining, 1);
	TestTrue(TEXT("Recovered state is ready"), UReclaimAbilitySystemComponent::IsRuntimeStateReadyAtTime(AbilityDefinition, &RuntimeState, 12.1f, FailureReason));
	return true;
}

#endif
