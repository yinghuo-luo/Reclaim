// Copyright Epic Games, Inc. All Rights Reserved.

#include "Network/ReclaimSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Net/OnlineEngineInterface.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

namespace ReclaimSession
{
	const FName Name = NAME_GameSession;
	const FName NullSubsystemName(TEXT("NULL"));
	const FName PhaseKey(TEXT("RECLAIM_SESSION_PHASE"));
	const FName BuildKey(TEXT("RECLAIM_BUILD_SCHEMA"));
	const FString BuildSchema(TEXT("M0"));

	bool ResolveLANMode(bool bRequestedLAN, const IOnlineSubsystem* OnlineSubsystem)
	{
		const bool bIsNullSubsystem = OnlineSubsystem && OnlineSubsystem->GetSubsystemName() == NullSubsystemName;
		if (bIsNullSubsystem && !bRequestedLAN)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Reclaim Session] OnlineSubsystemNull has no internet session directory; forcing LAN discovery."));
		}

		return bRequestedLAN || bIsNullSubsystem;
	}
}

void UReclaimSessionSubsystem::Deinitialize()
{
	bRetryHostAfterDestroy = false;
	ClearSessionDelegates();
	Super::Deinitialize();
}

void UReclaimSessionSubsystem::HostSession(int32 MaxPlayers, bool bIsLAN)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSubsystem* OnlineSubsystem = GetWorld() ? Online::GetSubsystem(GetWorld()) : IOnlineSubsystem::Get();
	const bool bEffectiveLAN = ReclaimSession::ResolveLANMode(bIsLAN, OnlineSubsystem);

	if (CreateSessionCompleteHandle.IsValid() || DestroySessionCompleteHandle.IsValid())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Reclaim Session] Ignoring HostSession while another session operation is in flight."));
		return;
	}

	if (Sessions->GetNamedSession(ReclaimSession::Name))
	{
		PendingHostMaxPlayers = FMath::Clamp(MaxPlayers, 1, 4);
		bPendingHostIsLAN = bEffectiveLAN;
		bRetryHostAfterDestroy = true;

		UE_LOG(LogTemp, Log, TEXT("[Reclaim Session] Existing GameSession found; destroying it before creating a fresh host session."));
		ClearSessionDelegates();

		DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UReclaimSessionSubsystem::HandleDestroySessionComplete));

		if (!Sessions->DestroySession(ReclaimSession::Name))
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
			DestroySessionCompleteHandle.Reset();
			bRetryHostAfterDestroy = false;
			OnHostSessionComplete.Broadcast(false);
		}
		return;
	}

	StartHostSession(MaxPlayers, bEffectiveLAN);
}

void UReclaimSessionSubsystem::StartHostSession(int32 MaxPlayers, bool bIsLAN)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	if (Sessions->GetNamedSession(ReclaimSession::Name))
	{
		UE_LOG(LogTemp, Error, TEXT("[Reclaim Session] A GameSession still exists after cleanup; refusing to call CreateSession again."));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	ClearSessionDelegates();

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = FMath::Clamp(MaxPlayers, 1, 4);
	SessionSettings.bIsLANMatch = bIsLAN;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = IsJoinablePhase(EReclaimSessionPhase::Lobby);
	SessionSettings.bAllowInvites = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = false;
	SessionSettings.Set(ReclaimSession::PhaseKey, static_cast<int32>(EReclaimSessionPhase::Lobby), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(ReclaimSession::BuildKey, ReclaimSession::BuildSchema, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UE_LOG(LogTemp, Log, TEXT("[Reclaim Session] Creating GameSession: LAN=%s MaxPlayers=%d BuildId=%d"), bIsLAN ? TEXT("true") : TEXT("false"), SessionSettings.NumPublicConnections, SessionSettings.BuildUniqueId);

	CreateSessionCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UReclaimSessionSubsystem::HandleCreateSessionComplete));

	if (!Sessions->CreateSession(0, ReclaimSession::Name, SessionSettings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		CreateSessionCompleteHandle.Reset();
		OnHostSessionComplete.Broadcast(false);
	}
}

void UReclaimSessionSubsystem::FindSessions(int32 MaxResults, bool bIsLAN)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnFindSessionsComplete.Broadcast(false, TArray<FReclaimSessionResult>());
		return;
	}

	const IOnlineSubsystem* OnlineSubsystem = GetWorld() ? Online::GetSubsystem(GetWorld()) : IOnlineSubsystem::Get();
	const bool bEffectiveLAN = ReclaimSession::ResolveLANMode(bIsLAN, OnlineSubsystem);

	ClearSessionDelegates();

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = FMath::Max(1, MaxResults);
	SessionSearch->bIsLanQuery = bEffectiveLAN;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	UE_LOG(LogTemp, Log, TEXT("[Reclaim Session] Finding sessions: LAN=%s MaxResults=%d LocalBuildId=%d"), bEffectiveLAN ? TEXT("true") : TEXT("false"), SessionSearch->MaxSearchResults, GetBuildUniqueId());

	FindSessionsCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UReclaimSessionSubsystem::HandleFindSessionsComplete));

	if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		FindSessionsCompleteHandle.Reset();
		SessionSearch.Reset();
		OnFindSessionsComplete.Broadcast(false, TArray<FReclaimSessionResult>());
	}
}

void UReclaimSessionSubsystem::JoinSessionByIndex(int32 SearchResultIndex)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	ClearSessionDelegates();

	JoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UReclaimSessionSubsystem::HandleJoinSessionComplete));

	if (!Sessions->JoinSession(0, ReclaimSession::Name, SessionSearch->SearchResults[SearchResultIndex]))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		JoinSessionCompleteHandle.Reset();
		OnJoinSessionComplete.Broadcast(false);
	}
}

void UReclaimSessionSubsystem::LeaveSession()
{
	DestroySession();
}

void UReclaimSessionSubsystem::DestroySession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	ClearSessionDelegates();

	DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UReclaimSessionSubsystem::HandleDestroySessionComplete));

	if (!Sessions->DestroySession(ReclaimSession::Name))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		DestroySessionCompleteHandle.Reset();
		OnDestroySessionComplete.Broadcast(false);
	}
}

bool UReclaimSessionSubsystem::UpdateSessionPhase(EReclaimSessionPhase NewPhase)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return false;
	}

	FNamedOnlineSession* ExistingSession = Sessions->GetNamedSession(ReclaimSession::Name);
	if (!ExistingSession)
	{
		return false;
	}

	if (UpdateSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
	}

	FOnlineSessionSettings UpdatedSettings = ExistingSession->SessionSettings;
	const bool bJoinablePhase = IsJoinablePhase(NewPhase);
	UpdatedSettings.bShouldAdvertise = true;
	UpdatedSettings.bAllowJoinInProgress = bJoinablePhase;
	UpdatedSettings.bAllowJoinViaPresence = bJoinablePhase;
	UpdatedSettings.bUsesPresence = true;
	UpdatedSettings.Set(ReclaimSession::PhaseKey, static_cast<int32>(NewPhase), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(ReclaimSession::BuildKey, ReclaimSession::BuildSchema, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	UpdateSessionCompleteHandle = Sessions->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UReclaimSessionSubsystem::HandleUpdateSessionComplete));

	if (!Sessions->UpdateSession(ReclaimSession::Name, UpdatedSettings, true))
	{
		Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
		return false;
	}

	return true;
}

bool UReclaimSessionSubsystem::HasActiveSession() const
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	return Sessions.IsValid() && Sessions->GetNamedSession(ReclaimSession::Name) != nullptr;
}

const TArray<FOnlineSessionSearchResult>& UReclaimSessionSubsystem::GetCachedSearchResults() const
{
	return SessionSearch.IsValid() ? SessionSearch->SearchResults : EmptySearchResults;
}

void UReclaimSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}
	CreateSessionCompleteHandle.Reset();
	OnHostSessionComplete.Broadcast(bWasSuccessful);
}

void UReclaimSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}
	FindSessionsCompleteHandle.Reset();

	const TArray<FReclaimSessionResult> Results = BuildBlueprintResults();
	int32 JoinableCount = 0;
	for (const FReclaimSessionResult& Result : Results)
	{
		JoinableCount += Result.bJoinable ? 1 : 0;
	}
	UE_LOG(LogTemp, Log, TEXT("[Reclaim Session] Find complete: Success=%s RawResults=%d JoinableResults=%d"), bWasSuccessful ? TEXT("true") : TEXT("false"), Results.Num(), JoinableCount);
	OnFindSessionsComplete.Broadcast(bWasSuccessful, Results);
}

void UReclaimSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bSucceeded = Result == EOnJoinSessionCompleteResult::Success;

	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);

		FString ConnectString;
		if (bSucceeded && Sessions->GetResolvedConnectString(SessionName, ConnectString))
		{
			if (UGameInstance* OwningGameInstance = GetGameInstance())
			{
				if (APlayerController* PlayerController = OwningGameInstance->GetFirstLocalPlayerController())
				{
					PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
				}
			}
		}
	}

	JoinSessionCompleteHandle.Reset();
	OnJoinSessionComplete.Broadcast(bSucceeded);
}

void UReclaimSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	const bool bShouldRetryHost = bRetryHostAfterDestroy && SessionName == ReclaimSession::Name;
	const int32 RetryMaxPlayers = PendingHostMaxPlayers;
	const bool bRetryIsLAN = bPendingHostIsLAN;
	bRetryHostAfterDestroy = false;

	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}
	DestroySessionCompleteHandle.Reset();
	OnDestroySessionComplete.Broadcast(bWasSuccessful);

	if (bShouldRetryHost)
	{
		if (bWasSuccessful)
		{
			StartHostSession(RetryMaxPlayers, bRetryIsLAN);
		}
		else
		{
			OnHostSessionComplete.Broadcast(false);
		}
	}
}

void UReclaimSessionSubsystem::HandleUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
	}
	UpdateSessionCompleteHandle.Reset();
}

IOnlineSessionPtr UReclaimSessionSubsystem::GetSessionInterface() const
{
	if (UWorld* World = GetWorld())
	{
		return Online::GetSessionInterface(World);
	}

	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
}

bool UReclaimSessionSubsystem::IsJoinablePhase(EReclaimSessionPhase Phase)
{
	return Phase == EReclaimSessionPhase::Lobby || Phase == EReclaimSessionPhase::ReturningLobby;
}

void UReclaimSessionSubsystem::ClearSessionDelegates()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}

	if (CreateSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		CreateSessionCompleteHandle.Reset();
	}

	if (FindSessionsCompleteHandle.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		FindSessionsCompleteHandle.Reset();
	}

	if (JoinSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		JoinSessionCompleteHandle.Reset();
	}

	if (DestroySessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		DestroySessionCompleteHandle.Reset();
	}

	if (UpdateSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
	}
}

TArray<FReclaimSessionResult> UReclaimSessionSubsystem::BuildBlueprintResults() const
{
	TArray<FReclaimSessionResult> Results;
	if (!SessionSearch.IsValid())
	{
		return Results;
	}

	for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[Index];
		const FOnlineSession& Session = SearchResult.Session;

		int32 PhaseValue = static_cast<int32>(EReclaimSessionPhase::MainMenu);
		const bool bHasPhase = Session.SessionSettings.Get(ReclaimSession::PhaseKey, PhaseValue);
		if (!bHasPhase && Session.SessionSettings.bAllowJoinInProgress)
		{
			PhaseValue = static_cast<int32>(EReclaimSessionPhase::Lobby);
		}

		FReclaimSessionResult Result;
		Result.SearchResultIndex = Index;
		Result.HostDisplayName = Session.OwningUserName;
		Result.MaxPlayers = Session.SessionSettings.NumPublicConnections;
		Result.CurrentPlayers = Result.MaxPlayers - Session.NumOpenPublicConnections;
		Result.PingInMs = SearchResult.PingInMs;
		Result.Phase = static_cast<EReclaimSessionPhase>(PhaseValue);
		const bool bAdvertised = Session.SessionSettings.bShouldAdvertise || Session.SessionSettings.bIsLANMatch;
		Result.bJoinable = bAdvertised
			&& Session.SessionSettings.bAllowJoinInProgress
			&& IsJoinablePhase(Result.Phase)
			&& Session.NumOpenPublicConnections > 0;
		Results.Add(Result);
	}

	return Results;
}
