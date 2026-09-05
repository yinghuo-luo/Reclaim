// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/ReclaimAnimConfig.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimAnimBrain.generated.h"

UENUM(BlueprintType)
enum class EReclaimAnimState : uint8
{
	Locomotion,
	JumpStart,
	Falling,
	Landing,
	Stop,
	TurnInPlace
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimAnimFacts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VerticalSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VelocityDirectionDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AccelerationDirectionDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimCardinalDirection VelocityCardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimCardinalDirection AccelerationCardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AccelerationMagnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AimYawDelta = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActorYawDelta = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveInputMagnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsFalling = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasMoveInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAccelerating = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCrouching = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimPlayerLifeState LifeState = EReclaimPlayerLifeState::Active;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimPlayerAnimBrain
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimAnimState State = EReclaimAnimState::Locomotion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimLocomotionPhase LocomotionPhase = EReclaimLocomotionPhase::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimCardinalDirection CardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimCardinalDirection PreviousCardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimCardinalDirection VelocityCardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimCardinalDirection AccelerationCardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimAnimTurnInPlaceState TurnInPlaceState = EReclaimAnimTurnInPlaceState::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float StateElapsedSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AirTimeSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float StationaryTurnYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DesiredLocomotionDirectionDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LastStableVelocityDirectionDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReclaimCardinalDirection LastStableVelocityCardinalDirection = EReclaimCardinalDirection::Forward;

	void Reset();
	void Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	void Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config, float DeltaSeconds);

	static float NormalizeLocomotionAngle(float DirectionDegrees);
	static float GetCardinalBaseAngle(EReclaimCardinalDirection Direction);
	static EReclaimCardinalDirection ClassifyCardinalDirection(float DirectionDegrees);
	static EReclaimCardinalDirection ClassifyCardinalDirectionWithHysteresis(
		float DirectionDegrees,
		EReclaimCardinalDirection CurrentDirection,
		float HysteresisDegrees);
	static float ComputeSignedDirectionDelta(float FromDegrees, float ToDegrees);

private:
	void SetState(EReclaimAnimState NewState);
	void SetGroundPhase(EReclaimLocomotionPhase NewPhase, EReclaimCardinalDirection NewDirection, float NewDesiredDirectionDegrees);
	void UpdateHistory(const FReclaimAnimFacts& Facts);
	void UpdateDirectionHistory(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	void EnterAirState(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	void UpdateAirState(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	void UpdateGroundState(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	void EnterCycleOrIdle(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	bool TryEnterStart(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	bool TryEnterPivot(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	bool TryInterruptPivot(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	bool TryEnterStop(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
	bool TryEnterTurnInPlace(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);

	static EReclaimAnimTurnInPlaceState ClassifyTurnInPlace(float YawDegrees, const UReclaimAnimConfig* Config);
	static bool ShouldEnterPivot(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);

	float PreviousGroundSpeed = 0.0f;
	float TurnCooldownRemaining = 0.0f;
	float LastDesiredDirectionDegrees = 0.0f;
	EReclaimCardinalDirection LastDesiredCardinalDirection = EReclaimCardinalDirection::Forward;
	bool bHasStableVelocityDirection = false;
	bool bHasDesiredDirection = false;
	bool bWasFalling = false;
	bool bHadMoveInput = false;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimEnemyAnimBrain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimAnimState State = EReclaimAnimState::Locomotion;

	void Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
};
