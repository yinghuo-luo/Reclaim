// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ReclaimUIWidgets.h"

#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Components/ReclaimRunInventoryComponent.h"
#include "Debug/ReclaimDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameModes/ReclaimLobbyGameMode.h"
#include "GameModes/ReclaimLobbyGameState.h"
#include "GameModes/ReclaimMissionGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReclaimPlayerController.h"
#include "Player/ReclaimPlayerState.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "ReclaimUIWidgets"

namespace
{
	const TCHAR* DefaultSessionBrowserWidgetClassPath = TEXT("/Game/Reclaim/UI/Menu/WBP_SessionBrowser.WBP_SessionBrowser_C");

	void SetText(UTextBlock* TextBlock, const FText& Text)
	{
		if (TextBlock)
		{
			TextBlock->SetText(Text);
		}
	}

	void SetButtonEnabled(UButton* Button, bool bEnabled)
	{
		if (Button)
		{
			Button->SetIsEnabled(bEnabled);
		}
	}

	UReclaimSessionSubsystem* GetSessionSubsystem(const UUserWidget* Widget)
	{
		UGameInstance* GameInstance = Widget ? Widget->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UReclaimSessionSubsystem>() : nullptr;
	}

	AReclaimPlayerController* GetReclaimOwningPlayer(const UUserWidget* Widget)
	{
		return Widget ? Cast<AReclaimPlayerController>(Widget->GetOwningPlayer()) : nullptr;
	}

	AReclaimPlayerState* GetReclaimOwningPlayerState(const UUserWidget* Widget)
	{
		const AReclaimPlayerController* PlayerController = GetReclaimOwningPlayer(Widget);
		return PlayerController ? PlayerController->GetPlayerState<AReclaimPlayerState>() : nullptr;
	}

	FText RoleToText(EReclaimRole Role)
	{
		switch (Role)
		{
		case EReclaimRole::Vanguard:
			return LOCTEXT("RoleVanguard", "Vanguard");
		case EReclaimRole::Ranger:
			return LOCTEXT("RoleRanger", "Ranger");
		case EReclaimRole::Engineer:
			return LOCTEXT("RoleEngineer", "Engineer");
		case EReclaimRole::Warden:
			return LOCTEXT("RoleWarden", "Warden");
		case EReclaimRole::None:
		default:
			return LOCTEXT("RoleNone", "None");
		}
	}

	FText SessionPhaseToText(EReclaimSessionPhase Phase)
	{
		switch (Phase)
		{
		case EReclaimSessionPhase::MainMenu:
			return LOCTEXT("SessionMainMenu", "Main Menu");
		case EReclaimSessionPhase::Lobby:
			return LOCTEXT("SessionLobby", "Lobby");
		case EReclaimSessionPhase::Travelling:
			return LOCTEXT("SessionTravelling", "Travelling");
		case EReclaimSessionPhase::Mission:
			return LOCTEXT("SessionMission", "Mission");
		case EReclaimSessionPhase::Result:
			return LOCTEXT("SessionResult", "Result");
		case EReclaimSessionPhase::ReturningLobby:
			return LOCTEXT("SessionReturningLobby", "Returning Lobby");
		default:
			return LOCTEXT("SessionUnknown", "Unknown");
		}
	}

	FText MissionPhaseToText(EReclaimMissionPhase Phase)
	{
		switch (Phase)
		{
		case EReclaimMissionPhase::None:
			return LOCTEXT("MissionNone", "None");
		case EReclaimMissionPhase::Landing:
			return LOCTEXT("MissionLanding", "Landing");
		case EReclaimMissionPhase::Node1:
			return LOCTEXT("MissionNode1", "Node 1");
		case EReclaimMissionPhase::Node2:
			return LOCTEXT("MissionNode2", "Node 2");
		case EReclaimMissionPhase::Node3:
			return LOCTEXT("MissionNode3", "Node 3");
		case EReclaimMissionPhase::RootNest:
			return LOCTEXT("MissionRootNest", "Root Nest");
		case EReclaimMissionPhase::ExtractionUnlocked:
			return LOCTEXT("MissionExtractionUnlocked", "Extraction Unlocked");
		case EReclaimMissionPhase::Extracting:
			return LOCTEXT("MissionExtracting", "Extracting");
		case EReclaimMissionPhase::Succeeded:
			return LOCTEXT("MissionSucceeded", "Succeeded");
		case EReclaimMissionPhase::Failed:
			return LOCTEXT("MissionFailed", "Failed");
		default:
			return LOCTEXT("MissionUnknown", "Unknown");
		}
	}

	FText LifeStateToText(EReclaimPlayerLifeState LifeState)
	{
		switch (LifeState)
		{
		case EReclaimPlayerLifeState::Active:
			return LOCTEXT("LifeActive", "Active");
		case EReclaimPlayerLifeState::Downed:
			return LOCTEXT("LifeDowned", "Downed");
		case EReclaimPlayerLifeState::Destroyed:
			return LOCTEXT("LifeDestroyed", "Destroyed");
		case EReclaimPlayerLifeState::Redeploying:
			return LOCTEXT("LifeRedeploying", "Redeploying");
		default:
			return LOCTEXT("LifeUnknown", "Unknown");
		}
	}

	FText RoleRequestResultToText(EReclaimRoleRequestResult Result)
	{
		switch (Result)
		{
		case EReclaimRoleRequestResult::Success:
			return LOCTEXT("RoleRequestSuccess", "Role request: Success");
		case EReclaimRoleRequestResult::InvalidRole:
			return LOCTEXT("RoleRequestInvalidRole", "Role request: Invalid role");
		case EReclaimRoleRequestResult::NotInLobby:
			return LOCTEXT("RoleRequestNotLobby", "Role request: Not in Lobby");
		case EReclaimRoleRequestResult::PlayerNotFound:
			return LOCTEXT("RoleRequestPlayerNotFound", "Role request: Player not found");
		case EReclaimRoleRequestResult::ReadyLocked:
			return LOCTEXT("RoleRequestReadyLocked", "Role request: Ready locked");
		case EReclaimRoleRequestResult::RoleOccupied:
			return LOCTEXT("RoleRequestOccupied", "Role request: Role occupied");
		case EReclaimRoleRequestResult::MissionLocked:
			return LOCTEXT("RoleRequestMissionLocked", "Role request: Mission locked");
		case EReclaimRoleRequestResult::InternalConflict:
			return LOCTEXT("RoleRequestInternalConflict", "Role request: Internal conflict");
		default:
			return LOCTEXT("RoleRequestUnknown", "Role request: Unknown");
		}
	}

	FString AbilitySummaryForRole(EReclaimRole Role)
	{
		switch (Role)
		{
		case EReclaimRole::Vanguard:
			return TEXT("Dash / Kinetic Barrier");
		case EReclaimRole::Ranger:
			return TEXT("Tactical Scan / Mag-Rail Leap");
		case EReclaimRole::Engineer:
			return TEXT("Repair Drone / Overclock");
		case EReclaimRole::Warden:
			return TEXT("Purification Pulse / Stability Field");
		case EReclaimRole::None:
		default:
			return TEXT("No role selected");
		}
	}

	FString ReadyToString(bool bReady)
	{
		return bReady ? TEXT("Ready") : TEXT("Not Ready");
	}

	FString PlayerLine(const AReclaimPlayerState* PlayerState)
	{
		if (!PlayerState)
		{
			return TEXT("Empty");
		}

		return FString::Printf(
			TEXT("Slot %d | %s | %s | %s"),
			PlayerState->GetPlayerSlot(),
			*PlayerState->GetPlayerName(),
			*RoleToText(PlayerState->GetSelectedRole()).ToString(),
			*ReadyToString(PlayerState->IsReady()));
	}

	FString ResourceBundleToString(const FReclaimResourceBundle& Resources)
	{
		return FString::Printf(
			TEXT("Alloy %d | Crystal %d | Biopolymer %d | Anomaly %d"),
			Resources.Alloy,
			Resources.Crystal,
			Resources.Biopolymer,
			Resources.AnomalyCore);
	}

	FText CandidateToText(const FReclaimRewardCandidate& Candidate)
	{
		return FText::FromString(FString::Printf(
			TEXT("%s [%s]"),
			*Candidate.CandidateId.ToString(),
			*Candidate.BuildTag.ToString()));
	}

	template<typename UserClass>
	void StartRefreshTimer(UserClass* Widget, FTimerHandle& TimerHandle, float Interval, void (UserClass::*Callback)())
	{
		if (UWorld* World = Widget ? Widget->GetWorld() : nullptr)
		{
			World->GetTimerManager().SetTimer(TimerHandle, Widget, Callback, FMath::Max(0.05f, Interval), true);
		}
	}

	void ClearRefreshTimer(const UUserWidget* Widget, FTimerHandle& TimerHandle)
	{
		if (UWorld* World = Widget ? Widget->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
}

void UReclaimMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UReclaimMainMenuWidget::HandleHostClicked);
	}
	if (FindButton)
	{
		FindButton->OnClicked.AddDynamic(this, &UReclaimMainMenuWidget::HandleFindClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UReclaimMainMenuWidget::HandleSettingsClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UReclaimMainMenuWidget::HandleQuitClicked);
	}
}

void UReclaimMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		SessionSubsystem->OnHostSessionComplete.AddDynamic(this, &UReclaimMainMenuWidget::HandleHostSessionComplete);
		SessionSubsystem->OnFindSessionsComplete.AddDynamic(this, &UReclaimMainMenuWidget::HandleFindSessionsComplete);
	}

	SetStatusText(LOCTEXT("MainMenuReady", "Ready"));
}

void UReclaimMainMenuWidget::NativeDestruct()
{
	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		SessionSubsystem->OnHostSessionComplete.RemoveDynamic(this, &UReclaimMainMenuWidget::HandleHostSessionComplete);
		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &UReclaimMainMenuWidget::HandleFindSessionsComplete);
	}

	Super::NativeDestruct();
}

void UReclaimMainMenuWidget::HandleHostClicked()
{
	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		SetStatusText(LOCTEXT("Hosting", "Creating LAN session..."));
		SessionSubsystem->HostSession(HostMaxPlayers, bLANSession);
		return;
	}

	SetStatusText(LOCTEXT("NoSessionSubsystem", "Session subsystem is unavailable."));
}

void UReclaimMainMenuWidget::HandleFindClicked()
{
	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		ShowSessionBrowser();
		SetStatusText(LOCTEXT("FindingSessions", "Searching for LAN sessions..."));
		SessionSubsystem->FindSessions(50, bLANSession);
		return;
	}

	SetStatusText(LOCTEXT("NoSessionSubsystemFind", "Session subsystem is unavailable."));
}

void UReclaimMainMenuWidget::ShowSessionBrowser()
{
	UClass* WidgetClass = SessionBrowserWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		WidgetClass = LoadClass<UUserWidget>(nullptr, DefaultSessionBrowserWidgetClassPath);
	}

	if (!WidgetClass)
	{
		SetStatusText(LOCTEXT("SessionBrowserMissing", "Session Browser is not configured."));
		UE_LOG(LogTemp, Error, TEXT("[Reclaim MainMenu] WBP_SessionBrowser could not be loaded from %s."), DefaultSessionBrowserWidgetClassPath);
		return;
	}

	if (!SessionBrowserWidget)
	{
		SessionBrowserWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	}

	if (!SessionBrowserWidget)
	{
		SetStatusText(LOCTEXT("SessionBrowserCreateFailed", "Session Browser could not be created."));
		return;
	}

	if (!SessionBrowserWidget->IsInViewport())
	{
		SessionBrowserWidget->AddToViewport(1100);
	}
}

void UReclaimMainMenuWidget::HandleSettingsClicked()
{
	SetStatusText(LOCTEXT("SettingsUnavailable", "Settings are not configured for M0."));
}

void UReclaimMainMenuWidget::HandleQuitClicked()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->ConsoleCommand(TEXT("quit"));
	}
}

void UReclaimMainMenuWidget::HandleHostSessionComplete(bool bSucceeded)
{
	if (bSucceeded)
	{
		SetStatusText(LOCTEXT("HostSucceeded", "Session created. Opening Lobby..."));
		TravelToLobbyAsHost();
		return;
	}

	SetStatusText(LOCTEXT("HostFailed", "Session creation failed."));
}

void UReclaimMainMenuWidget::HandleFindSessionsComplete(bool bSucceeded, const TArray<FReclaimSessionResult>& Results)
{
	if (!bSucceeded)
	{
		SetStatusText(LOCTEXT("FindFailed", "Session search failed."));
		return;
	}

	int32 JoinableCount = 0;
	for (const FReclaimSessionResult& Result : Results)
	{
		JoinableCount += Result.bJoinable ? 1 : 0;
	}

	SetStatusText(FText::Format(LOCTEXT("FindSucceededFormat", "Found {0} session(s), {1} joinable."), FText::AsNumber(Results.Num()), FText::AsNumber(JoinableCount)));
}

void UReclaimMainMenuWidget::SetStatusText(const FText& NewStatus) const
{
	SetText(StatusText, NewStatus);
}

void UReclaimMainMenuWidget::TravelToLobbyAsHost()
{
	if (LobbyMap.IsNull())
	{
		SetStatusText(LOCTEXT("LobbyMapMissing", "LobbyMap is not configured on this widget."));
		return;
	}

	const FString LobbyPackageName = LobbyMap.GetLongPackageName();
	if (LobbyPackageName.IsEmpty())
	{
		SetStatusText(LOCTEXT("LobbyMapInvalid", "LobbyMap does not resolve to a level package."));
		return;
	}

	UGameplayStatics::OpenLevel(this, FName(*LobbyPackageName), true, TEXT("listen"));
}

void UReclaimSessionBrowserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &UReclaimSessionBrowserWidget::HandleRefreshClicked);
	}
	if (JoinFirstButton)
	{
		JoinFirstButton->OnClicked.AddDynamic(this, &UReclaimSessionBrowserWidget::HandleJoinFirstClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UReclaimSessionBrowserWidget::HandleBackClicked);
	}
}

void UReclaimSessionBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		SessionSubsystem->OnFindSessionsComplete.AddDynamic(this, &UReclaimSessionBrowserWidget::HandleFindSessionsComplete);
		SessionSubsystem->OnJoinSessionComplete.AddDynamic(this, &UReclaimSessionBrowserWidget::HandleJoinSessionComplete);
	}

	RefreshSessionListText();
}

void UReclaimSessionBrowserWidget::NativeDestruct()
{
	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &UReclaimSessionBrowserWidget::HandleFindSessionsComplete);
		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &UReclaimSessionBrowserWidget::HandleJoinSessionComplete);
	}

	Super::NativeDestruct();
}

void UReclaimSessionBrowserWidget::SetSelectedSearchResultIndex(int32 SearchResultIndex)
{
	SelectedSearchResultIndex = SearchResultIndex;
	RefreshSessionListText();
}

void UReclaimSessionBrowserWidget::HandleRefreshClicked()
{
	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		SetText(StatusText, LOCTEXT("SessionBrowserSearching", "Searching..."));
		SessionSubsystem->FindSessions(MaxSearchResults, bLANQuery);
		return;
	}

	SetText(StatusText, LOCTEXT("SessionBrowserNoSubsystem", "Session subsystem is unavailable."));
}

void UReclaimSessionBrowserWidget::HandleJoinFirstClicked()
{
	const int32 SearchResultIndex = FindFirstJoinableSearchResultIndex();
	if (SearchResultIndex == INDEX_NONE)
	{
		SetText(StatusText, LOCTEXT("NoJoinableSession", "No joinable Lobby session selected."));
		return;
	}

	if (UReclaimSessionSubsystem* SessionSubsystem = GetSessionSubsystem(this))
	{
		SetText(StatusText, LOCTEXT("JoiningSession", "Joining session..."));
		SessionSubsystem->JoinSessionByIndex(SearchResultIndex);
		return;
	}

	SetText(StatusText, LOCTEXT("JoinNoSubsystem", "Session subsystem is unavailable."));
}

void UReclaimSessionBrowserWidget::HandleBackClicked()
{
	RemoveFromParent();
}

void UReclaimSessionBrowserWidget::HandleFindSessionsComplete(bool bSucceeded, const TArray<FReclaimSessionResult>& Results)
{
	CachedResults = Results;
	SetText(StatusText, bSucceeded ? LOCTEXT("SessionSearchComplete", "Search complete.") : LOCTEXT("SessionSearchFailed", "Search failed."));
	RefreshSessionListText();
}

void UReclaimSessionBrowserWidget::HandleJoinSessionComplete(bool bSucceeded)
{
	SetText(StatusText, bSucceeded ? LOCTEXT("JoinSucceeded", "Join request accepted.") : LOCTEXT("JoinFailed", "Join failed."));
}

void UReclaimSessionBrowserWidget::RefreshSessionListText()
{
	TArray<FString> Lines;
	for (const FReclaimSessionResult& Result : CachedResults)
	{
		if (!Result.bJoinable)
		{
			continue;
		}

		Lines.Add(FString::Printf(
			TEXT("[%d] %s | %d/%d | %d ms | %s"),
			Result.SearchResultIndex,
			*Result.HostDisplayName,
			Result.CurrentPlayers,
			Result.MaxPlayers,
			Result.PingInMs,
			*SessionPhaseToText(Result.Phase).ToString()));
	}

	SetButtonEnabled(JoinFirstButton, FindFirstJoinableSearchResultIndex() != INDEX_NONE);
	SetText(SessionListText, Lines.IsEmpty() ? LOCTEXT("NoSessionsListed", "No joinable Lobby sessions.") : FText::FromString(FString::Join(Lines, TEXT("\n"))));
}

int32 UReclaimSessionBrowserWidget::FindFirstJoinableSearchResultIndex() const
{
	for (const FReclaimSessionResult& Result : CachedResults)
	{
		if (Result.bJoinable && Result.SearchResultIndex == SelectedSearchResultIndex)
		{
			return Result.SearchResultIndex;
		}
	}

	for (const FReclaimSessionResult& Result : CachedResults)
	{
		if (Result.bJoinable)
		{
			return Result.SearchResultIndex;
		}
	}

	return INDEX_NONE;
}

void UReclaimLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (VanguardButton)
	{
		VanguardButton->OnClicked.AddDynamic(this, &UReclaimLobbyWidget::HandleVanguardClicked);
	}
	if (RangerButton)
	{
		RangerButton->OnClicked.AddDynamic(this, &UReclaimLobbyWidget::HandleRangerClicked);
	}
	if (EngineerButton)
	{
		EngineerButton->OnClicked.AddDynamic(this, &UReclaimLobbyWidget::HandleEngineerClicked);
	}
	if (WardenButton)
	{
		WardenButton->OnClicked.AddDynamic(this, &UReclaimLobbyWidget::HandleWardenClicked);
	}
	if (ReadyButton)
	{
		ReadyButton->OnClicked.AddDynamic(this, &UReclaimLobbyWidget::HandleReadyClicked);
	}
	if (StartMissionButton)
	{
		StartMissionButton->OnClicked.AddDynamic(this, &UReclaimLobbyWidget::HandleStartMissionClicked);
	}
}

void UReclaimLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshLobby();
	StartRefreshTimer(this, RefreshTimerHandle, RefreshInterval, &UReclaimLobbyWidget::RefreshLobby);
}

void UReclaimLobbyWidget::NativeDestruct()
{
	ClearRefreshTimer(this, RefreshTimerHandle);
	Super::NativeDestruct();
}

void UReclaimLobbyWidget::HandleVanguardClicked()
{
	RequestRole(EReclaimRole::Vanguard);
}

void UReclaimLobbyWidget::HandleRangerClicked()
{
	RequestRole(EReclaimRole::Ranger);
}

void UReclaimLobbyWidget::HandleEngineerClicked()
{
	RequestRole(EReclaimRole::Engineer);
}

void UReclaimLobbyWidget::HandleWardenClicked()
{
	RequestRole(EReclaimRole::Warden);
}

void UReclaimLobbyWidget::HandleReadyClicked()
{
	AReclaimPlayerController* PlayerController = GetReclaimOwningPlayer(this);
	const AReclaimPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (PlayerController && PlayerState)
	{
		PlayerController->SetReady(!PlayerState->IsReady());
	}
}

void UReclaimLobbyWidget::HandleStartMissionClicked()
{
	if (AReclaimPlayerController* PlayerController = GetReclaimOwningPlayer(this))
	{
		PlayerController->RequestStartMission();
	}
}

void UReclaimLobbyWidget::RequestRole(EReclaimRole DesiredRole)
{
	if (AReclaimPlayerController* PlayerController = GetReclaimOwningPlayer(this))
	{
		PlayerController->RequestRole(DesiredRole);
	}
}

void UReclaimLobbyWidget::RefreshLobby()
{
	UWorld* World = GetWorld();
	const AReclaimLobbyGameState* LobbyGameState = World ? World->GetGameState<AReclaimLobbyGameState>() : nullptr;
	const AReclaimPlayerState* LocalPlayerState = GetReclaimOwningPlayerState(this);
	AReclaimPlayerController* PlayerController = GetReclaimOwningPlayer(this);

	if (!LobbyGameState)
	{
		SetText(LobbyStatusText, LOCTEXT("NoLobbyGameState", "No Lobby GameState."));
		SetText(PlayerSlotsText, FText::GetEmpty());
		SetText(RoleReservationsText, FText::GetEmpty());
		RefreshRoleButtons(LocalPlayerState);
		return;
	}

	SetText(LobbyStatusText, FText::Format(
		LOCTEXT("LobbyStatusFormat", "Phase: {0} | Mission: {1} | Ready: {2}/{3}"),
		SessionPhaseToText(LobbyGameState->GetSessionPhase()),
		FText::FromName(LobbyGameState->GetSelectedMissionId()),
		FText::AsNumber(LobbyGameState->GetReadyCount()),
		FText::AsNumber(LobbyGameState->PlayerArray.Num())));

	TArray<FString> PlayerLines;
	for (const APlayerState* PlayerState : LobbyGameState->PlayerArray)
	{
		PlayerLines.Add(PlayerLine(Cast<AReclaimPlayerState>(PlayerState)));
	}
	SetText(PlayerSlotsText, PlayerLines.IsEmpty() ? LOCTEXT("NoLobbyPlayers", "No players.") : FText::FromString(FString::Join(PlayerLines, TEXT("\n"))));

	TArray<FString> ReservationLines;
	for (const FReclaimRoleReservation& Reservation : LobbyGameState->GetRoleReservations())
	{
		ReservationLines.Add(FString::Printf(
			TEXT("%s: %s"),
			*RoleToText(Reservation.Role).ToString(),
			Reservation.Owner ? *Reservation.Owner->GetPlayerName() : TEXT("Open")));
	}
	SetText(RoleReservationsText, ReservationLines.IsEmpty() ? LOCTEXT("NoReservations", "No reservations.") : FText::FromString(FString::Join(ReservationLines, TEXT("\n"))));

	FString LastResultLine = PlayerController ? RoleRequestResultToText(PlayerController->GetLastRoleRequestResult()).ToString() : TEXT("No PlayerController");
	if (PlayerController && !PlayerController->WasLastReadyRequestSuccessful())
	{
		LastResultLine += FString::Printf(TEXT("\nReady: %s"), *PlayerController->GetLastReadyFailureReason().ToString());
	}
	if (PlayerController && !PlayerController->WasLastStartMissionRequestSuccessful())
	{
		LastResultLine += FString::Printf(TEXT("\nStart: %s"), *PlayerController->GetLastStartMissionFailureReason().ToString());
	}
	SetText(LastRoleResultText, FText::FromString(LastResultLine));

	SetText(ReadyButtonText, LocalPlayerState && LocalPlayerState->IsReady() ? LOCTEXT("UnreadyButton", "Unready") : LOCTEXT("ReadyButton", "Ready"));
	SetText(StartMissionButtonText, LOCTEXT("StartMissionButton", "Start Mission"));

	const bool bCanToggleReady = LocalPlayerState && (LocalPlayerState->IsReady() || LocalPlayerState->GetSelectedRole() != EReclaimRole::None);
	SetButtonEnabled(ReadyButton, bCanToggleReady);

	bool bCanStart = false;
	if (PlayerController && PlayerController->HasAuthority() && PlayerController->IsLocalController())
	{
		if (const AReclaimLobbyGameMode* LobbyGameMode = World ? World->GetAuthGameMode<AReclaimLobbyGameMode>() : nullptr)
		{
			FText FailureReason;
			bCanStart = LobbyGameMode->CanStartMission(FailureReason);
		}
	}
	SetButtonEnabled(StartMissionButton, bCanStart);

	RefreshRoleButtons(LocalPlayerState);
}

void UReclaimLobbyWidget::RefreshRoleButtons(const AReclaimPlayerState* LocalPlayerState)
{
	SetButtonEnabled(VanguardButton, IsRoleSelectable(EReclaimRole::Vanguard, LocalPlayerState));
	SetButtonEnabled(RangerButton, IsRoleSelectable(EReclaimRole::Ranger, LocalPlayerState));
	SetButtonEnabled(EngineerButton, IsRoleSelectable(EReclaimRole::Engineer, LocalPlayerState));
	SetButtonEnabled(WardenButton, IsRoleSelectable(EReclaimRole::Warden, LocalPlayerState));
}

bool UReclaimLobbyWidget::IsRoleSelectable(EReclaimRole DesiredRole, const AReclaimPlayerState* LocalPlayerState) const
{
	if (!LocalPlayerState || LocalPlayerState->IsReady())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const AReclaimLobbyGameState* LobbyGameState = World ? World->GetGameState<AReclaimLobbyGameState>() : nullptr;
	return LobbyGameState && LobbyGameState->GetSessionPhase() == EReclaimSessionPhase::Lobby && LobbyGameState->IsRoleAvailable(DesiredRole, LocalPlayerState);
}

void UReclaimRoleCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UReclaimRoleCardWidget::HandleSelectClicked);
	}
}

void UReclaimRoleCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshRoleCard();
	StartRefreshTimer(this, RefreshTimerHandle, RefreshInterval, &UReclaimRoleCardWidget::RefreshRoleCard);
}

void UReclaimRoleCardWidget::NativeDestruct()
{
	ClearRefreshTimer(this, RefreshTimerHandle);
	Super::NativeDestruct();
}

void UReclaimRoleCardWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshRoleCard();
}

void UReclaimRoleCardWidget::SetRole(EReclaimRole NewRole)
{
	Role = NewRole;
	RefreshRoleCard();
}

void UReclaimRoleCardWidget::HandleSelectClicked()
{
	if (Role == EReclaimRole::None)
	{
		return;
	}

	if (AReclaimPlayerController* PlayerController = GetReclaimOwningPlayer(this))
	{
		PlayerController->RequestRole(Role);
	}
}

void UReclaimRoleCardWidget::RefreshRoleCard()
{
	const UWorld* World = GetWorld();
	const AReclaimLobbyGameState* LobbyGameState = World ? World->GetGameState<AReclaimLobbyGameState>() : nullptr;
	const AReclaimPlayerState* LocalPlayerState = GetReclaimOwningPlayerState(this);
	const AReclaimPlayerState* Owner = LobbyGameState ? LobbyGameState->GetReservationOwner(Role) : nullptr;
	const bool bSelectedByLocalPlayer = LocalPlayerState && Owner == LocalPlayerState;
	const bool bSelectable = Role != EReclaimRole::None && LocalPlayerState && !LocalPlayerState->IsReady() && (!Owner || bSelectedByLocalPlayer);

	SetText(RoleNameText, RoleToText(Role));
	SetText(AbilitySummaryText, FText::FromString(AbilitySummaryForRole(Role)));

	if (Role == EReclaimRole::None)
	{
		SetText(RoleStatusText, LOCTEXT("RoleCardNoRole", "No role assigned."));
	}
	else if (bSelectedByLocalPlayer)
	{
		SetText(RoleStatusText, LOCTEXT("RoleCardSelected", "Selected"));
	}
	else if (Owner)
	{
		SetText(RoleStatusText, FText::Format(LOCTEXT("RoleCardTakenFormat", "Taken by {0}"), FText::FromString(Owner->GetPlayerName())));
	}
	else if (LocalPlayerState && LocalPlayerState->IsReady())
	{
		SetText(RoleStatusText, LOCTEXT("RoleCardReadyLocked", "Ready locked"));
	}
	else
	{
		SetText(RoleStatusText, LOCTEXT("RoleCardAvailable", "Available"));
	}

	SetButtonEnabled(SelectButton, bSelectable);
}

void UReclaimPlayerSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshPlayerSlot();
	StartRefreshTimer(this, RefreshTimerHandle, RefreshInterval, &UReclaimPlayerSlotWidget::RefreshPlayerSlot);
}

void UReclaimPlayerSlotWidget::NativeDestruct()
{
	ClearRefreshTimer(this, RefreshTimerHandle);
	Super::NativeDestruct();
}

void UReclaimPlayerSlotWidget::SetObservedPlayerState(AReclaimPlayerState* NewPlayerState)
{
	ObservedPlayerState = NewPlayerState;
	RefreshPlayerSlot();
}

void UReclaimPlayerSlotWidget::RefreshPlayerSlot()
{
	const AReclaimPlayerState* PlayerState = ResolveObservedPlayerState();
	if (!PlayerState)
	{
		SetText(PlayerNameText, LOCTEXT("PlayerSlotEmpty", "Empty"));
		SetText(SlotText, LOCTEXT("PlayerSlotNone", "Slot --"));
		SetText(RoleText, LOCTEXT("PlayerSlotNoRole", "Role: None"));
		SetText(ReadyText, LOCTEXT("PlayerSlotNotReady", "Not Ready"));
		SetText(LifeStateText, LOCTEXT("PlayerSlotNoLife", "Life: --"));
		return;
	}

	SetText(PlayerNameText, FText::FromString(PlayerState->GetPlayerName()));
	SetText(SlotText, FText::Format(LOCTEXT("PlayerSlotFormat", "Slot {0}"), FText::AsNumber(PlayerState->GetPlayerSlot())));
	SetText(RoleText, FText::Format(LOCTEXT("PlayerSlotRoleFormat", "Role: {0}"), RoleToText(PlayerState->GetSelectedRole())));
	SetText(ReadyText, FText::FromString(ReadyToString(PlayerState->IsReady())));
	SetText(LifeStateText, FText::Format(LOCTEXT("PlayerSlotLifeFormat", "Life: {0}"), LifeStateToText(PlayerState->GetLifeState())));
}

AReclaimPlayerState* UReclaimPlayerSlotWidget::ResolveObservedPlayerState() const
{
	return ObservedPlayerState ? ObservedPlayerState.Get() : GetReclaimOwningPlayerState(this);
}

void UReclaimHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshHUD();
	StartRefreshTimer(this, RefreshTimerHandle, RefreshInterval, &UReclaimHUDWidget::RefreshHUD);
}

void UReclaimHUDWidget::NativeDestruct()
{
	ClearRefreshTimer(this, RefreshTimerHandle);
	Super::NativeDestruct();
}

void UReclaimHUDWidget::RefreshHUD()
{
	const AReclaimPlayerController* PlayerController = GetReclaimOwningPlayer(this);
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UReclaimHealthShieldComponent* HealthShield = Pawn ? Pawn->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
	const AReclaimPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AReclaimPlayerState>() : nullptr;

	const float Health = HealthShield ? HealthShield->GetHealth() : 0.0f;
	const float MaxHealth = HealthShield ? HealthShield->GetMaxHealth() : 0.0f;
	const float Shield = HealthShield ? HealthShield->GetShield() : 0.0f;
	const float MaxShield = HealthShield ? HealthShield->GetMaxShield() : 0.0f;

	if (HealthBar)
	{
		HealthBar->SetPercent(MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f);
	}
	if (ShieldBar)
	{
		ShieldBar->SetPercent(MaxShield > 0.0f ? FMath::Clamp(Shield / MaxShield, 0.0f, 1.0f) : 0.0f);
	}

	SetText(HealthText, FText::Format(LOCTEXT("HealthFormat", "Health: {0}/{1}"), FText::AsNumber(FMath::RoundToInt(Health)), FText::AsNumber(FMath::RoundToInt(MaxHealth))));
	SetText(ShieldText, FText::Format(LOCTEXT("ShieldFormat", "Shield: {0}/{1}"), FText::AsNumber(FMath::RoundToInt(Shield)), FText::AsNumber(FMath::RoundToInt(MaxShield))));
	SetText(LifeStateText, FText::Format(LOCTEXT("HUDLifeFormat", "Life: {0}"), LifeStateToText(PlayerState ? PlayerState->GetLifeState() : EReclaimPlayerLifeState::Active)));

	const UWorld* World = GetWorld();
	const AReclaimMissionGameState* MissionGameState = World ? World->GetGameState<AReclaimMissionGameState>() : nullptr;
	SetText(MissionPhaseText, FText::Format(LOCTEXT("HUDMissionPhaseFormat", "Mission: {0}"), MissionPhaseToText(MissionGameState ? MissionGameState->GetMissionPhase() : EReclaimMissionPhase::None)));
	SetText(TeamResourcesText, FText::FromString(MissionGameState ? ResourceBundleToString(MissionGameState->GetTeamResources()) : TEXT("No mission resources")));

	const UReclaimRunInventoryComponent* RunInventory = PlayerState ? PlayerState->GetRunInventoryComponent() : nullptr;
	TArray<FString> ModifierIds;
	if (RunInventory)
	{
		for (const FName ModifierId : RunInventory->GetActiveModifierIds())
		{
			ModifierIds.Add(ModifierId.ToString());
		}
	}
	SetText(ActiveModifiersText, ModifierIds.IsEmpty() ? LOCTEXT("NoActiveModifiers", "Modifiers: None") : FText::FromString(TEXT("Modifiers: ") + FString::Join(ModifierIds, TEXT(", "))));

	TArray<FString> CandidateIds;
	if (RunInventory)
	{
		for (const FReclaimRewardCandidate& Candidate : RunInventory->GetPendingRewardCandidates())
		{
			CandidateIds.Add(Candidate.CandidateId.ToString());
		}
	}
	SetText(RewardCandidatesText, CandidateIds.IsEmpty() ? LOCTEXT("NoRewardCandidates", "Rewards: None") : FText::FromString(TEXT("Rewards: ") + FString::Join(CandidateIds, TEXT(", "))));
}

void UReclaimTeammateRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshTeammateRow();
	StartRefreshTimer(this, RefreshTimerHandle, RefreshInterval, &UReclaimTeammateRowWidget::RefreshTeammateRow);
}

void UReclaimTeammateRowWidget::NativeDestruct()
{
	ClearRefreshTimer(this, RefreshTimerHandle);
	Super::NativeDestruct();
}

void UReclaimTeammateRowWidget::SetObservedPlayerState(AReclaimPlayerState* NewPlayerState)
{
	ObservedPlayerState = NewPlayerState;
	RefreshTeammateRow();
}

void UReclaimTeammateRowWidget::RefreshTeammateRow()
{
	const AReclaimPlayerState* PlayerState = ResolveObservedPlayerState();
	if (!PlayerState)
	{
		SetText(PlayerNameText, LOCTEXT("TeammateEmpty", "Empty"));
		SetText(SlotText, LOCTEXT("TeammateSlotEmpty", "Slot --"));
		SetText(RoleText, LOCTEXT("TeammateRoleEmpty", "Role: None"));
		SetText(LifeStateText, LOCTEXT("TeammateLifeEmpty", "Life: --"));
		SetText(ReadyText, LOCTEXT("TeammateReadyEmpty", "Not Ready"));
		return;
	}

	SetText(PlayerNameText, FText::FromString(PlayerState->GetPlayerName()));
	SetText(SlotText, FText::Format(LOCTEXT("TeammateSlotFormat", "Slot {0}"), FText::AsNumber(PlayerState->GetPlayerSlot())));
	SetText(RoleText, FText::Format(LOCTEXT("TeammateRoleFormat", "Role: {0}"), RoleToText(PlayerState->GetSelectedRole())));
	SetText(LifeStateText, FText::Format(LOCTEXT("TeammateLifeFormat", "Life: {0}"), LifeStateToText(PlayerState->GetLifeState())));
	SetText(ReadyText, FText::FromString(ReadyToString(PlayerState->IsReady())));
}

AReclaimPlayerState* UReclaimTeammateRowWidget::ResolveObservedPlayerState() const
{
	return ObservedPlayerState ? ObservedPlayerState.Get() : GetReclaimOwningPlayerState(this);
}

void UReclaimRewardSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CandidateOneButton)
	{
		CandidateOneButton->OnClicked.AddDynamic(this, &UReclaimRewardSelectionWidget::HandleCandidateOneClicked);
	}
	if (CandidateTwoButton)
	{
		CandidateTwoButton->OnClicked.AddDynamic(this, &UReclaimRewardSelectionWidget::HandleCandidateTwoClicked);
	}
	if (CandidateThreeButton)
	{
		CandidateThreeButton->OnClicked.AddDynamic(this, &UReclaimRewardSelectionWidget::HandleCandidateThreeClicked);
	}
}

void UReclaimRewardSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshRewardSelection();
	StartRefreshTimer(this, RefreshTimerHandle, RefreshInterval, &UReclaimRewardSelectionWidget::RefreshRewardSelection);
}

void UReclaimRewardSelectionWidget::NativeDestruct()
{
	ClearRefreshTimer(this, RefreshTimerHandle);
	Super::NativeDestruct();
}

void UReclaimRewardSelectionWidget::HandleCandidateOneClicked()
{
	RequestCandidateSelection(0);
}

void UReclaimRewardSelectionWidget::HandleCandidateTwoClicked()
{
	RequestCandidateSelection(1);
}

void UReclaimRewardSelectionWidget::HandleCandidateThreeClicked()
{
	RequestCandidateSelection(2);
}

void UReclaimRewardSelectionWidget::RefreshRewardSelection()
{
	FReclaimRewardCandidate Candidate;
	const bool bHasOne = TryGetCandidate(0, Candidate);
	SetText(CandidateOneText, bHasOne ? CandidateToText(Candidate) : LOCTEXT("CandidateOneEmpty", "Reward 1: Empty"));
	SetButtonEnabled(CandidateOneButton, bHasOne);

	const bool bHasTwo = TryGetCandidate(1, Candidate);
	SetText(CandidateTwoText, bHasTwo ? CandidateToText(Candidate) : LOCTEXT("CandidateTwoEmpty", "Reward 2: Empty"));
	SetButtonEnabled(CandidateTwoButton, bHasTwo);

	const bool bHasThree = TryGetCandidate(2, Candidate);
	SetText(CandidateThreeText, bHasThree ? CandidateToText(Candidate) : LOCTEXT("CandidateThreeEmpty", "Reward 3: Empty"));
	SetButtonEnabled(CandidateThreeButton, bHasThree);

	if (!bHasOne && !bHasTwo && !bHasThree)
	{
		SetText(StatusText, LOCTEXT("NoPendingRewards", "Waiting for server reward candidates."));
	}
}

void UReclaimRewardSelectionWidget::RequestCandidateSelection(int32 CandidateIndex)
{
	FReclaimRewardCandidate Candidate;
	if (!TryGetCandidate(CandidateIndex, Candidate))
	{
		SetText(StatusText, LOCTEXT("InvalidRewardCandidate", "Reward candidate is unavailable."));
		return;
	}

	SetText(StatusText, FText::Format(LOCTEXT("RewardCandidateRequested", "Selection requested: {0}"), FText::FromName(Candidate.CandidateId)));
	OnCandidateSelectionRequested(Candidate.CandidateId);
}

bool UReclaimRewardSelectionWidget::TryGetCandidate(int32 CandidateIndex, FReclaimRewardCandidate& OutCandidate) const
{
	const AReclaimPlayerState* PlayerState = GetReclaimOwningPlayerState(this);
	const UReclaimRunInventoryComponent* RunInventory = PlayerState ? PlayerState->GetRunInventoryComponent() : nullptr;
	if (!RunInventory)
	{
		return false;
	}

	const TArray<FReclaimRewardCandidate>& Candidates = RunInventory->GetPendingRewardCandidates();
	if (!Candidates.IsValidIndex(CandidateIndex))
	{
		return false;
	}

	OutCandidate = Candidates[CandidateIndex];
	return true;
}

void UReclaimNetDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDebugText();
	StartRefreshTimer(this, RefreshTimerHandle, RefreshInterval, &UReclaimNetDebugWidget::RefreshDebugText);
}

void UReclaimNetDebugWidget::NativeDestruct()
{
	ClearRefreshTimer(this, RefreshTimerHandle);
	Super::NativeDestruct();
}

void UReclaimNetDebugWidget::RefreshDebugText()
{
	const UWorld* World = GetWorld();
	const UReclaimDebugSubsystem* DebugSubsystem = World ? World->GetSubsystem<UReclaimDebugSubsystem>() : nullptr;
	SetText(RoleDebugText, FText::FromString(DebugSubsystem ? DebugSubsystem->GetRoleDebugString() : TEXT("No debug subsystem")));
	SetText(MissionDebugText, FText::FromString(DebugSubsystem ? DebugSubsystem->GetMissionDebugString() : TEXT("No debug subsystem")));
}

#undef LOCTEXT_NAMESPACE
