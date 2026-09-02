// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReclaimWeaponBase.generated.h"

class UReclaimWeaponDefinition;

UCLASS(Abstract)
class RECLAIM_API AReclaimWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AReclaimWeaponBase();

	virtual bool ExecuteAuthoritativeFire(APawn* InstigatorPawn, const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Weapon")
	void SetWeaponDefinition(UReclaimWeaponDefinition* NewWeaponDefinition);

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	UReclaimWeaponDefinition* GetWeaponDefinition() const { return WeaponDefinition; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon")
	TObjectPtr<UReclaimWeaponDefinition> WeaponDefinition = nullptr;
};
