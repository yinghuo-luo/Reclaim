// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/ReclaimWeaponBase.h"
#include "ReclaimProjectileWeapon.generated.h"

class AReclaimProjectile;

UCLASS()
class RECLAIM_API AReclaimProjectileWeapon : public AReclaimWeaponBase
{
	GENERATED_BODY()

public:
	virtual bool ExecuteAuthoritativeFire(APawn* InstigatorPawn, const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon")
	TSubclassOf<AReclaimProjectile> ProjectileClass;
};
