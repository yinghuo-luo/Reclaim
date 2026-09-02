// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ReclaimAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UReclaimAttributeSet::UReclaimAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitMaxShield(75.0f);
	InitShield(75.0f);
	InitShieldRegenRate(15.0f);
}

void UReclaimAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimAttributeSet, ShieldRegenRate, COND_None, REPNOTIFY_Always);
}

void UReclaimAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxShieldAttribute() || Attribute == GetShieldRegenRateAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void UReclaimAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UReclaimAttributeSet, Health, OldValue);
}

void UReclaimAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UReclaimAttributeSet, MaxHealth, OldValue);
}

void UReclaimAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UReclaimAttributeSet, Shield, OldValue);
}

void UReclaimAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UReclaimAttributeSet, MaxShield, OldValue);
}

void UReclaimAttributeSet::OnRep_ShieldRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UReclaimAttributeSet, ShieldRegenRate, OldValue);
}
