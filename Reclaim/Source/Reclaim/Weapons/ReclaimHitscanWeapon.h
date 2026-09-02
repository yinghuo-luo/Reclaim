// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/ReclaimWeaponBase.h"
#include "ReclaimHitscanWeapon.generated.h"

UCLASS()
class RECLAIM_API AReclaimHitscanWeapon : public AReclaimWeaponBase
{
	GENERATED_BODY()

public:
	virtual bool ExecuteAuthoritativeFire(APawn* InstigatorPawn, const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence) override;
};
