// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/ReclaimHitscanWeapon.h"

#include "Weapons/ReclaimWeaponDefinition.h"

bool AReclaimHitscanWeapon::ExecuteAuthoritativeFire(APawn* InstigatorPawn, const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence)
{
	return Super::ExecuteAuthoritativeFire(InstigatorPawn, AimOrigin, AimDirection, ShotSequence)
		&& WeaponDefinition
		&& WeaponDefinition->FireModel == EReclaimWeaponFireModel::Hitscan;
}
