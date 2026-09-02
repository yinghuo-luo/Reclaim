// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimPlayerState.generated.h"

class UReclaimRunInventoryComponent;

UCLASS()
class RECLAIM_API AReclaimPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AReclaimPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;

	EReclaimRole GetSelectedRole() const { return SelectedRole; }
	bool IsReady() const { return bReady; }
	int32 GetPlayerSlot() const { return PlayerSlot; }
	EReclaimPlayerLifeState GetLifeState() const { return LifeState; }
	int32 GetRedeploysRemaining() const { return RedeploysRemaining; }
	const FReclaimLoadoutSummary& GetSubmittedLoadout() const { return SubmittedLoadout; }
	UReclaimRunInventoryComponent* GetRunInventoryComponent() const { return RunInventoryComponent; }

	void SetSelectedRole_Server(EReclaimRole NewRole);
	void SetReady_Server(bool bNewReady);
	void SetPlayerSlot_Server(int32 NewSlot);
	void SetLifeState_Server(EReclaimPlayerLifeState NewLifeState);
	void SetRedeploysRemaining_Server(int32 NewRedeploysRemaining);
	void SetSubmittedLoadout_Server(const FReclaimLoadoutSummary& NewLoadout);

protected:
	void CopyReclaimPropertiesTo(AReclaimPlayerState* TargetPlayerState) const;
	void ApplyLifeStateToCurrentPawn() const;

	UFUNCTION()
	void OnRep_SelectedRole();

	UFUNCTION()
	void OnRep_Ready();

	UFUNCTION()
	void OnRep_LifeState();

	UPROPERTY(ReplicatedUsing=OnRep_SelectedRole, BlueprintReadOnly, Category="Reclaim|Lobby")
	EReclaimRole SelectedRole = EReclaimRole::None;

	UPROPERTY(ReplicatedUsing=OnRep_Ready, BlueprintReadOnly, Category="Reclaim|Lobby")
	bool bReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Lobby")
	int32 PlayerSlot = INDEX_NONE;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Loadout")
	FReclaimLoadoutSummary SubmittedLoadout;

	UPROPERTY(ReplicatedUsing=OnRep_LifeState, BlueprintReadOnly, Category="Reclaim|Life")
	EReclaimPlayerLifeState LifeState = EReclaimPlayerLifeState::Active;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Life")
	int32 RedeploysRemaining = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|RunInventory")
	TObjectPtr<UReclaimRunInventoryComponent> RunInventoryComponent;
};
