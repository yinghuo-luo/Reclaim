// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/ReclaimDebugSubsystem.h"

#include "Components/ReclaimHealthShieldComponent.h"
#include "Components/ReclaimWeaponComponent.h"
#include "AI/ReclaimEnemyDirector.h"
#include "EngineUtils.h"
#include "GameModes/ReclaimLobbyGameState.h"
#include "GameModes/ReclaimMissionGameState.h"
#include "GameFramework/PlayerController.h"
#include "Player/ReclaimPlayerState.h"

FString UReclaimDebugSubsystem::GetRoleDebugString() const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	const UWorld* World = GetWorld();
	const AReclaimLobbyGameState* LobbyGameState = World ? World->GetGameState<AReclaimLobbyGameState>() : nullptr;
	return LobbyGameState ? FString::Printf(TEXT("Ready=%d Reservations=%d"), LobbyGameState->GetReadyCount(), LobbyGameState->GetRoleReservations().Num()) : TEXT("No LobbyGameState");
#endif
}

FString UReclaimDebugSubsystem::GetMissionDebugString() const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	const UWorld* World = GetWorld();
	const AReclaimMissionGameState* MissionGameState = World ? World->GetGameState<AReclaimMissionGameState>() : nullptr;
	return MissionGameState ? FString::Printf(TEXT("RunSeed=%d Phase=%d"), MissionGameState->GetRunSeed(), static_cast<int32>(MissionGameState->GetMissionPhase())) : TEXT("No MissionGameState");
#endif
}

FString UReclaimDebugSubsystem::GetCombatDebugString(APlayerController* PlayerController) const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const AReclaimPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AReclaimPlayerState>() : nullptr;
	const UReclaimHealthShieldComponent* HealthShield = Pawn ? Pawn->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
	const UReclaimWeaponComponent* Weapon = Pawn ? Pawn->FindComponentByClass<UReclaimWeaponComponent>() : nullptr;

	const float Health = HealthShield ? HealthShield->GetHealth() : 0.0f;
	const float Shield = HealthShield ? HealthShield->GetShield() : 0.0f;
	const EReclaimPlayerLifeState LifeState = PlayerState ? PlayerState->GetLifeState() : EReclaimPlayerLifeState::Active;
	return FString::Printf(
		TEXT("Player Health=%.0f Shield=%.0f LifeState=%d | Weapon %s"),
		Health,
		Shield,
		static_cast<int32>(LifeState),
		Weapon ? *Weapon->GetCombatDebugString() : TEXT("No WeaponComponent"));
#endif
}

FString UReclaimDebugSubsystem::GetThreatDebugString() const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	const UWorld* World = GetWorld();
	if (!World)
	{
		return TEXT("No World");
	}

	for (TActorIterator<AReclaimEnemyDirector> It(World); It; ++It)
	{
		return It->GetThreatDebugString();
	}

	return TEXT("No EnemyDirector");
#endif
}
