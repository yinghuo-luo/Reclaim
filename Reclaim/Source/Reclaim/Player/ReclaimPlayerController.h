// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimPlayerController.generated.h"

class UUserWidget;

UCLASS()
class RECLAIM_API AReclaimPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Lobby")
	void RequestRole(EReclaimRole DesiredRole);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Lobby")
	void SetReady(bool bNewReady);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Lobby")
	void RequestStartMission();

	void EnterMissionInputMode();
	EReclaimRole GetSelectedRoleForTravel() const { return SelectedRoleForTravel; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Lobby")
	EReclaimRoleRequestResult GetLastRoleRequestResult() const { return LastRoleRequestResult; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Lobby")
	bool WasLastReadyRequestSuccessful() const { return bLastReadyRequestSucceeded; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Lobby")
	const FText& GetLastReadyFailureReason() const { return LastReadyFailureReason; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Lobby")
	bool WasLastStartMissionRequestSuccessful() const { return bLastStartMissionRequestSucceeded; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Lobby")
	const FText& GetLastStartMissionFailureReason() const { return LastStartMissionFailureReason; }

protected:
	UFUNCTION(Server, Reliable)
	void Server_RequestRole(EReclaimRole DesiredRole);

	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bNewReady);

	UFUNCTION(Server, Reliable)
	void Server_RequestStartMission();

	UFUNCTION(Client, Reliable)
	void Client_RoleRequestResult(EReclaimRoleRequestResult Result, EReclaimRole DesiredRole);

	UFUNCTION(Client, Reliable)
	void Client_ReadyRequestResult(bool bSucceeded, const FText& FailureReason);

	UFUNCTION(Client, Reliable)
	void Client_StartMissionRequestResult(bool bSucceeded, const FText& FailureReason);

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Lobby")
	EReclaimRoleRequestResult LastRoleRequestResult = EReclaimRoleRequestResult::Success;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Lobby")
	bool bLastReadyRequestSucceeded = true;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Lobby")
	FText LastReadyFailureReason;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Lobby")
	bool bLastStartMissionRequestSucceeded = true;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Lobby")
	FText LastStartMissionFailureReason;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Lobby")
	TSoftClassPtr<UUserWidget> LobbyWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Lobby")
	EReclaimRole SelectedRoleForTravel = EReclaimRole::None;

private:
	void EnsureLobbyWidget();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LobbyWidget;
};
