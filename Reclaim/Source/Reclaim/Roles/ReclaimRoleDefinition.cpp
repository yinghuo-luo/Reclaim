// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roles/ReclaimRoleDefinition.h"

UReclaimAbilityDefinition* UReclaimRoleDefinition::GetCoreAbilityBySlot(int32 AbilitySlot) const
{
	return CoreAbilities.IsValidIndex(AbilitySlot) ? CoreAbilities[AbilitySlot] : nullptr;
}
