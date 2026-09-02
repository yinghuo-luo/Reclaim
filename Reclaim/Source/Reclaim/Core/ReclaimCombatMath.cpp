// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ReclaimCombatMath.h"

#include "Core/ReclaimDeterministicRandom.h"
#include "Core/ReclaimTypes.h"

bool FReclaimCombatMath::CanSpendAmmo(int32 AmmoInMagazine, int32 AmmoPerShot)
{
	const int32 ClampedCost = FMath::Max(1, AmmoPerShot);
	return AmmoInMagazine >= ClampedCost;
}

int32 FReclaimCombatMath::ApplyAmmoCostClamped(int32 AmmoInMagazine, int32 AmmoPerShot)
{
	if (!CanSpendAmmo(AmmoInMagazine, AmmoPerShot))
	{
		return FMath::Max(0, AmmoInMagazine);
	}

	return FMath::Max(0, AmmoInMagazine - FMath::Max(1, AmmoPerShot));
}

bool FReclaimCombatMath::CanReload(int32 AmmoInMagazine, int32 ReserveAmmo, int32 MagazineSize)
{
	return MagazineSize > 0 && AmmoInMagazine < MagazineSize && ReserveAmmo > 0;
}

FReclaimReloadMathResult FReclaimCombatMath::ComputeReload(int32 AmmoInMagazine, int32 ReserveAmmo, int32 MagazineSize)
{
	FReclaimReloadMathResult Result;
	Result.NewMagazineAmmo = FMath::Clamp(AmmoInMagazine, 0, FMath::Max(0, MagazineSize));
	Result.NewReserveAmmo = FMath::Max(0, ReserveAmmo);

	if (!CanReload(Result.NewMagazineAmmo, Result.NewReserveAmmo, MagazineSize))
	{
		return Result;
	}

	const int32 MissingAmmo = FMath::Max(0, MagazineSize - Result.NewMagazineAmmo);
	Result.AmmoLoaded = FMath::Min(MissingAmmo, Result.NewReserveAmmo);
	Result.NewMagazineAmmo += Result.AmmoLoaded;
	Result.NewReserveAmmo -= Result.AmmoLoaded;
	Result.bCanReload = Result.AmmoLoaded > 0;
	return Result;
}

bool FReclaimCombatMath::IsFireIntervalReady(float CurrentTimeSeconds, float LastAcceptedFireTimeSeconds, float FireIntervalSeconds)
{
	if (LastAcceptedFireTimeSeconds <= -FLT_MAX * 0.5f)
	{
		return true;
	}

	return CurrentTimeSeconds + UE_KINDA_SMALL_NUMBER >= LastAcceptedFireTimeSeconds + FMath::Max(0.0f, FireIntervalSeconds);
}

int32 FReclaimCombatMath::MakeWeaponShotSeed(int32 RunSeed, int32 PlayerSlot, int32 WeaponSeedSalt, int32 ShotSequence)
{
	return UReclaimDeterministicRandom::MakeSubSeed(RunSeed, EReclaimRandomDomain::WeaponSpread, PlayerSlot, WeaponSeedSalt, ShotSequence);
}

FVector FReclaimCombatMath::MakeDeterministicSpreadDirection(const FVector& AimDirection, float SpreadDegrees, int32 ShotSeed, int32 PelletIndex)
{
	const FVector NormalizedAim = AimDirection.GetSafeNormal();
	if (NormalizedAim.IsNearlyZero())
	{
		return FVector::ForwardVector;
	}

	if (SpreadDegrees <= UE_KINDA_SMALL_NUMBER)
	{
		return NormalizedAim;
	}

	const int32 PelletSeed = UReclaimDeterministicRandom::MakeSubSeed(ShotSeed, EReclaimRandomDomain::WeaponSpread, PelletIndex);
	FRandomStream PelletStream(PelletSeed);
	const float ConeHalfAngleRadians = FMath::DegreesToRadians(FMath::Max(0.0f, SpreadDegrees));
	return PelletStream.VRandCone(NormalizedAim, ConeHalfAngleRadians).GetSafeNormal();
}

void FReclaimCombatMath::BuildDeterministicSpreadDirections(const FVector& AimDirection, float SpreadDegrees, int32 PelletCount, int32 ShotSeed, TArray<FVector>& OutDirections)
{
	OutDirections.Reset();

	const int32 ClampedPelletCount = FMath::Max(1, PelletCount);
	OutDirections.Reserve(ClampedPelletCount);
	for (int32 PelletIndex = 0; PelletIndex < ClampedPelletCount; ++PelletIndex)
	{
		OutDirections.Add(MakeDeterministicSpreadDirection(AimDirection, SpreadDegrees, ShotSeed, PelletIndex));
	}
}

FReclaimShieldDamageMathResult FReclaimCombatMath::ApplyShieldHealthDamage(float CurrentHealth, float CurrentShield, float MaxHealth, float MaxShield, float IncomingDamage)
{
	FReclaimShieldDamageMathResult Result;
	Result.IncomingDamage = FMath::Max(0.0f, IncomingDamage);

	const float ClampedMaxHealth = FMath::Max(0.0f, MaxHealth);
	const float ClampedMaxShield = FMath::Max(0.0f, MaxShield);
	const float StartingHealth = FMath::Clamp(CurrentHealth, 0.0f, ClampedMaxHealth);
	const float StartingShield = FMath::Clamp(CurrentShield, 0.0f, ClampedMaxShield);

	Result.ShieldDamage = FMath::Min(StartingShield, Result.IncomingDamage);
	const float RemainingDamage = FMath::Max(0.0f, Result.IncomingDamage - Result.ShieldDamage);
	Result.HealthDamage = FMath::Min(StartingHealth, RemainingDamage);

	Result.NewShield = FMath::Clamp(StartingShield - Result.ShieldDamage, 0.0f, ClampedMaxShield);
	Result.NewHealth = FMath::Clamp(StartingHealth - Result.HealthDamage, 0.0f, ClampedMaxHealth);
	Result.bShieldBroken = StartingShield > 0.0f && Result.NewShield <= 0.0f && Result.ShieldDamage > 0.0f;
	Result.bTargetDefeated = StartingHealth > 0.0f && Result.NewHealth <= 0.0f && Result.HealthDamage > 0.0f;
	return Result;
}
