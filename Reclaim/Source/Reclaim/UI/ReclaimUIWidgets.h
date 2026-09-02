// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReclaimTypes.h"
#include "Network/ReclaimSessionSubsystem.h"
#include "ReclaimUIWidgets.generated.h"

class AReclaimPlayerState;
class UButton;
class UProgressBar;
class UTextBlock;
class UUserWidget;

UCLASS()
class RECLAIM_API UReclaimMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleFindClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleHostSessionComplete(bool bSucceeded);

	UFUNCTION()
	void HandleFindSessionsComplete(bool bSucceeded, const TArray<FReclaimSessionResult>& Results);

	void SetStatusText(const FText& NewStatus) const;
	void ShowSessionBrowser();
	void TravelToLobbyAsHost();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Menu")
	FSoftObjectPath LobbyMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Menu", meta=(ClampMin="1", ClampMax="4"))
	int32 HostMaxPlayers = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Menu")
	bool bLANSession = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Menu")
	TSoftClassPtr<UUserWidget> SessionBrowserWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Menu")
	TObjectPtr<UButton> HostButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Menu")
	TObjectPtr<UButton> FindButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Menu")
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Menu")
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Menu")
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> SessionBrowserWidget;
};

UCLASS()
class RECLAIM_API UReclaimSessionBrowserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Session")
	void SetSelectedSearchResultIndex(int32 SearchResultIndex);

protected:
	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleJoinFirstClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleFindSessionsComplete(bool bSucceeded, const TArray<FReclaimSessionResult>& Results);

	UFUNCTION()
	void HandleJoinSessionComplete(bool bSucceeded);

	void RefreshSessionListText();
	int32 FindFirstJoinableSearchResultIndex() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Session", meta=(ClampMin="1"))
	int32 MaxSearchResults = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Session")
	bool bLANQuery = true;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Session")
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Session")
	TObjectPtr<UButton> JoinFirstButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Session")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Session")
	TObjectPtr<UTextBlock> SessionListText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Session")
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	TArray<FReclaimSessionResult> CachedResults;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Session")
	int32 SelectedSearchResultIndex = INDEX_NONE;
};

UCLASS()
class RECLAIM_API UReclaimLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void HandleVanguardClicked();

	UFUNCTION()
	void HandleRangerClicked();

	UFUNCTION()
	void HandleEngineerClicked();

	UFUNCTION()
	void HandleWardenClicked();

	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleStartMissionClicked();

	void RequestRole(EReclaimRole DesiredRole);
	void RefreshLobby();
	void RefreshRoleButtons(const AReclaimPlayerState* LocalPlayerState);
	bool IsRoleSelectable(EReclaimRole DesiredRole, const AReclaimPlayerState* LocalPlayerState) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Lobby", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.25f;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UButton> VanguardButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UButton> RangerButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UButton> EngineerButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UButton> WardenButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UButton> ReadyButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UButton> StartMissionButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> LobbyStatusText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> PlayerSlotsText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> RoleReservationsText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> LastRoleResultText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> ReadyButtonText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> StartMissionButtonText;

	FTimerHandle RefreshTimerHandle;
};

UCLASS()
class RECLAIM_API UReclaimRoleCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void SynchronizeProperties() override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Lobby")
	void SetRole(EReclaimRole NewRole);

protected:
	UFUNCTION()
	void HandleSelectClicked();

	void RefreshRoleCard();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Lobby")
	EReclaimRole Role = EReclaimRole::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Lobby", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.25f;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> RoleNameText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> RoleStatusText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> AbilitySummaryText;

	FTimerHandle RefreshTimerHandle;
};

UCLASS()
class RECLAIM_API UReclaimPlayerSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Lobby")
	void SetObservedPlayerState(AReclaimPlayerState* NewPlayerState);

protected:
	void RefreshPlayerSlot();
	AReclaimPlayerState* ResolveObservedPlayerState() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Lobby", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.25f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Lobby")
	TObjectPtr<AReclaimPlayerState> ObservedPlayerState;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> SlotText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> RoleText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> ReadyText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Lobby")
	TObjectPtr<UTextBlock> LifeStateText;

	FTimerHandle RefreshTimerHandle;
};

UCLASS()
class RECLAIM_API UReclaimHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	void RefreshHUD();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|HUD", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.20f;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UProgressBar> ShieldBar;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> ShieldText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> LifeStateText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> MissionPhaseText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> TeamResourcesText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> ActiveModifiersText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> RewardCandidatesText;

	FTimerHandle RefreshTimerHandle;
};

UCLASS()
class RECLAIM_API UReclaimTeammateRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|HUD")
	void SetObservedPlayerState(AReclaimPlayerState* NewPlayerState);

protected:
	void RefreshTeammateRow();
	AReclaimPlayerState* ResolveObservedPlayerState() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|HUD", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.25f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|HUD")
	TObjectPtr<AReclaimPlayerState> ObservedPlayerState;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> SlotText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> RoleText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> LifeStateText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|HUD")
	TObjectPtr<UTextBlock> ReadyText;

	FTimerHandle RefreshTimerHandle;
};

UCLASS()
class RECLAIM_API UReclaimRewardSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void HandleCandidateOneClicked();

	UFUNCTION()
	void HandleCandidateTwoClicked();

	UFUNCTION()
	void HandleCandidateThreeClicked();

	UFUNCTION(BlueprintImplementableEvent, Category="Reclaim|Rewards")
	void OnCandidateSelectionRequested(FName CandidateId);

	void RefreshRewardSelection();
	void RequestCandidateSelection(int32 CandidateIndex);
	bool TryGetCandidate(int32 CandidateIndex, FReclaimRewardCandidate& OutCandidate) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Rewards", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.25f;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Rewards")
	TObjectPtr<UButton> CandidateOneButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Rewards")
	TObjectPtr<UButton> CandidateTwoButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Rewards")
	TObjectPtr<UButton> CandidateThreeButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Rewards")
	TObjectPtr<UTextBlock> CandidateOneText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Rewards")
	TObjectPtr<UTextBlock> CandidateTwoText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Rewards")
	TObjectPtr<UTextBlock> CandidateThreeText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Rewards")
	TObjectPtr<UTextBlock> StatusText;

	FTimerHandle RefreshTimerHandle;
};

UCLASS()
class RECLAIM_API UReclaimNetDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	void RefreshDebugText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Debug", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.50f;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Debug")
	TObjectPtr<UTextBlock> RoleDebugText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Reclaim|Debug")
	TObjectPtr<UTextBlock> MissionDebugText;

	FTimerHandle RefreshTimerHandle;
};
