// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ReclaimProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class RECLAIM_API AReclaimProjectile : public AActor
{
	GENERATED_BODY()

public:
	AReclaimProjectile();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Projectile")
	void InitializeDamageProjectile_Server(float InDamageAmount, FGameplayTag InDamageTypeTag, AActor* InIgnoredActor, float InitialSpeed, float LifeSeconds);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Projectile|Damage", meta=(ClampMin="0"))
	float DamageAmount = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Projectile|Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(Transient)
	TObjectPtr<AActor> IgnoredActor = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Projectile|Damage")
	bool bDestroyOnImpact = true;
};
