// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ReclaimTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "ReclaimSessionSubsystem.generated.h"

class FOnlineSessionSearch;

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimSessionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	int32 SearchResultIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	FString HostDisplayName;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	int32 PingInMs = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	EReclaimSessionPhase Phase = EReclaimSessionPhase::MainMenu;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	bool bJoinable = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReclaimSessionOperationDelegate, bool, bSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReclaimFindSessionsDelegate, bool, bSucceeded, const TArray<FReclaimSessionResult>&, Results);

UCLASS()
class RECLAIM_API UReclaimSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Session")
	void HostSession(int32 MaxPlayers = 4, bool bIsLAN = true);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Session")
	void FindSessions(int32 MaxResults = 50, bool bIsLAN = true);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Session")
	void JoinSessionByIndex(int32 SearchResultIndex);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Session")
	void LeaveSession();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Session")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Session")
	bool UpdateSessionPhase(EReclaimSessionPhase NewPhase);

	UFUNCTION(BlueprintPure, Category="Reclaim|Session")
	bool HasActiveSession() const;

	const TArray<FOnlineSessionSearchResult>& GetCachedSearchResults() const;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Session")
	FReclaimSessionOperationDelegate OnHostSessionComplete;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Session")
	FReclaimFindSessionsDelegate OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Session")
	FReclaimSessionOperationDelegate OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Session")
	FReclaimSessionOperationDelegate OnDestroySessionComplete;

private:
	void StartHostSession(int32 MaxPlayers, bool bIsLAN);
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleUpdateSessionComplete(FName SessionName, bool bWasSuccessful);

	IOnlineSessionPtr GetSessionInterface() const;
	static bool IsJoinablePhase(EReclaimSessionPhase Phase);
	void ClearSessionDelegates();
	TArray<FReclaimSessionResult> BuildBlueprintResults() const;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	TArray<FOnlineSessionSearchResult> EmptySearchResults;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle UpdateSessionCompleteHandle;

	int32 PendingHostMaxPlayers = 4;
	bool bPendingHostIsLAN = true;
	bool bRetryHostAfterDestroy = false;
};
