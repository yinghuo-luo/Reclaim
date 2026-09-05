// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/ReclaimAnimBrain.h"
#include "ReclaimAnimInstance.generated.h"

class ACharacter;
class AReclaimPlayerCharacter;
class UAnimSequenceBase;
class UCharacterMovementComponent;

UCLASS(Blueprintable, BlueprintType)
class RECLAIM_API UReclaimAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category="Reclaim|Animation")
	ACharacter* GetOwningCharacter() const { return OwningCharacter.Get(); }

	UFUNCTION(BlueprintPure, Category="Reclaim|Animation")
	AReclaimPlayerCharacter* GetReclaimPlayerCharacter() const { return ReclaimPlayerCharacter.Get(); }

	UFUNCTION(BlueprintPure, Category="Reclaim|Animation")
	UReclaimAnimConfig* GetAnimConfig() const { return AnimConfig.Get(); }

	UFUNCTION(BlueprintPure, Category="Reclaim|Animation")
	EReclaimAnimState GetPlayerAnimState() const { return PlayerAnimState; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation|Config")
	TObjectPtr<UReclaimAnimConfig> AnimConfig = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Owner")
	TObjectPtr<ACharacter> OwningCharacter = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Owner")
	TObjectPtr<AReclaimPlayerCharacter> ReclaimPlayerCharacter = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Owner")
	TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	FReclaimPlayerAnimBrain PlayerBrain;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	EReclaimAnimState PlayerAnimState = EReclaimAnimState::Locomotion;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	EReclaimLocomotionPhase LocomotionPhase = EReclaimLocomotionPhase::Idle;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	EReclaimCardinalDirection CardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	EReclaimCardinalDirection VelocityCardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	EReclaimCardinalDirection AccelerationCardinalDirection = EReclaimCardinalDirection::Forward;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	EReclaimAnimTurnInPlaceState TurnInPlaceState = EReclaimAnimTurnInPlaceState::None;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	float PlayerAnimStateElapsed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	float PlayerAirTime = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Brain")
	float StationaryTurnYaw = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float Speed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float GroundSpeed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float VerticalSpeed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float VelocityDirectionDegrees = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float AccelerationDirectionDegrees = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float AccelerationMagnitude = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float AimYawDelta = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float AimPitch = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float ActorYawDelta = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	float MoveInputMagnitude = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	bool bIsMoving = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	bool bHasMoveInput = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	bool bIsAccelerating = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	bool bIsInAir = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	bool bIsFalling = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	bool bIsCrouching = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Movement")
	bool bIsLocallyControlled = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Life")
	EReclaimPlayerLifeState LifeState = EReclaimPlayerLifeState::Active;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	TObjectPtr<UAnimSequenceBase> ActiveLocomotionSequence = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	float ActiveLocomotionPlayRate = 1.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	bool bActiveLocomotionLoops = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Warping")
	float OrientationWarpAngle = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Lean")
	float LeanAngle = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	TObjectPtr<UAnimSequenceBase> JumpStartSequence = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	TObjectPtr<UAnimSequenceBase> FallLoopSequence = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	TObjectPtr<UAnimSequenceBase> LandingSequence = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	TObjectPtr<UAnimSequenceBase> ActiveTurnSequence = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	float JumpStartPlayRate = 1.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	float FallLoopPlayRate = 1.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	float LandingPlayRate = 1.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Reclaim|Animation|Assets")
	float ActiveTurnPlayRate = 1.0f;

private:
	void CacheOwningCharacter();
	void ResetAnimationState();
	void ResetAnimationAssets();
	void ResolveAnimationAssets();
	void UpdatePresentationWarping(float DeltaSeconds);
	FReclaimAnimFacts BuildAnimFacts() const;

	static float CalculateDirectionDegrees(const FVector& InVector, const FRotator& BaseRotation);
	static float ResolveMoveInputMagnitude(const ACharacter& Character);
	static float GetClipPlayRate(const FReclaimAnimClip& Clip);
	static bool DoesLocomotionPhaseLoop(EReclaimLocomotionPhase Phase);

	float PreviousActorYaw = 0.0f;
	bool bHasPreviousActorYaw = false;
};
