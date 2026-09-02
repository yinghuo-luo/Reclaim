// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ReclaimPlayerState.h"

#include "Characters/ReclaimPlayerCharacter.h"
#include "Components/ReclaimRunInventoryComponent.h"
#include "Net/UnrealNetwork.h"

AReclaimPlayerState::AReclaimPlayerState()
{
	bReplicates = true;
	RunInventoryComponent = CreateDefaultSubobject<UReclaimRunInventoryComponent>(TEXT("RunInventoryComponent"));
}

void AReclaimPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimPlayerState, SelectedRole);
	DOREPLIFETIME(AReclaimPlayerState, bReady);
	DOREPLIFETIME(AReclaimPlayerState, PlayerSlot);
	DOREPLIFETIME(AReclaimPlayerState, SubmittedLoadout);
	DOREPLIFETIME(AReclaimPlayerState, LifeState);
	DOREPLIFETIME(AReclaimPlayerState, RedeploysRemaining);
}

void AReclaimPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	CopyReclaimPropertiesTo(Cast<AReclaimPlayerState>(PlayerState));
}

void AReclaimPlayerState::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);

	if (const AReclaimPlayerState* SourcePlayerState = Cast<AReclaimPlayerState>(PlayerState))
	{
		SelectedRole = SourcePlayerState->SelectedRole;
		PlayerSlot = SourcePlayerState->PlayerSlot;
		SubmittedLoadout = SourcePlayerState->SubmittedLoadout;
		LifeState = SourcePlayerState->LifeState;
		RedeploysRemaining = SourcePlayerState->RedeploysRemaining;
		bReady = false;
	}
}

void AReclaimPlayerState::CopyReclaimPropertiesTo(AReclaimPlayerState* TargetPlayerState) const
{
	if (!TargetPlayerState)
	{
		return;
	}

	TargetPlayerState->SelectedRole = SelectedRole;
	TargetPlayerState->PlayerSlot = PlayerSlot;
	TargetPlayerState->SubmittedLoadout = SubmittedLoadout;
	TargetPlayerState->LifeState = LifeState;
	TargetPlayerState->RedeploysRemaining = RedeploysRemaining;
	TargetPlayerState->bReady = false;
}

void AReclaimPlayerState::SetSelectedRole_Server(EReclaimRole NewRole)
{
	check(HasAuthority());
	SelectedRole = NewRole;
}

void AReclaimPlayerState::SetReady_Server(bool bNewReady)
{
	check(HasAuthority());
	bReady = bNewReady;
}

void AReclaimPlayerState::SetPlayerSlot_Server(int32 NewSlot)
{
	check(HasAuthority());
	PlayerSlot = NewSlot;
}

void AReclaimPlayerState::SetLifeState_Server(EReclaimPlayerLifeState NewLifeState)
{
	check(HasAuthority());
	LifeState = NewLifeState;
	ApplyLifeStateToCurrentPawn();
}

void AReclaimPlayerState::SetRedeploysRemaining_Server(int32 NewRedeploysRemaining)
{
	check(HasAuthority());
	RedeploysRemaining = FMath::Max(0, NewRedeploysRemaining);
}

void AReclaimPlayerState::SetSubmittedLoadout_Server(const FReclaimLoadoutSummary& NewLoadout)
{
	check(HasAuthority());
	SubmittedLoadout = NewLoadout;
}

void AReclaimPlayerState::OnRep_SelectedRole()
{
}

void AReclaimPlayerState::OnRep_Ready()
{
}

void AReclaimPlayerState::OnRep_LifeState()
{
	ApplyLifeStateToCurrentPawn();
}

void AReclaimPlayerState::ApplyLifeStateToCurrentPawn() const
{
	if (AReclaimPlayerCharacter* ReclaimCharacter = Cast<AReclaimPlayerCharacter>(GetPawn()))
	{
		ReclaimCharacter->ApplyReplicatedLifeState(LifeState);
	}
}
