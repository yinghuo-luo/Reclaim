// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ReclaimPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameModes/ReclaimLobbyGameMode.h"
#include "GameModes/ReclaimLobbyGameState.h"
#include "Player/ReclaimPlayerState.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "ReclaimPlayerController"

namespace
{
	const TCHAR* DefaultLobbyWidgetClassPath = TEXT("/Game/Reclaim/UI/Lobby/WBP_Lobby.WBP_Lobby_C");
}

void AReclaimPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && GetWorld() && GetWorld()->GetGameState<AReclaimLobbyGameState>())
	{
		EnsureLobbyWidget();
	}
}

void AReclaimPlayerController::EnsureLobbyWidget()
{
	UClass* WidgetClass = LobbyWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		WidgetClass = LoadClass<UUserWidget>(nullptr, DefaultLobbyWidgetClassPath);
	}

	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Reclaim Lobby] WBP_Lobby could not be loaded from %s."), DefaultLobbyWidgetClassPath);
		return;
	}

	if (!LobbyWidget)
	{
		LobbyWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	}

	if (!LobbyWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[Reclaim Lobby] Failed to create the lobby widget."));
		return;
	}

	if (!LobbyWidget->IsInViewport())
	{
		LobbyWidget->AddToViewport(1000);
	}

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AReclaimPlayerController::RequestRole(EReclaimRole DesiredRole)
{
	Server_RequestRole(DesiredRole);
}

void AReclaimPlayerController::SetReady(bool bNewReady)
{
	Server_SetReady(bNewReady);
}

void AReclaimPlayerController::RequestStartMission()
{
	Server_RequestStartMission();
}

void AReclaimPlayerController::EnterMissionInputMode()
{
	if (!IsLocalController())
	{
		return;
	}

	if (LobbyWidget)
	{
		LobbyWidget->RemoveFromParent();
		LobbyWidget = nullptr;
	}

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetInputMode(FInputModeGameOnly());
}

void AReclaimPlayerController::Server_RequestRole_Implementation(EReclaimRole DesiredRole)
{
	EReclaimRoleRequestResult Result = EReclaimRoleRequestResult::NotInLobby;

	if (UWorld* World = GetWorld())
	{
		if (AReclaimLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<AReclaimLobbyGameMode>())
		{
			Result = LobbyGameMode->RequestRole(GetPlayerState<AReclaimPlayerState>(), DesiredRole);
		}
	}

	if (Result == EReclaimRoleRequestResult::Success)
	{
		SelectedRoleForTravel = DesiredRole;
	}

	Client_RoleRequestResult(Result, DesiredRole);
}

void AReclaimPlayerController::Server_SetReady_Implementation(bool bNewReady)
{
	bool bSucceeded = false;
	FText FailureReason = LOCTEXT("ReadyNoLobby", "Not in Lobby.");

	if (UWorld* World = GetWorld())
	{
		if (AReclaimLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<AReclaimLobbyGameMode>())
		{
			bSucceeded = LobbyGameMode->SetPlayerReady(GetPlayerState<AReclaimPlayerState>(), bNewReady, FailureReason);
		}
	}

	Client_ReadyRequestResult(bSucceeded, FailureReason);
}

void AReclaimPlayerController::Server_RequestStartMission_Implementation()
{
	bool bSucceeded = false;
	FText FailureReason = LOCTEXT("StartNoLobby", "Not in Lobby.");

	if (!IsLocalController())
	{
		FailureReason = LOCTEXT("StartHostOnly", "Only the listen-server Host can start the mission.");
		Client_StartMissionRequestResult(false, FailureReason);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (AReclaimLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<AReclaimLobbyGameMode>())
		{
			bSucceeded = LobbyGameMode->CanStartMission(FailureReason);
			if (bSucceeded)
			{
				LobbyGameMode->StartMission();
			}
		}
	}

	Client_StartMissionRequestResult(bSucceeded, FailureReason);
}

void AReclaimPlayerController::Client_RoleRequestResult_Implementation(EReclaimRoleRequestResult Result, EReclaimRole DesiredRole)
{
	LastRoleRequestResult = Result;
}

void AReclaimPlayerController::Client_ReadyRequestResult_Implementation(bool bSucceeded, const FText& FailureReason)
{
	bLastReadyRequestSucceeded = bSucceeded;
	LastReadyFailureReason = bSucceeded ? FText::GetEmpty() : FailureReason;
}

void AReclaimPlayerController::Client_StartMissionRequestResult_Implementation(bool bSucceeded, const FText& FailureReason)
{
	bLastStartMissionRequestSucceeded = bSucceeded;
	LastStartMissionFailureReason = bSucceeded ? FText::GetEmpty() : FailureReason;
}

#undef LOCTEXT_NAMESPACE
