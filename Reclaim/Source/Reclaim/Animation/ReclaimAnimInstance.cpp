// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/ReclaimAnimInstance.h"

#include "Characters/ReclaimPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/ReclaimPlayerState.h"

namespace
{
	float GetMovingSpeedThreshold(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->MovingSpeedThreshold, 0.0f) : 1.0f;
	}

	float GetMoveInputThreshold(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->MoveInputThreshold, 0.0f) : 0.1f;
	}

	float GetAccelerationThreshold(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->AccelerationThreshold, 0.0f) : 1.0f;
	}

	float GetOrientationWarpMaxAngle(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->OrientationWarpMaxAngle, 0.0f) : 45.0f;
	}

	float GetLeanMaxAngle(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->LeanMaxAngle, 0.0f) : 10.0f;
	}

	float GetLeanInterpSpeed(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->LeanInterpSpeed, 0.0f) : 8.0f;
	}

	float GetLeanYawRateForMaxAngle(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->LeanYawRateForMaxAngle, 1.0f) : 360.0f;
	}
}

void UReclaimAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheOwningCharacter();
	PlayerBrain.Reset();
	ResetAnimationState();
}

void UReclaimAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (OwningCharacter.Get() != Cast<ACharacter>(TryGetPawnOwner()))
	{
		CacheOwningCharacter();
		PlayerBrain.Reset();
	}

	if (!OwningCharacter)
	{
		PlayerBrain.Reset();
		ResetAnimationState();
		return;
	}

	CharacterMovementComponent = OwningCharacter->GetCharacterMovement();
	Velocity = OwningCharacter->GetVelocity();
	Acceleration = CharacterMovementComponent ? CharacterMovementComponent->GetCurrentAcceleration() : FVector::ZeroVector;
	Speed = Velocity.Size();
	GroundSpeed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;
	AccelerationMagnitude = Acceleration.Size2D();
	VelocityDirectionDegrees = CalculateDirectionDegrees(Velocity, OwningCharacter->GetActorRotation());
	AccelerationDirectionDegrees = CalculateDirectionDegrees(Acceleration, OwningCharacter->GetActorRotation());

	const FRotator ActorRotation = OwningCharacter->GetActorRotation();
	const FRotator BaseAimRotation = OwningCharacter->GetBaseAimRotation();
	AimYawDelta = FRotator::NormalizeAxis(BaseAimRotation.Yaw - ActorRotation.Yaw);
	AimPitch = FRotator::NormalizeAxis(BaseAimRotation.Pitch);

	const float CurrentActorYaw = ActorRotation.Yaw;
	ActorYawDelta = bHasPreviousActorYaw ? FRotator::NormalizeAxis(CurrentActorYaw - PreviousActorYaw) : 0.0f;
	PreviousActorYaw = CurrentActorYaw;
	bHasPreviousActorYaw = true;

	MoveInputMagnitude = ResolveMoveInputMagnitude(*OwningCharacter);
	bIsMoving = GroundSpeed > GetMovingSpeedThreshold(AnimConfig);
	bIsAccelerating = AccelerationMagnitude > GetAccelerationThreshold(AnimConfig);
	bHasMoveInput = MoveInputMagnitude > GetMoveInputThreshold(AnimConfig) || bIsAccelerating;
	bIsFalling = CharacterMovementComponent ? CharacterMovementComponent->IsFalling() : false;
	bIsInAir = bIsFalling;
	bIsCrouching = CharacterMovementComponent ? CharacterMovementComponent->IsCrouching() : false;
	bIsLocallyControlled = OwningCharacter->IsLocallyControlled();

	LifeState = EReclaimPlayerLifeState::Active;
	if (const AReclaimPlayerState* ReclaimPlayerState = ReclaimPlayerCharacter ? ReclaimPlayerCharacter->GetReclaimPlayerState() : nullptr)
	{
		LifeState = ReclaimPlayerState->GetLifeState();
	}

	PlayerBrain.Update(BuildAnimFacts(), AnimConfig, DeltaSeconds);
	PlayerAnimState = PlayerBrain.State;
	LocomotionPhase = PlayerBrain.LocomotionPhase;
	CardinalDirection = PlayerBrain.CardinalDirection;
	VelocityCardinalDirection = PlayerBrain.VelocityCardinalDirection;
	AccelerationCardinalDirection = PlayerBrain.AccelerationCardinalDirection;
	TurnInPlaceState = PlayerBrain.TurnInPlaceState;
	PlayerAnimStateElapsed = PlayerBrain.StateElapsedSeconds;
	PlayerAirTime = PlayerBrain.AirTimeSeconds;
	StationaryTurnYaw = PlayerBrain.StationaryTurnYaw;

	ResolveAnimationAssets();
	UpdatePresentationWarping(DeltaSeconds);
}

void UReclaimAnimInstance::CacheOwningCharacter()
{
	OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
	ReclaimPlayerCharacter = Cast<AReclaimPlayerCharacter>(OwningCharacter);
	CharacterMovementComponent = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
	bHasPreviousActorYaw = false;
	PreviousActorYaw = OwningCharacter ? OwningCharacter->GetActorRotation().Yaw : 0.0f;
}

void UReclaimAnimInstance::ResetAnimationState()
{
	CharacterMovementComponent = nullptr;
	Velocity = FVector::ZeroVector;
	Acceleration = FVector::ZeroVector;
	Speed = 0.0f;
	GroundSpeed = 0.0f;
	VerticalSpeed = 0.0f;
	VelocityDirectionDegrees = 0.0f;
	AccelerationDirectionDegrees = 0.0f;
	VelocityCardinalDirection = EReclaimCardinalDirection::Forward;
	AccelerationCardinalDirection = EReclaimCardinalDirection::Forward;
	AccelerationMagnitude = 0.0f;
	AimYawDelta = 0.0f;
	AimPitch = 0.0f;
	ActorYawDelta = 0.0f;
	MoveInputMagnitude = 0.0f;
	bIsMoving = false;
	bHasMoveInput = false;
	bIsAccelerating = false;
	bIsInAir = false;
	bIsFalling = false;
	bIsCrouching = false;
	bIsLocallyControlled = false;
	LifeState = EReclaimPlayerLifeState::Active;

	PlayerAnimState = EReclaimAnimState::Locomotion;
	LocomotionPhase = EReclaimLocomotionPhase::Idle;
	CardinalDirection = EReclaimCardinalDirection::Forward;
	TurnInPlaceState = EReclaimAnimTurnInPlaceState::None;
	PlayerAnimStateElapsed = 0.0f;
	PlayerAirTime = 0.0f;
	StationaryTurnYaw = 0.0f;
	OrientationWarpAngle = 0.0f;
	LeanAngle = 0.0f;

	ResetAnimationAssets();
}

void UReclaimAnimInstance::ResetAnimationAssets()
{
	ActiveLocomotionSequence = nullptr;
	JumpStartSequence = nullptr;
	FallLoopSequence = nullptr;
	LandingSequence = nullptr;
	ActiveTurnSequence = nullptr;

	ActiveLocomotionPlayRate = 1.0f;
	JumpStartPlayRate = 1.0f;
	FallLoopPlayRate = 1.0f;
	LandingPlayRate = 1.0f;
	ActiveTurnPlayRate = 1.0f;
	bActiveLocomotionLoops = true;
}

void UReclaimAnimInstance::ResolveAnimationAssets()
{
	if (!AnimConfig)
	{
		ResetAnimationAssets();
		return;
	}

	JumpStartSequence = AnimConfig->JumpStart.Sequence;
	FallLoopSequence = AnimConfig->FallLoop.Sequence;
	LandingSequence = AnimConfig->Landing.Sequence;
	JumpStartPlayRate = GetClipPlayRate(AnimConfig->JumpStart);
	FallLoopPlayRate = GetClipPlayRate(AnimConfig->FallLoop);
	LandingPlayRate = GetClipPlayRate(AnimConfig->Landing);

	const FReclaimAnimClip& LocomotionClip = AnimConfig->GetLocomotionClip(LocomotionPhase, CardinalDirection);
	ActiveLocomotionSequence = LocomotionClip.Sequence;
	ActiveLocomotionPlayRate = GetClipPlayRate(LocomotionClip);
	bActiveLocomotionLoops = DoesLocomotionPhaseLoop(LocomotionPhase);

	const FReclaimAnimClip& TurnClip = AnimConfig->GetTurnClip(TurnInPlaceState);
	ActiveTurnSequence = TurnClip.Sequence;
	ActiveTurnPlayRate = GetClipPlayRate(TurnClip);
}

void UReclaimAnimInstance::UpdatePresentationWarping(float DeltaSeconds)
{
	const bool bUsesGroundLocomotion = (PlayerAnimState == EReclaimAnimState::Locomotion || PlayerAnimState == EReclaimAnimState::Stop)
		&& LifeState == EReclaimPlayerLifeState::Active;

	if (bUsesGroundLocomotion)
	{
		const float BaseAngle = FReclaimPlayerAnimBrain::GetCardinalBaseAngle(CardinalDirection);
		const float DesiredDelta = FReclaimPlayerAnimBrain::ComputeSignedDirectionDelta(BaseAngle, PlayerBrain.DesiredLocomotionDirectionDegrees);
		const float MaxWarpAngle = GetOrientationWarpMaxAngle(AnimConfig);
		OrientationWarpAngle = FMath::Clamp(DesiredDelta, -MaxWarpAngle, MaxWarpAngle);
	}
	else
	{
		OrientationWarpAngle = 0.0f;
	}

	const float SafeDeltaSeconds = FMath::Max(DeltaSeconds, 0.0f);
	const bool bCanLean = LifeState == EReclaimPlayerLifeState::Active
		&& !bIsFalling
		&& bIsMoving
		&& PlayerAnimState != EReclaimAnimState::TurnInPlace;
	const float YawRateDegreesPerSecond = SafeDeltaSeconds > KINDA_SMALL_NUMBER ? ActorYawDelta / SafeDeltaSeconds : 0.0f;
	const float LeanScale = FMath::Clamp(YawRateDegreesPerSecond / GetLeanYawRateForMaxAngle(AnimConfig), -1.0f, 1.0f);
	const float TargetLeanAngle = bCanLean ? LeanScale * GetLeanMaxAngle(AnimConfig) : 0.0f;
	const float InterpSpeed = GetLeanInterpSpeed(AnimConfig);
	LeanAngle = InterpSpeed > 0.0f
		? FMath::FInterpTo(LeanAngle, TargetLeanAngle, SafeDeltaSeconds, InterpSpeed)
		: TargetLeanAngle;
}

FReclaimAnimFacts UReclaimAnimInstance::BuildAnimFacts() const
{
	FReclaimAnimFacts Facts;
	Facts.bValid = OwningCharacter != nullptr;
	Facts.Velocity = Velocity;
	Facts.Acceleration = Acceleration;
	Facts.Speed = Speed;
	Facts.GroundSpeed = GroundSpeed;
	Facts.VerticalSpeed = VerticalSpeed;
	Facts.VelocityDirectionDegrees = VelocityDirectionDegrees;
	Facts.AccelerationDirectionDegrees = AccelerationDirectionDegrees;
	Facts.VelocityCardinalDirection = FReclaimPlayerAnimBrain::ClassifyCardinalDirection(VelocityDirectionDegrees);
	Facts.AccelerationCardinalDirection = FReclaimPlayerAnimBrain::ClassifyCardinalDirection(AccelerationDirectionDegrees);
	Facts.AccelerationMagnitude = AccelerationMagnitude;
	Facts.AimYawDelta = AimYawDelta;
	Facts.ActorYawDelta = ActorYawDelta;
	Facts.MoveInputMagnitude = MoveInputMagnitude;
	Facts.bIsFalling = bIsFalling;
	Facts.bHasMoveInput = bHasMoveInput;
	Facts.bIsAccelerating = bIsAccelerating;
	Facts.bIsCrouching = bIsCrouching;
	Facts.LifeState = LifeState;
	return Facts;
}

float UReclaimAnimInstance::CalculateDirectionDegrees(const FVector& InVector, const FRotator& BaseRotation)
{
	const FVector PlanarVector(InVector.X, InVector.Y, 0.0f);
	if (PlanarVector.SizeSquared() <= 1.0f)
	{
		return 0.0f;
	}

	const FVector LocalVector = BaseRotation.UnrotateVector(PlanarVector);
	return FReclaimPlayerAnimBrain::NormalizeLocomotionAngle(
		FMath::RadiansToDegrees(FMath::Atan2(LocalVector.Y, LocalVector.X)));
}

float UReclaimAnimInstance::ResolveMoveInputMagnitude(const ACharacter& Character)
{
	const FVector PendingInput = Character.GetPendingMovementInputVector();
	const FVector LastInput = Character.GetLastMovementInputVector();
	return FMath::Max(PendingInput.Size2D(), LastInput.Size2D());
}

float UReclaimAnimInstance::GetClipPlayRate(const FReclaimAnimClip& Clip)
{
	return FMath::Max(Clip.PlayRate, 0.01f);
}

bool UReclaimAnimInstance::DoesLocomotionPhaseLoop(EReclaimLocomotionPhase Phase)
{
	return Phase == EReclaimLocomotionPhase::Idle || Phase == EReclaimLocomotionPhase::Cycle;
}
