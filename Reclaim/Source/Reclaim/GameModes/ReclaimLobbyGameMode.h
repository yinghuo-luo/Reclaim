// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimLobbyGameMode.generated.h"

class AReclaimPlayerState;

UCLASS()
class RECLAIM_API AReclaimLobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AReclaimLobbyGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	EReclaimRoleRequestResult RequestRole(AReclaimPlayerState* PlayerState, EReclaimRole DesiredRole);
	bool SetPlayerReady(AReclaimPlayerState* PlayerState, bool bNewReady, FText& OutFailureReason);
	bool CanStartMission(FText& OutFailureReason) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Lobby")
	void StartMission();

	void ReleaseRoleFor(AReclaimPlayerState* PlayerState);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Reclaim|Travel")
	FSoftObjectPath MissionMap;

	int32 AllocatePlayerSlot() const;
	bool IsLobbyPlayer(const AReclaimPlayerState* PlayerState) const;
};
