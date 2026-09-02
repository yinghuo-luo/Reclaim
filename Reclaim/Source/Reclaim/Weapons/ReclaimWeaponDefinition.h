// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReclaimWeaponDefinition.generated.h"

class AReclaimProjectile;

UENUM(BlueprintType)
enum class EReclaimWeaponFireModel : uint8
{
	Hitscan,
	Projectile
};

UCLASS(BlueprintType)
class RECLAIM_API UReclaimWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon")
	FName WeaponId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon")
	FGameplayTagContainer WeaponTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon")
	EReclaimWeaponFireModel FireModel = EReclaimWeaponFireModel::Hitscan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Ammo", meta=(ClampMin="1"))
	int32 MagazineSize = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Ammo", meta=(ClampMin="0"))
	int32 ReserveAmmo = 90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Ammo", meta=(ClampMin="1"))
	int32 AmmoPerShot = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Ammo", meta=(ClampMin="0"))
	float ReloadDuration = 1.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Fire", meta=(ClampMin="0"))
	float FireInterval = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Damage", meta=(ClampMin="0"))
	float Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Fire", meta=(ClampMin="0"))
	float Range = 12000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Fire", meta=(ClampMin="1"))
	int32 PelletCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Fire", meta=(ClampMin="0"))
	float SpreadDegrees = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Fire")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Random")
	int32 WeaponSeedSalt = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Projectile")
	TSubclassOf<AReclaimProjectile> ProjectileClass;
};
