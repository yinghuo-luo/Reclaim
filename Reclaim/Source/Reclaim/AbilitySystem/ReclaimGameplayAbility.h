// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimGameplayAbility.generated.h"

class AReclaimPlayerCharacter;
class UReclaimAbilityDefinition;
class UReclaimAbilitySystemComponent;

UCLASS(Abstract)
class RECLAIM_API UReclaimGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UReclaimGameplayAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability")
	TObjectPtr<UReclaimAbilityDefinition> Definition = nullptr;

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	const UReclaimAbilityDefinition* GetReclaimAbilityDefinition() const;
	UReclaimAbilitySystemComponent* GetReclaimAbilitySystemComponentFromActorInfo() const;
	AReclaimPlayerCharacter* GetReclaimAvatarCharacter() const;
	bool ConsumeActivationPayload(FReclaimAbilityActivationPayload& OutPayload) const;
	bool ValidateCommonPayload(const FReclaimAbilityActivationPayload& Payload, float MaxAimOriginDistance, float MaxAimAngleDegrees) const;
	bool CommitReclaimAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);
	void ConfirmAbilityPresentation(FGameplayTag AbilityTag, FVector Location) const;
};
