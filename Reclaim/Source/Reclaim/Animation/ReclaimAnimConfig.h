// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReclaimAnimConfig.generated.h"

class UAnimSequenceBase;

UENUM(BlueprintType)
enum class EReclaimAnimTurnInPlaceState : uint8
{
	None,
	Left90,
	Right90,
	Left180,
	Right180
};

UENUM(BlueprintType)
enum class EReclaimCardinalDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EReclaimLocomotionPhase : uint8
{
	Idle,
	Start,
	Cycle,
	Pivot,
	Stop
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimAnimClip
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation")
	TObjectPtr<UAnimSequenceBase> Sequence = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation", meta=(ClampMin="0.01"))
	float PlayRate = 1.0f;

	float GetDurationSeconds() const;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimDirectionalLocomotionClips
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Directional Locomotion")
	FReclaimAnimClip Start;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Directional Locomotion")
	FReclaimAnimClip Cycle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Directional Locomotion")
	FReclaimAnimClip Pivot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Directional Locomotion")
	FReclaimAnimClip Stop;
};

UCLASS(BlueprintType)
class RECLAIM_API UReclaimAnimConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	FReclaimAnimClip Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	FReclaimDirectionalLocomotionClips ForwardLocomotion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	FReclaimDirectionalLocomotionClips BackwardLocomotion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	FReclaimDirectionalLocomotionClips LeftLocomotion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	FReclaimDirectionalLocomotionClips RightLocomotion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	bool bEnableStartAnimations = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	bool bEnablePivotAnimations = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion")
	bool bEnableStopAnimations = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float MovingSpeedThreshold = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float StopSpeedThreshold = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float MoveInputThreshold = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float AccelerationThreshold = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float StartMinGroundSpeed = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float PivotMinGroundSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float PivotMinAcceleration = 512.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="1", ClampMax="180"))
	float PivotAngleThresholdDegrees = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="1", ClampMax="180"))
	float PivotInterruptAngleThresholdDegrees = 135.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0"))
	float PivotMinInterruptElapsed = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Locomotion", meta=(ClampMin="0", ClampMax="45"))
	float CardinalDirectionHysteresisDegrees = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Stop", meta=(ClampMin="0"))
	float StopMinEntrySpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Stop", meta=(ClampMin="0"))
	float StableVelocitySampleMinSpeed = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Warping", meta=(ClampMin="0", ClampMax="180"))
	float OrientationWarpMaxAngle = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Lean", meta=(ClampMin="0"))
	float LeanMaxAngle = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Lean", meta=(ClampMin="0"))
	float LeanInterpSpeed = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Lean", meta=(ClampMin="1"))
	float LeanYawRateForMaxAngle = 360.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air")
	FReclaimAnimClip JumpStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air")
	FReclaimAnimClip FallLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air")
	FReclaimAnimClip Landing;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air", meta=(ClampMin="0"))
	float JumpStartMinVerticalSpeed = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air")
	float JumpStartToFallVerticalSpeed = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air", meta=(ClampMin="0"))
	float MinAirTimeForLanding = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air")
	bool bAllowLandingMovementInterrupt = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Air", meta=(ClampMin="0"))
	float LandingMovementInterruptDelay = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place")
	bool bEnableTurnInPlace = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place", meta=(ClampMin="1", ClampMax="179"))
	float TurnInPlaceYawThreshold = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place", meta=(ClampMin="1", ClampMax="179"))
	float Turn180YawThreshold = 135.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place", meta=(ClampMin="0"))
	float TurnMaxGroundSpeed = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place", meta=(ClampMin="0", ClampMax="1"))
	float TurnInputDeadZone = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place", meta=(ClampMin="0"))
	float TurnRetriggerDelay = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place")
	FReclaimAnimClip TurnLeft90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place")
	FReclaimAnimClip TurnRight90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place")
	FReclaimAnimClip TurnLeft180;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Turn In Place")
	FReclaimAnimClip TurnRight180;

	const FReclaimDirectionalLocomotionClips& GetDirectionalLocomotionClips(EReclaimCardinalDirection Direction) const;
	const FReclaimAnimClip& GetLocomotionClip(EReclaimLocomotionPhase Phase, EReclaimCardinalDirection Direction) const;
	const FReclaimAnimClip& GetTurnClip(EReclaimAnimTurnInPlaceState TurnState) const;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Animation")
	bool ValidateConfig(TArray<FString>& OutErrors) const;
};
