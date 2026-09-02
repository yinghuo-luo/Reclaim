// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/ReclaimLobbyGameMode.h"

#include "Core/ReclaimRoleReservationRules.h"
#include "Engine/GameInstance.h"
#include "GameModes/ReclaimLobbyGameState.h"
#include "Misc/PackageName.h"
#include "Network/ReclaimSessionSubsystem.h"
#include "Player/ReclaimPlayerController.h"
#include "Player/ReclaimPlayerState.h"

#define LOCTEXT_NAMESPACE "ReclaimLobbyGameMode"

AReclaimLobbyGameMode::AReclaimLobbyGameMode()
{
	GameStateClass = AReclaimLobbyGameState::StaticClass();
	PlayerStateClass = AReclaimPlayerState::StaticClass();
	PlayerControllerClass = AReclaimPlayerController::StaticClass();
	bUseSeamlessTravel = true;
}

void AReclaimLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (AReclaimLobbyGameState* LobbyGameState = GetGameState<AReclaimLobbyGameState>())
	{
		LobbyGameState->SetSessionPhase_Server(EReclaimSessionPhase::Lobby);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UReclaimSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UReclaimSessionSubsystem>())
		{
			SessionSubsystem->UpdateSessionPhase(EReclaimSessionPhase::Lobby);
		}
	}
}

void AReclaimLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AReclaimPlayerState* ReclaimPlayerState = NewPlayer ? NewPlayer->GetPlayerState<AReclaimPlayerState>() : nullptr)
	{
		ReclaimPlayerState->SetPlayerSlot_Server(AllocatePlayerSlot());
		ReclaimPlayerState->SetReady_Server(false);
	}
}

void AReclaimLobbyGameMode::Logout(AController* Exiting)
{
	if (AReclaimPlayerState* ReclaimPlayerState = Exiting ? Exiting->GetPlayerState<AReclaimPlayerState>() : nullptr)
	{
		ReleaseRoleFor(ReclaimPlayerState);
	}

	Super::Logout(Exiting);
}

EReclaimRoleRequestResult AReclaimLobbyGameMode::RequestRole(AReclaimPlayerState* PlayerState, EReclaimRole DesiredRole)
{
	AReclaimLobbyGameState* LobbyGameState = GetGameState<AReclaimLobbyGameState>();
	const bool bInLobby = LobbyGameState && LobbyGameState->GetSessionPhase() == EReclaimSessionPhase::Lobby;
	const bool bPlayerFound = PlayerState && IsLobbyPlayer(PlayerState);
	const bool bDesiredRoleOccupiedByOther = bInLobby && bPlayerFound && !LobbyGameState->IsRoleAvailable(DesiredRole, PlayerState);
	const EReclaimRole CurrentRole = PlayerState ? PlayerState->GetSelectedRole() : EReclaimRole::None;
	const bool bReady = PlayerState && PlayerState->IsReady();
	const EReclaimRoleRequestResult Precheck = UReclaimRoleReservationRules::EvaluateRoleRequest(CurrentRole, DesiredRole, bReady, bInLobby, bPlayerFound, bDesiredRoleOccupiedByOther);
	if (Precheck != EReclaimRoleRequestResult::Success || CurrentRole == DesiredRole)
	{
		return Precheck;
	}

	LobbyGameState->ReleaseReservationFor_Server(PlayerState);
	LobbyGameState->SetRoleReservation_Server(DesiredRole, PlayerState);
	PlayerState->SetSelectedRole_Server(DesiredRole);

	ensureMsgf(LobbyGameState->ValidateRoleInvariants(), TEXT("Lobby role reservation invariant failed."));
	return EReclaimRoleRequestResult::Success;
}

bool AReclaimLobbyGameMode::SetPlayerReady(AReclaimPlayerState* PlayerState, bool bNewReady, FText& OutFailureReason)
{
	AReclaimLobbyGameState* LobbyGameState = GetGameState<AReclaimLobbyGameState>();
	if (!LobbyGameState || LobbyGameState->GetSessionPhase() != EReclaimSessionPhase::Lobby)
	{
		OutFailureReason = LOCTEXT("NotInLobby", "Not in Lobby.");
		return false;
	}

	if (!PlayerState || !IsLobbyPlayer(PlayerState))
	{
		OutFailureReason = LOCTEXT("PlayerNotFound", "Player is not in this Lobby.");
		return false;
	}

	if (bNewReady && PlayerState->GetSelectedRole() == EReclaimRole::None)
	{
		OutFailureReason = LOCTEXT("RoleRequired", "Select a role before Ready.");
		return false;
	}

	PlayerState->SetReady_Server(bNewReady);
	return true;
}

bool AReclaimLobbyGameMode::CanStartMission(FText& OutFailureReason) const
{
	const AReclaimLobbyGameState* LobbyGameState = GetGameState<AReclaimLobbyGameState>();
	if (!LobbyGameState || LobbyGameState->GetSessionPhase() != EReclaimSessionPhase::Lobby)
	{
		OutFailureReason = LOCTEXT("CannotStartNotLobby", "Lobby is not joinable.");
		return false;
	}

	int32 ParticipatingPlayers = 0;
	for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
	{
		const AReclaimPlayerState* ReclaimPlayerState = Cast<AReclaimPlayerState>(PlayerState);
		if (!ReclaimPlayerState)
		{
			continue;
		}

		++ParticipatingPlayers;
		if (ReclaimPlayerState->GetSelectedRole() == EReclaimRole::None)
		{
			OutFailureReason = LOCTEXT("CannotStartMissingRole", "Every player must select a role.");
			return false;
		}

		if (!ReclaimPlayerState->IsReady())
		{
			OutFailureReason = LOCTEXT("CannotStartNotReady", "Every player must be Ready.");
			return false;
		}
	}

	if (ParticipatingPlayers < 1 || ParticipatingPlayers > 4)
	{
		OutFailureReason = LOCTEXT("CannotStartPlayerCount", "Lobby requires one to four players.");
		return false;
	}

	if (!LobbyGameState->ValidateRoleInvariants())
	{
		OutFailureReason = LOCTEXT("CannotStartRoleConflict", "Role reservation state is invalid.");
		return false;
	}

	return true;
}

void AReclaimLobbyGameMode::StartMission()
{
	FText FailureReason;
	if (!CanStartMission(FailureReason))
	{
		UE_LOG(LogGameMode, Warning, TEXT("StartMission rejected: %s"), *FailureReason.ToString());
		return;
	}

	AReclaimLobbyGameState* LobbyGameState = GetGameState<AReclaimLobbyGameState>();
	if (!LobbyGameState)
	{
		return;
	}

	LobbyGameState->SetSessionPhase_Server(EReclaimSessionPhase::Travelling);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UReclaimSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UReclaimSessionSubsystem>())
		{
			SessionSubsystem->UpdateSessionPhase(EReclaimSessionPhase::Travelling);
		}
	}

	if (MissionMap.IsNull())
	{
		UE_LOG(LogGameMode, Warning, TEXT("MissionMap is not configured; server travel was not attempted."));
		return;
	}

	const FString MissionPackageName = MissionMap.GetLongPackageName();
	if (MissionPackageName.IsEmpty() || !FPackageName::IsValidLongPackageName(MissionPackageName))
	{
		UE_LOG(LogGameMode, Warning, TEXT("MissionMap is invalid; expected a level package path, got '%s'."), *MissionMap.ToString());
		return;
	}

	GetWorld()->ServerTravel(MissionPackageName, true);
}

void AReclaimLobbyGameMode::ReleaseRoleFor(AReclaimPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	if (AReclaimLobbyGameState* LobbyGameState = GetGameState<AReclaimLobbyGameState>())
	{
		LobbyGameState->ReleaseReservationFor_Server(PlayerState);
		PlayerState->SetSelectedRole_Server(EReclaimRole::None);
		PlayerState->SetReady_Server(false);
		ensureMsgf(LobbyGameState->ValidateRoleInvariants(), TEXT("Lobby role reservation invariant failed after release."));
	}
}

int32 AReclaimLobbyGameMode::AllocatePlayerSlot() const
{
	const AGameStateBase* CurrentGameState = GameState;
	if (!CurrentGameState)
	{
		return INDEX_NONE;
	}

	bool UsedSlots[4] = { false, false, false, false };
	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		if (const AReclaimPlayerState* ReclaimPlayerState = Cast<AReclaimPlayerState>(PlayerState))
		{
			const int32 Slot = ReclaimPlayerState->GetPlayerSlot();
			if (Slot >= 0 && Slot < 4)
			{
				UsedSlots[Slot] = true;
			}
		}
	}

	for (int32 Slot = 0; Slot < 4; ++Slot)
	{
		if (!UsedSlots[Slot])
		{
			return Slot;
		}
	}

	return INDEX_NONE;
}

bool AReclaimLobbyGameMode::IsLobbyPlayer(const AReclaimPlayerState* PlayerState) const
{
	const AGameStateBase* CurrentGameState = GameState;
	return PlayerState && CurrentGameState && CurrentGameState->PlayerArray.Contains(const_cast<AReclaimPlayerState*>(PlayerState));
}

#undef LOCTEXT_NAMESPACE
