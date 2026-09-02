// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ReclaimGameplayTags.h"

namespace ReclaimGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Role_Vanguard, "Role.Vanguard");
	UE_DEFINE_GAMEPLAY_TAG(Role_Ranger, "Role.Ranger");
	UE_DEFINE_GAMEPLAY_TAG(Role_Engineer, "Role.Engineer");
	UE_DEFINE_GAMEPLAY_TAG(Role_Warden, "Role.Warden");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Vanguard_Dash, "Ability.Vanguard.Dash");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Vanguard_KineticBarrier, "Ability.Vanguard.KineticBarrier");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ranger_TacticalScan, "Ability.Ranger.TacticalScan");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Ranger_MagRailLeap, "Ability.Ranger.MagRailLeap");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Engineer_RepairDrone, "Ability.Engineer.RepairDrone");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Engineer_Overclock, "Ability.Engineer.Overclock");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warden_PurificationPulse, "Ability.Warden.PurificationPulse");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warden_StabilityField, "Ability.Warden.StabilityField");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Type_Combat, "Ability.Type.Combat");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Type_Movement, "Ability.Type.Movement");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Type_Deploy, "Ability.Type.Deploy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Type_Scan, "Ability.Type.Scan");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Type_Channel, "Ability.Type.Channel");

	UE_DEFINE_GAMEPLAY_TAG(State_Shielded, "State.Shielded");
	UE_DEFINE_GAMEPLAY_TAG(State_Downed, "State.Downed");
	UE_DEFINE_GAMEPLAY_TAG(State_Reviving, "State.Reviving");
	UE_DEFINE_GAMEPLAY_TAG(State_Redeploying, "State.Redeploying");
	UE_DEFINE_GAMEPLAY_TAG(State_CannotFire, "State.CannotFire");
	UE_DEFINE_GAMEPLAY_TAG(State_CannotUseAbility, "State.CannotUseAbility");
	UE_DEFINE_GAMEPLAY_TAG(State_Marked, "State.Marked");
	UE_DEFINE_GAMEPLAY_TAG(State_Overclocked, "State.Overclocked");
	UE_DEFINE_GAMEPLAY_TAG(State_StabilityField, "State.StabilityField");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Kinetic, "Damage.Kinetic");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Electric, "Damage.Electric");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Explosive, "Damage.Explosive");

	UE_DEFINE_GAMEPLAY_TAG(Build_Kinetic, "Build.Kinetic");
	UE_DEFINE_GAMEPLAY_TAG(Build_Electric, "Build.Electric");
	UE_DEFINE_GAMEPLAY_TAG(Build_Dash, "Build.Dash");
	UE_DEFINE_GAMEPLAY_TAG(Build_Defense, "Build.Defense");
	UE_DEFINE_GAMEPLAY_TAG(Build_Device, "Build.Device");
	UE_DEFINE_GAMEPLAY_TAG(Build_Survival, "Build.Survival");

	UE_DEFINE_GAMEPLAY_TAG(Enemy_Crawler, "Enemy.Crawler");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Spitter, "Enemy.Spitter");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Brute, "Enemy.Brute");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Elite, "Enemy.Elite");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Armored, "Enemy.Armored");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ranged, "Enemy.Ranged");

	UE_DEFINE_GAMEPLAY_TAG(Mission_Active, "Mission.Active");
	UE_DEFINE_GAMEPLAY_TAG(Mission_Complete, "Mission.Complete");
	UE_DEFINE_GAMEPLAY_TAG(Mission_Failed, "Mission.Failed");
	UE_DEFINE_GAMEPLAY_TAG(Mission_Extracting, "Mission.Extracting");

	UE_DEFINE_GAMEPLAY_TAG(Resource_Alloy, "Resource.Alloy");
	UE_DEFINE_GAMEPLAY_TAG(Resource_Crystal, "Resource.Crystal");
	UE_DEFINE_GAMEPLAY_TAG(Resource_Biopolymer, "Resource.Biopolymer");
	UE_DEFINE_GAMEPLAY_TAG(Resource_Anomaly, "Resource.Anomaly");

	UE_DEFINE_GAMEPLAY_TAG(Target_Hostile, "Target.Hostile");
	UE_DEFINE_GAMEPLAY_TAG(Target_Purifiable, "Target.Purifiable");
}
