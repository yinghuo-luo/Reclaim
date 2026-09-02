// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ReclaimRoleReservationRules.h"

EReclaimRoleRequestResult UReclaimRoleReservationRules::EvaluateRoleRequest(EReclaimRole CurrentRole, EReclaimRole DesiredRole, bool bReady, bool bInLobby, bool bPlayerFound, bool bDesiredRoleOccupiedByOther)
{
	if (!bInLobby)
	{
		return EReclaimRoleRequestResult::NotInLobby;
	}

	if (!bPlayerFound)
	{
		return EReclaimRoleRequestResult::PlayerNotFound;
	}

	if (DesiredRole == EReclaimRole::None)
	{
		return EReclaimRoleRequestResult::InvalidRole;
	}

	if (bReady)
	{
		return EReclaimRoleRequestResult::ReadyLocked;
	}

	if (CurrentRole == DesiredRole)
	{
		return EReclaimRoleRequestResult::Success;
	}

	if (bDesiredRoleOccupiedByOther)
	{
		return EReclaimRoleRequestResult::RoleOccupied;
	}

	return EReclaimRoleRequestResult::Success;
}
