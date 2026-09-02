// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/ReclaimCombatMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimCombatShotgunSpreadTest, "Reclaim.M2Combat.DeterministicShotgunSpread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimCombatShotgunSpreadTest::RunTest(const FString& Parameters)
{
	TArray<FVector> FirstDirections;
	TArray<FVector> SecondDirections;
	TArray<FVector> DifferentShotDirections;

	const int32 ShotSeed = FReclaimCombatMath::MakeWeaponShotSeed(1337, 2, 901, 7);
	const int32 DifferentShotSeed = FReclaimCombatMath::MakeWeaponShotSeed(1337, 2, 901, 8);
	FReclaimCombatMath::BuildDeterministicSpreadDirections(FVector::ForwardVector, 8.0f, 8, ShotSeed, FirstDirections);
	FReclaimCombatMath::BuildDeterministicSpreadDirections(FVector::ForwardVector, 8.0f, 8, ShotSeed, SecondDirections);
	FReclaimCombatMath::BuildDeterministicSpreadDirections(FVector::ForwardVector, 8.0f, 8, DifferentShotSeed, DifferentShotDirections);

	TestEqual(TEXT("Configured pellet count is preserved"), FirstDirections.Num(), 8);
	TestEqual(TEXT("Same seed reproduces pellet count"), SecondDirections.Num(), FirstDirections.Num());

	for (int32 Index = 0; Index < FirstDirections.Num(); ++Index)
	{
		TestTrue(TEXT("Same seed reproduces each pellet direction"), FirstDirections[Index].Equals(SecondDirections[Index], UE_KINDA_SMALL_NUMBER));
	}

	TestFalse(TEXT("Changing shot sequence changes spread"), FirstDirections[0].Equals(DifferentShotDirections[0], UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimCombatAmmoTest, "Reclaim.M2Combat.AmmoCannotGoNegative", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimCombatAmmoTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Cannot spend unavailable ammo"), FReclaimCombatMath::CanSpendAmmo(0, 1));
	TestFalse(TEXT("Cannot partially spend ammo per shot"), FReclaimCombatMath::CanSpendAmmo(1, 2));
	TestEqual(TEXT("Failed spend leaves ammo clamped"), FReclaimCombatMath::ApplyAmmoCostClamped(0, 1), 0);
	TestEqual(TEXT("Accepted spend decrements by cost"), FReclaimCombatMath::ApplyAmmoCostClamped(3, 2), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimCombatReloadTest, "Reclaim.M2Combat.ReloadTransitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimCombatReloadTest::RunTest(const FString& Parameters)
{
	const FReclaimReloadMathResult Reload = FReclaimCombatMath::ComputeReload(10, 90, 30);
	TestTrue(TEXT("Partial magazine with reserve can reload"), Reload.bCanReload);
	TestEqual(TEXT("Reload fills magazine"), Reload.NewMagazineAmmo, 30);
	TestEqual(TEXT("Reload spends reserve"), Reload.NewReserveAmmo, 70);
	TestEqual(TEXT("Reload reports loaded ammo"), Reload.AmmoLoaded, 20);

	TestFalse(TEXT("Full magazine cannot reload"), FReclaimCombatMath::ComputeReload(30, 90, 30).bCanReload);
	TestFalse(TEXT("No reserve cannot reload"), FReclaimCombatMath::ComputeReload(10, 0, 30).bCanReload);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimCombatShieldAbsorptionTest, "Reclaim.M2Combat.ShieldAbsorption", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimCombatShieldAbsorptionTest::RunTest(const FString& Parameters)
{
	const FReclaimShieldDamageMathResult Result = FReclaimCombatMath::ApplyShieldHealthDamage(100.0f, 75.0f, 100.0f, 75.0f, 50.0f);
	TestEqual(TEXT("Shield absorbs normal damage first"), Result.NewShield, 25.0f);
	TestEqual(TEXT("Health is untouched while shield remains"), Result.NewHealth, 100.0f);
	TestEqual(TEXT("Shield damage amount is reported"), Result.ShieldDamage, 50.0f);
	TestEqual(TEXT("No health damage is reported"), Result.HealthDamage, 0.0f);
	TestFalse(TEXT("Shield is not broken"), Result.bShieldBroken);
	TestFalse(TEXT("Target is not defeated"), Result.bTargetDefeated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimCombatDamageOverflowTest, "Reclaim.M2Combat.ShieldOverflowToHealth", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimCombatDamageOverflowTest::RunTest(const FString& Parameters)
{
	const FReclaimShieldDamageMathResult Result = FReclaimCombatMath::ApplyShieldHealthDamage(100.0f, 75.0f, 100.0f, 75.0f, 100.0f);
	TestEqual(TEXT("Shield reaches zero before health damage"), Result.NewShield, 0.0f);
	TestEqual(TEXT("Overflow damage enters health"), Result.NewHealth, 75.0f);
	TestEqual(TEXT("All shield was consumed"), Result.ShieldDamage, 75.0f);
	TestEqual(TEXT("Overflow health damage is reported"), Result.HealthDamage, 25.0f);
	TestTrue(TEXT("Shield broken is reported"), Result.bShieldBroken);
	TestFalse(TEXT("Target remains alive"), Result.bTargetDefeated);

	const FReclaimShieldDamageMathResult Defeat = FReclaimCombatMath::ApplyShieldHealthDamage(25.0f, 0.0f, 100.0f, 75.0f, 40.0f);
	TestEqual(TEXT("Defeat clamps health to zero"), Defeat.NewHealth, 0.0f);
	TestTrue(TEXT("Zero health reports target defeated"), Defeat.bTargetDefeated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReclaimCombatFireRateTest, "Reclaim.M2Combat.FireRateValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReclaimCombatFireRateTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("First accepted fire has no interval block"), FReclaimCombatMath::IsFireIntervalReady(10.0f, -FLT_MAX, 0.1f));
	TestFalse(TEXT("Early fire is rejected"), FReclaimCombatMath::IsFireIntervalReady(10.05f, 10.0f, 0.1f));
	TestTrue(TEXT("Fire at interval is accepted"), FReclaimCombatMath::IsFireIntervalReady(10.1f, 10.0f, 0.1f));
	return true;
}

#endif
