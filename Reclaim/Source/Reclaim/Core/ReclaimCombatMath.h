// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct RECLAIM_API FReclaimShieldDamageMathResult
{
	float IncomingDamage = 0.0f;
	float ShieldDamage = 0.0f;
	float HealthDamage = 0.0f;
	float NewHealth = 0.0f;
	float NewShield = 0.0f;
	bool bShieldBroken = false;
	bool bTargetDefeated = false;
};

struct RECLAIM_API FReclaimReloadMathResult
{
	int32 NewMagazineAmmo = 0;
	int32 NewReserveAmmo = 0;
	int32 AmmoLoaded = 0;
	bool bCanReload = false;
};

class RECLAIM_API FReclaimCombatMath
{
public:
	static bool CanSpendAmmo(int32 AmmoInMagazine, int32 AmmoPerShot);
	static int32 ApplyAmmoCostClamped(int32 AmmoInMagazine, int32 AmmoPerShot);

	static bool CanReload(int32 AmmoInMagazine, int32 ReserveAmmo, int32 MagazineSize);
	static FReclaimReloadMathResult ComputeReload(int32 AmmoInMagazine, int32 ReserveAmmo, int32 MagazineSize);

	static bool IsFireIntervalReady(float CurrentTimeSeconds, float LastAcceptedFireTimeSeconds, float FireIntervalSeconds);

	static int32 MakeWeaponShotSeed(int32 RunSeed, int32 PlayerSlot, int32 WeaponSeedSalt, int32 ShotSequence);
	static FVector MakeDeterministicSpreadDirection(const FVector& AimDirection, float SpreadDegrees, int32 ShotSeed, int32 PelletIndex);
	static void BuildDeterministicSpreadDirections(const FVector& AimDirection, float SpreadDegrees, int32 PelletCount, int32 ShotSeed, TArray<FVector>& OutDirections);

	static FReclaimShieldDamageMathResult ApplyShieldHealthDamage(float CurrentHealth, float CurrentShield, float MaxHealth, float MaxShield, float IncomingDamage);
};
