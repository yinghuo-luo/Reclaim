// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimLobbyGameState.generated.h"

class AReclaimPlayerState;

UCLASS()
class RECLAIM_API AReclaimLobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
	AReclaimLobbyGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	EReclaimSessionPhase GetSessionPhase() const { return SessionPhase; }
	const TArray<FReclaimRoleReservation>& GetRoleReservations() const { return RoleReservations; }
	FName GetSelectedMissionId() const { return SelectedMissionId; }

	void SetSessionPhase_Server(EReclaimSessionPhase NewPhase);
	void SetSelectedMissionId_Server(FName NewMissionId);
	void ResetReservations_Server();
	bool IsRoleAvailable(EReclaimRole DesiredRole, const AReclaimPlayerState* RequestingPlayer) const;
	AReclaimPlayerState* GetReservationOwner(EReclaimRole DesiredRole) const;
	void SetRoleReservation_Server(EReclaimRole DesiredRole, AReclaimPlayerState* NewOwner);
	void ReleaseReservationFor_Server(AReclaimPlayerState* PlayerState);
	int32 GetReadyCount() const;
	bool ValidateRoleInvariants() const;

protected:
	UFUNCTION()
	void OnRep_SessionPhase();

	UFUNCTION()
	void OnRep_RoleReservations();

	UPROPERTY(ReplicatedUsing=OnRep_SessionPhase, BlueprintReadOnly, Category="Reclaim|Lobby")
	EReclaimSessionPhase SessionPhase = EReclaimSessionPhase::Lobby;

	UPROPERTY(ReplicatedUsing=OnRep_RoleReservations, BlueprintReadOnly, Category="Reclaim|Lobby")
	TArray<FReclaimRoleReservation> RoleReservations;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Lobby")
	FName SelectedMissionId = TEXT("ClearInfectedNest");
};
