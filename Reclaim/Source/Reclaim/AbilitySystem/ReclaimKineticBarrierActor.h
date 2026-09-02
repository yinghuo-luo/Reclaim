// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ReclaimKineticBarrierActor.generated.h"

class AReclaimPlayerCharacter;
class UBoxComponent;
class UReclaimAbilityDefinition;

UCLASS()
class RECLAIM_API AReclaimKineticBarrierActor : public AActor
{
	GENERATED_BODY()

public:
	AReclaimKineticBarrierActor();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Barrier")
	bool InitializeBarrier_Server(AReclaimPlayerCharacter* NewOwnerCharacter, const UReclaimAbilityDefinition* AbilityDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Barrier")
	bool TryAbsorbDamage_Server(AActor* DamageInstigator, FGameplayTag DamageTypeTag, float IncomingDamage, float& OutRemainingDamage);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Barrier")
	void EndBarrier_Server();

	UFUNCTION(BlueprintPure, Category="Reclaim|Barrier")
	float GetRemainingAbsorption() const { return RemainingAbsorption; }

protected:
	bool IsDamageFromProtectedArc(AActor* DamageInstigator) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Barrier")
	TObjectPtr<UBoxComponent> BarrierCollision;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Barrier")
	TObjectPtr<AReclaimPlayerCharacter> OwnerCharacter = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Barrier")
	float MaxAbsorption = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Barrier")
	float RemainingAbsorption = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Barrier")
	float EndServerTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Barrier", meta=(ClampMin="0"))
	float DefaultDurationSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Barrier", meta=(ClampMin="1"))
	float DefaultAbsorption = 125.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Barrier", meta=(ClampMin="-1", ClampMax="1"))
	float ProtectedArcMinimumDot = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Barrier", meta=(ClampMin="0"))
	float ForwardOffset = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Barrier")
	FVector CollisionExtent = FVector(24.0f, 150.0f, 120.0f);

	FTimerHandle DurationTimerHandle;
};
