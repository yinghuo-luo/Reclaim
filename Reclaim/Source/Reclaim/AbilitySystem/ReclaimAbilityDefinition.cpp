// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ReclaimAbilityDefinition.h"

FGameplayTag UReclaimAbilityDefinition::GetPrimaryAbilityTag() const
{
	for (const FGameplayTag& Tag : AbilityTags)
	{
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Ability"), false)))
		{
			return Tag;
		}
	}

	return FGameplayTag();
}
