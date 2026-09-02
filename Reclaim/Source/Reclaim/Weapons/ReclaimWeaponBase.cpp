// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/ReclaimWeaponBase.h"

#include "Weapons/ReclaimWeaponDefinition.h"

AReclaimWeaponBase::AReclaimWeaponBase()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
}

bool AReclaimWeaponBase::ExecuteAuthoritativeFire(APawn* InstigatorPawn, const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence)
{
	return HasAuthority()
		&& InstigatorPawn != nullptr
		&& WeaponDefinition != nullptr
		&& !AimOrigin.ContainsNaN()
		&& !AimDirection.ContainsNaN()
		&& !AimDirection.IsNearlyZero()
		&& ShotSequence >= 0;
}

void AReclaimWeaponBase::SetWeaponDefinition(UReclaimWeaponDefinition* NewWeaponDefinition)
{
	if (HasAuthority())
	{
		WeaponDefinition = NewWeaponDefinition;
	}
}
