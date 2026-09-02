// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/ReclaimLobbyGameState.h"

#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"

AReclaimLobbyGameState::AReclaimLobbyGameState()
{
	bReplicates = true;
	ResetReservations_Server();
}

void AReclaimLobbyGameState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && RoleReservations.Num() != 4)
	{
		ResetReservations_Server();
	}
}

void AReclaimLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimLobbyGameState, SessionPhase);
	DOREPLIFETIME(AReclaimLobbyGameState, RoleReservations);
	DOREPLIFETIME(AReclaimLobbyGameState, SelectedMissionId);
}

void AReclaimLobbyGameState::SetSessionPhase_Server(EReclaimSessionPhase NewPhase)
{
	check(HasAuthority());
	SessionPhase = NewPhase;
}

void AReclaimLobbyGameState::SetSelectedMissionId_Server(FName NewMissionId)
{
	check(HasAuthority());
	SelectedMissionId = NewMissionId;
}

void AReclaimLobbyGameState::ResetReservations_Server()
{
	RoleReservations.Reset();

	const EReclaimRole Roles[] =
	{
		EReclaimRole::Vanguard,
		EReclaimRole::Ranger,
		EReclaimRole::Engineer,
		EReclaimRole::Warden
	};

	for (EReclaimRole ReservationRole : Roles)
	{
		FReclaimRoleReservation Reservation;
		Reservation.Role = ReservationRole;
		RoleReservations.Add(Reservation);
	}
}

bool AReclaimLobbyGameState::IsRoleAvailable(EReclaimRole DesiredRole, const AReclaimPlayerState* RequestingPlayer) const
{
	const AReclaimPlayerState* ReservationOwner = GetReservationOwner(DesiredRole);
	return ReservationOwner == nullptr || ReservationOwner == RequestingPlayer;
}

AReclaimPlayerState* AReclaimLobbyGameState::GetReservationOwner(EReclaimRole DesiredRole) const
{
	for (const FReclaimRoleReservation& Reservation : RoleReservations)
	{
		if (Reservation.Role == DesiredRole)
		{
			return Reservation.Owner;
		}
	}

	return nullptr;
}

void AReclaimLobbyGameState::SetRoleReservation_Server(EReclaimRole DesiredRole, AReclaimPlayerState* NewOwner)
{
	check(HasAuthority());

	for (FReclaimRoleReservation& Reservation : RoleReservations)
	{
		if (Reservation.Role == DesiredRole)
		{
			Reservation.Owner = NewOwner;
			return;
		}
	}
}

void AReclaimLobbyGameState::ReleaseReservationFor_Server(AReclaimPlayerState* PlayerState)
{
	check(HasAuthority());

	if (!PlayerState)
	{
		return;
	}

	for (FReclaimRoleReservation& Reservation : RoleReservations)
	{
		if (Reservation.Owner == PlayerState)
		{
			Reservation.Owner = nullptr;
		}
	}
}

int32 AReclaimLobbyGameState::GetReadyCount() const
{
	int32 ReadyCount = 0;
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (const AReclaimPlayerState* ReclaimPlayerState = Cast<AReclaimPlayerState>(PlayerState))
		{
			ReadyCount += ReclaimPlayerState->IsReady() ? 1 : 0;
		}
	}
	return ReadyCount;
}

bool AReclaimLobbyGameState::ValidateRoleInvariants() const
{
	TSet<EReclaimRole> SeenRoles;
	TSet<const AReclaimPlayerState*> SeenOwners;

	for (const FReclaimRoleReservation& Reservation : RoleReservations)
	{
		if (Reservation.Role == EReclaimRole::None || SeenRoles.Contains(Reservation.Role))
		{
			return false;
		}
		SeenRoles.Add(Reservation.Role);

		if (Reservation.Owner)
		{
			if (SeenOwners.Contains(Reservation.Owner))
			{
				return false;
			}
			SeenOwners.Add(Reservation.Owner);
		}
	}

	return RoleReservations.Num() == 4;
}

void AReclaimLobbyGameState::OnRep_SessionPhase()
{
}

void AReclaimLobbyGameState::OnRep_RoleReservations()
{
}
