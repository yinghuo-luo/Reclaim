// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReclaimStabilityFieldActor.generated.h"

class AReclaimPlayerCharacter;
class UAbilitySystemComponent;
class USphereComponent;
class UReclaimAbilityDefinition;

UCLASS()
class RECLAIM_API AReclaimStabilityFieldActor : public AActor
{
	GENERATED_BODY()

public:
	AReclaimStabilityFieldActor();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|StabilityField")
	bool InitializeStabilityField_Server(AReclaimPlayerCharacter* NewSourceCharacter, const UReclaimAbilityDefinition* AbilityDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|StabilityField")
	void EndStabilityField_Server();

protected:
	UFUNCTION()
	void OnFieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnFieldEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void RefreshFieldTargets_Server();
	bool TryApplyFieldToActor_Server(AActor* Actor);
	void RemoveFieldFromAbilitySystem_Server(UAbilitySystemComponent* AbilitySystem);
	void ClearAllFieldEffects_Server();
	bool IsActorEligibleForField_Server(AActor* Actor, UAbilitySystemComponent*& OutAbilitySystem) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|StabilityField")
	TObjectPtr<USphereComponent> FieldCollision;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|StabilityField")
	TObjectPtr<AReclaimPlayerCharacter> SourceCharacter = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|StabilityField")
	float FieldRadius = 500.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|StabilityField")
	float EndServerTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|StabilityField", meta=(ClampMin="0"))
	float DefaultDurationSeconds = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|StabilityField", meta=(ClampMin="0"))
	float DefaultRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|StabilityField", meta=(ClampMin="0.05"))
	float TargetRefreshInterval = 0.2f;

	TSet<TWeakObjectPtr<UAbilitySystemComponent>> AffectedAbilitySystems;
	FTimerHandle DurationTimerHandle;
	FTimerHandle TargetRefreshTimerHandle;
};
