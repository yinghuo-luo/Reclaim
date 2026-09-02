// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ReclaimTypes.generated.h"

class AReclaimPlayerState;
class AActor;

UENUM(BlueprintType)
enum class EReclaimMovementExecutionMode : uint8
{
	None,
	Dash,
	Leap
};

UENUM(BlueprintType)
enum class EReclaimDeployExecutionMode : uint8
{
	None,
	KineticBarrier,
	RepairDrone,
	StabilityField
};

UENUM(BlueprintType)
enum class EReclaimCombatExecutionMode : uint8
{
	None,
	Overclock,
	PurificationPulse
};

UENUM(BlueprintType)
enum class EReclaimReviveValidationResult : uint8
{
	Success,
	InvalidReviver,
	InvalidTarget,
	ReviverUnavailable,
	TargetNotDowned,
	SamePlayer,
	TooFar,
	NotFriendly
};

UENUM(BlueprintType)
enum class EReclaimRole : uint8
{
	None,
	Vanguard,
	Ranger,
	Engineer,
	Warden
};

UENUM(BlueprintType)
enum class EReclaimSessionPhase : uint8
{
	MainMenu,
	Lobby,
	Travelling,
	Mission,
	Result,
	ReturningLobby
};

UENUM(BlueprintType)
enum class EReclaimPlayerLifeState : uint8
{
	Active,
	Downed,
	Destroyed,
	Redeploying
};

UENUM(BlueprintType)
enum class EReclaimMissionPhase : uint8
{
	None,
	Landing,
	Node1,
	Node2,
	Node3,
	RootNest,
	ExtractionUnlocked,
	Extracting,
	Succeeded,
	Failed
};

UENUM(BlueprintType)
enum class EReclaimRoleRequestResult : uint8
{
	Success,
	InvalidRole,
	NotInLobby,
	PlayerNotFound,
	ReadyLocked,
	RoleOccupied,
	MissionLocked,
	InternalConflict
};

UENUM(BlueprintType)
enum class EReclaimResourceType : uint8
{
	Alloy,
	Crystal,
	Biopolymer,
	AnomalyCore
};

UENUM(BlueprintType)
enum class EReclaimRandomDomain : uint8
{
	MissionObjectiveOrder,
	ResourceSpawn,
	EncounterComposition,
	EliteAffix,
	RandomEvent,
	ExtractionPoint,
	RewardTerminal,
	WeaponSpread,
	Fabrication,
	AICombatDecision
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimResourceBundle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Alloy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Crystal = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Biopolymer = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AnomalyCore = 0;

	bool CanSpend(const FReclaimResourceBundle& Cost) const
	{
		return Alloy >= Cost.Alloy
			&& Crystal >= Cost.Crystal
			&& Biopolymer >= Cost.Biopolymer
			&& AnomalyCore >= Cost.AnomalyCore;
	}

	void AddClamped(const FReclaimResourceBundle& Delta)
	{
		Alloy = FMath::Max(0, Alloy + Delta.Alloy);
		Crystal = FMath::Max(0, Crystal + Delta.Crystal);
		Biopolymer = FMath::Max(0, Biopolymer + Delta.Biopolymer);
		AnomalyCore = FMath::Max(0, AnomalyCore + Delta.AnomalyCore);
	}

	bool SpendIfPossible(const FReclaimResourceBundle& Cost)
	{
		if (!CanSpend(Cost))
		{
			return false;
		}

		Alloy -= Cost.Alloy;
		Crystal -= Cost.Crystal;
		Biopolymer -= Cost.Biopolymer;
		AnomalyCore -= Cost.AnomalyCore;
		return true;
	}
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimLoadoutSummary
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BaseWeaponId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ExperimentalWeaponId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DeviceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FabricatedItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FabricationRollId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SchemaVersion = 1;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimRoleReservation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EReclaimRole Role = EReclaimRole::None;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AReclaimPlayerState> Owner = nullptr;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimRewardCandidate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CandidateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag BuildTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Weight = 1;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimAbilityActivationPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Ability")
	int32 AbilitySlot = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Ability")
	FVector_NetQuantize AimOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Ability")
	FVector_NetQuantizeNormal AimDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Ability")
	bool bHasTargetLocation = false;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Ability")
	FVector_NetQuantize TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category="Reclaim|Ability")
	TObjectPtr<AActor> TargetActor = nullptr;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimParticipantLifeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Life")
	bool bParticipating = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Life")
	EReclaimPlayerLifeState LifeState = EReclaimPlayerLifeState::Active;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reclaim|Life")
	int32 RedeploysRemaining = 0;
};
