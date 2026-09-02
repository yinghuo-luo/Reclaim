// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ReclaimAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class RECLAIM_API UReclaimAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UReclaimAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Reclaim|Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UReclaimAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Reclaim|Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UReclaimAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Shield, Category="Reclaim|Attributes")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UReclaimAttributeSet, Shield);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxShield, Category="Reclaim|Attributes")
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UReclaimAttributeSet, MaxShield);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ShieldRegenRate, Category="Reclaim|Attributes")
	FGameplayAttributeData ShieldRegenRate;
	ATTRIBUTE_ACCESSORS(UReclaimAttributeSet, ShieldRegenRate);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ShieldRegenRate(const FGameplayAttributeData& OldValue);
};
