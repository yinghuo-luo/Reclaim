// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/ReclaimMissionGameMode.h"

#include "Characters/ReclaimPlayerCharacter.h"
#include "Characters/ReclaimRolePlayerCharacters.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameModes/ReclaimMissionGameState.h"
#include "Network/ReclaimSessionSubsystem.h"
#include "Player/ReclaimPlayerController.h"
#include "Player/ReclaimPlayerState.h"
#include "Roles/ReclaimRoleDefinition.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const TCHAR* RoleDefinitionPath(EReclaimRole Role)
	{
		switch (Role)
		{
		case EReclaimRole::Vanguard:
			return TEXT("/Game/Reclaim/Data/Roles/DA_Role_Vanguard.DA_Role_Vanguard");
		case EReclaimRole::Ranger:
			return TEXT("/Game/Reclaim/Data/Roles/DA_Role_Ranger.DA_Role_Ranger");
		case EReclaimRole::Engineer:
			return TEXT("/Game/Reclaim/Data/Roles/DA_Role_Engineer.DA_Role_Engineer");
		case EReclaimRole::Warden:
			return TEXT("/Game/Reclaim/Data/Roles/DA_Role_Warden.DA_Role_Warden");
		case EReclaimRole::None:
		default:
			return nullptr;
		}
	}

	const TCHAR* RolePawnClassPath(EReclaimRole Role)
	{
		switch (Role)
		{
		case EReclaimRole::Vanguard:
			return TEXT("/Game/Reclaim/Characters/Player/BP_PlayerRobot_Vanguard.BP_PlayerRobot_Vanguard_C");
		case EReclaimRole::Ranger:
			return TEXT("/Game/Reclaim/Characters/Player/BP_PlayerRobot_Ranger.BP_PlayerRobot_Ranger_C");
		case EReclaimRole::Engineer:
			return TEXT("/Game/Reclaim/Characters/Player/BP_PlayerRobot_Engineer.BP_PlayerRobot_Engineer_C");
		case EReclaimRole::Warden:
			return TEXT("/Game/Reclaim/Characters/Player/BP_PlayerRobot_Warden.BP_PlayerRobot_Warden_C");
		case EReclaimRole::None:
		default:
			return nullptr;
		}
	}
}

FReclaimRolePawnClass::FReclaimRolePawnClass(EReclaimRole InRole, TSubclassOf<AReclaimPlayerCharacter> InPawnClass)
	: Role(InRole)
	, PawnClass(InPawnClass)
{
}

FReclaimRoleDefinitionMapping::FReclaimRoleDefinitionMapping(EReclaimRole InRole, TSoftObjectPtr<UReclaimRoleDefinition> InRoleDefinition)
	: Role(InRole)
	, RoleDefinition(InRoleDefinition)
{
}

AReclaimMissionGameMode::AReclaimMissionGameMode()
{
	GameStateClass = AReclaimMissionGameState::StaticClass();
	PlayerStateClass = AReclaimPlayerState::StaticClass();
	PlayerControllerClass = AReclaimPlayerController::StaticClass();
	bUseSeamlessTravel = true;
	FallbackPlayerCharacterClass = AReclaimPlayerCharacter::StaticClass();

	RolePawnClasses =
	{
		FReclaimRolePawnClass(EReclaimRole::Vanguard, AReclaimVanguardPlayerCharacter::StaticClass()),
		FReclaimRolePawnClass(EReclaimRole::Ranger, AReclaimRangerPlayerCharacter::StaticClass()),
		FReclaimRolePawnClass(EReclaimRole::Engineer, AReclaimEngineerPlayerCharacter::StaticClass()),
		FReclaimRolePawnClass(EReclaimRole::Warden, AReclaimWardenPlayerCharacter::StaticClass())
	};

	RoleDefinitions =
	{
		FReclaimRoleDefinitionMapping(EReclaimRole::Vanguard, TSoftObjectPtr<UReclaimRoleDefinition>(FSoftObjectPath(TEXT("/Game/Reclaim/Data/Roles/DA_Role_Vanguard.DA_Role_Vanguard")))),
		FReclaimRoleDefinitionMapping(EReclaimRole::Ranger, TSoftObjectPtr<UReclaimRoleDefinition>(FSoftObjectPath(TEXT("/Game/Reclaim/Data/Roles/DA_Role_Ranger.DA_Role_Ranger")))),
		FReclaimRoleDefinitionMapping(EReclaimRole::Engineer, TSoftObjectPtr<UReclaimRoleDefinition>(FSoftObjectPath(TEXT("/Game/Reclaim/Data/Roles/DA_Role_Engineer.DA_Role_Engineer")))),
		FReclaimRoleDefinitionMapping(EReclaimRole::Warden, TSoftObjectPtr<UReclaimRoleDefinition>(FSoftObjectPath(TEXT("/Game/Reclaim/Data/Roles/DA_Role_Warden.DA_Role_Warden"))))
	};
}

void AReclaimMissionGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (AReclaimMissionGameState* MissionGameState = GetGameState<AReclaimMissionGameState>())
	{
		MissionGameState->SetMissionPhase_Server(EReclaimMissionPhase::Landing);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UReclaimSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UReclaimSessionSubsystem>())
		{
			SessionSubsystem->UpdateSessionPhase(EReclaimSessionPhase::Mission);
		}
	}
}

void AReclaimMissionGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AReclaimPlayerState* ReclaimPlayerState = NewPlayer ? NewPlayer->GetPlayerState<AReclaimPlayerState>() : nullptr)
	{
		ReclaimPlayerState->SetReady_Server(false);
	}
}

void AReclaimMissionGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	PreparePlayerForMission(NewPlayer);
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	EnsurePlayerPawnClassForMission_Server(NewPlayer);
	InitializePlayerPawnForMission_Server(NewPlayer);
}

void AReclaimMissionGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);
	PreparePlayerForMission(C);
	EnsurePlayerPawnClassForMission_Server(C);
	InitializePlayerPawnForMission_Server(C);
}

void AReclaimMissionGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	EnsurePlayerPawnClassForMission_Server(NewPlayer);
	InitializePlayerPawnForMission_Server(NewPlayer);
}

UClass* AReclaimMissionGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const EReclaimRole SelectedRole = ResolveRoleForController(InController);
	if (SelectedRole != EReclaimRole::None)
	{
		if (TSubclassOf<AReclaimPlayerCharacter> ResolvedClass = ResolvePawnClassForRole(SelectedRole))
		{
			return ResolvedClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

TSubclassOf<AReclaimPlayerCharacter> AReclaimMissionGameMode::ResolvePawnClassForRole(EReclaimRole InRole) const
{
	if (const UReclaimRoleDefinition* RoleDefinition = ResolveRoleDefinitionForRole(InRole))
	{
		if (RoleDefinition->PawnClass)
		{
			return RoleDefinition->PawnClass;
		}
	}

	if (TSubclassOf<AReclaimPlayerCharacter> BlueprintPawnClass = ResolveBlueprintPawnClassForRole(InRole))
	{
		return BlueprintPawnClass;
	}

	for (const FReclaimRolePawnClass& Entry : RolePawnClasses)
	{
		if (Entry.Role == InRole && Entry.PawnClass)
		{
			return Entry.PawnClass;
		}
	}

	if (TSubclassOf<AReclaimPlayerCharacter> NativeRoleClass = ResolveNativePawnClassForRole(InRole))
	{
		return NativeRoleClass;
	}

	if (FallbackPlayerCharacterClass)
	{
		return FallbackPlayerCharacterClass;
	}

	return AReclaimPlayerCharacter::StaticClass();
}

EReclaimRole AReclaimMissionGameMode::ResolveRoleForController(const AController* Controller) const
{
	const AReclaimPlayerState* ReclaimPlayerState = Controller ? Controller->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (ReclaimPlayerState && ReclaimPlayerState->GetSelectedRole() != EReclaimRole::None)
	{
		return ReclaimPlayerState->GetSelectedRole();
	}

	const AReclaimPlayerController* ReclaimPlayerController = Cast<AReclaimPlayerController>(Controller);
	return ReclaimPlayerController ? ReclaimPlayerController->GetSelectedRoleForTravel() : EReclaimRole::None;
}

bool AReclaimMissionGameMode::EnsurePlayerPawnClassForMission_Server(AController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return false;
	}

	AReclaimPlayerState* ReclaimPlayerState = Controller->GetPlayerState<AReclaimPlayerState>();
	const EReclaimRole SelectedRole = ResolveRoleForController(Controller);
	if (!ReclaimPlayerState || SelectedRole == EReclaimRole::None)
	{
		return false;
	}

	if (ReclaimPlayerState->GetSelectedRole() == EReclaimRole::None)
	{
		ReclaimPlayerState->SetSelectedRole_Server(SelectedRole);
	}

	const TSubclassOf<AReclaimPlayerCharacter> DesiredPawnClass = ResolvePawnClassForRole(SelectedRole);
	if (!DesiredPawnClass)
	{
		return false;
	}

	if (APawn* CurrentPawn = Controller->GetPawn())
	{
		if (CurrentPawn->IsA(DesiredPawnClass))
		{
			return true;
		}

		UE_LOG(LogGameMode, Warning, TEXT("Replacing mission pawn '%s' with role pawn '%s' for role %s."), *CurrentPawn->GetClass()->GetName(), *DesiredPawnClass->GetName(), *UEnum::GetValueAsString(SelectedRole));
		Controller->UnPossess();
		CurrentPawn->Destroy();
	}

	Super::RestartPlayer(Controller);

	const APawn* SpawnedPawn = Controller->GetPawn();
	const bool bCorrectClass = SpawnedPawn && SpawnedPawn->IsA(DesiredPawnClass);
	if (!bCorrectClass)
	{
		UE_LOG(LogGameMode, Error, TEXT("Mission pawn class could not be resolved for role %s; spawned '%s', expected '%s'."), *UEnum::GetValueAsString(SelectedRole), SpawnedPawn ? *SpawnedPawn->GetClass()->GetName() : TEXT("None"), *DesiredPawnClass->GetName());
	}

	return bCorrectClass;
}

UReclaimRoleDefinition* AReclaimMissionGameMode::ResolveRoleDefinitionForRole(EReclaimRole InRole) const
{
	for (const FReclaimRoleDefinitionMapping& Entry : RoleDefinitions)
	{
		if (Entry.Role == InRole && !Entry.RoleDefinition.IsNull())
		{
			return Entry.RoleDefinition.LoadSynchronous();
		}
	}

	if (const TCHAR* Path = RoleDefinitionPath(InRole))
	{
		return LoadObject<UReclaimRoleDefinition>(nullptr, Path);
	}

	return nullptr;
}

TSubclassOf<AReclaimPlayerCharacter> AReclaimMissionGameMode::ResolveBlueprintPawnClassForRole(EReclaimRole InRole) const
{
	if (const TCHAR* Path = RolePawnClassPath(InRole))
	{
		return LoadClass<AReclaimPlayerCharacter>(nullptr, Path);
	}

	return nullptr;
}

void AReclaimMissionGameMode::HandlePlayerDestroyed(AReclaimPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	PlayerState->SetLifeState_Server(EReclaimPlayerLifeState::Destroyed);

	if (PlayerState->GetRedeploysRemaining() > 0)
	{
		TryRedeployPlayer(PlayerState);
	}
}

bool AReclaimMissionGameMode::TryRedeployPlayer(AReclaimPlayerState* PlayerState)
{
	if (!PlayerState || PlayerState->GetRedeploysRemaining() <= 0 || PlayerState->GetLifeState() != EReclaimPlayerLifeState::Destroyed)
	{
		return false;
	}

	AController* Controller = FindControllerForPlayerState(PlayerState);
	if (!Controller)
	{
		return false;
	}

	const int32 PreviousRedeploysRemaining = PlayerState->GetRedeploysRemaining();
	PlayerState->SetRedeploysRemaining_Server(PreviousRedeploysRemaining - 1);
	PlayerState->SetLifeState_Server(EReclaimPlayerLifeState::Redeploying);
	CleanupOldPawnForRedeploy_Server(Controller);
	RestartPlayer(Controller);

	if (PlayerState->GetLifeState() == EReclaimPlayerLifeState::Active && Controller->GetPawn())
	{
		return true;
	}

	PlayerState->SetRedeploysRemaining_Server(PreviousRedeploysRemaining);
	PlayerState->SetLifeState_Server(EReclaimPlayerLifeState::Destroyed);
	return false;
}

void AReclaimMissionGameMode::PreparePlayerForMission(AController* Controller) const
{
	if (AReclaimPlayerState* ReclaimPlayerState = Controller ? Controller->GetPlayerState<AReclaimPlayerState>() : nullptr)
	{
		ReclaimPlayerState->SetReady_Server(false);
	}
}

bool AReclaimMissionGameMode::InitializePlayerPawnForMission_Server(AController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return false;
	}

	AReclaimPlayerCharacter* ReclaimCharacter = Cast<AReclaimPlayerCharacter>(Controller->GetPawn());
	AReclaimPlayerState* ReclaimPlayerState = Controller->GetPlayerState<AReclaimPlayerState>();
	if (!ReclaimCharacter || !ReclaimPlayerState)
	{
		return false;
	}

	const EReclaimRole SelectedRole = ResolveRoleForController(Controller);
	if (ReclaimPlayerState->GetSelectedRole() == EReclaimRole::None && SelectedRole != EReclaimRole::None)
	{
		ReclaimPlayerState->SetSelectedRole_Server(SelectedRole);
	}

	UReclaimRoleDefinition* RoleDefinition = ResolveRoleDefinitionForRole(ReclaimPlayerState->GetSelectedRole());
	ReclaimCharacter->InitializeMissionPawn_Server(RoleDefinition);
	ReclaimPlayerState->SetLifeState_Server(EReclaimPlayerLifeState::Active);
	return true;
}

AController* AReclaimMissionGameMode::FindControllerForPlayerState(AReclaimPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return nullptr;
	}

	if (AController* OwnerController = Cast<AController>(PlayerState->GetOwner()))
	{
		return OwnerController;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		AController* Controller = It->Get();
		if (Controller && Controller->PlayerState == PlayerState)
		{
			return Controller;
		}
	}

	return nullptr;
}

void AReclaimMissionGameMode::CleanupOldPawnForRedeploy_Server(AController* Controller) const
{
	if (!Controller)
	{
		return;
	}

	APawn* OldPawn = Controller->GetPawn();
	if (!OldPawn)
	{
		return;
	}

	if (AReclaimPlayerCharacter* OldReclaimCharacter = Cast<AReclaimPlayerCharacter>(OldPawn))
	{
		OldReclaimCharacter->PrepareForRedeployCleanup_Server();
	}
	else
	{
		OldPawn->SetActorEnableCollision(false);
	}

	Controller->UnPossess();
	OldPawn->Destroy();
}

TSubclassOf<AReclaimPlayerCharacter> AReclaimMissionGameMode::ResolveNativePawnClassForRole(EReclaimRole InRole) const
{
	switch (InRole)
	{
	case EReclaimRole::Vanguard:
		return AReclaimVanguardPlayerCharacter::StaticClass();
	case EReclaimRole::Ranger:
		return AReclaimRangerPlayerCharacter::StaticClass();
	case EReclaimRole::Engineer:
		return AReclaimEngineerPlayerCharacter::StaticClass();
	case EReclaimRole::Warden:
		return AReclaimWardenPlayerCharacter::StaticClass();
	case EReclaimRole::None:
	default:
		return nullptr;
	}
}
