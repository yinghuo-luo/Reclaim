// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/ReclaimProjectileWeapon.h"

#include "Weapons/ReclaimProjectile.h"
#include "Weapons/ReclaimWeaponDefinition.h"

bool AReclaimProjectileWeapon::ExecuteAuthoritativeFire(APawn* InstigatorPawn, const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence)
{
	if (!Super::ExecuteAuthoritativeFire(InstigatorPawn, AimOrigin, AimDirection, ShotSequence))
	{
		return false;
	}

	TSubclassOf<AReclaimProjectile> ClassToSpawn = ProjectileClass;
	if (!ClassToSpawn && WeaponDefinition)
	{
		ClassToSpawn = WeaponDefinition->ProjectileClass;
	}

	if (!ClassToSpawn)
	{
		ClassToSpawn = AReclaimProjectile::StaticClass();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = InstigatorPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector SpawnLocation = AimOrigin;
	const FRotator SpawnRotation = AimDirection.GetSafeNormal().Rotation();
	AReclaimProjectile* Projectile = World->SpawnActor<AReclaimProjectile>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParameters);
	if (!Projectile)
	{
		return false;
	}

	Projectile->InitializeDamageProjectile_Server(WeaponDefinition->Damage, WeaponDefinition->DamageTypeTag, InstigatorPawn, 1400.0f, 8.0f);
	return true;
}
