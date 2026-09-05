// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/ReclaimAnimBrain.h"

namespace
{
	float ResolveGroundSpeed(const FReclaimAnimFacts& Facts)
	{
		return Facts.GroundSpeed > KINDA_SMALL_NUMBER ? Facts.GroundSpeed : Facts.Speed;
	}

	float GetMovingSpeedThreshold(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->MovingSpeedThreshold, 0.0f) : 1.0f;
	}

	float GetStopSpeedThreshold(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->StopSpeedThreshold, 0.0f) : 10.0f;
	}

	float GetAccelerationThreshold(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->AccelerationThreshold, 0.0f) : 1.0f;
	}

	float GetStartMinGroundSpeed(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->StartMinGroundSpeed, 0.0f) : 120.0f;
	}

	float GetPivotMinGroundSpeed(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->PivotMinGroundSpeed, 0.0f) : 180.0f;
	}

	float GetPivotMinAcceleration(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->PivotMinAcceleration, 0.0f) : 512.0f;
	}

	float GetPivotAngleThreshold(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Clamp(Config->PivotAngleThresholdDegrees, 1.0f, 180.0f) : 120.0f;
	}

	float GetPivotInterruptAngleThreshold(const UReclaimAnimConfig* Config)
	{
		const float PivotThreshold = GetPivotAngleThreshold(Config);
		const float ConfiguredThreshold = Config ? FMath::Clamp(Config->PivotInterruptAngleThresholdDegrees, 1.0f, 180.0f) : 135.0f;
		return FMath::Max(PivotThreshold, ConfiguredThreshold);
	}

	float GetPivotMinInterruptElapsed(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->PivotMinInterruptElapsed, 0.0f) : 0.12f;
	}

	float GetCardinalDirectionHysteresis(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Clamp(Config->CardinalDirectionHysteresisDegrees, 0.0f, 45.0f) : 10.0f;
	}

	float GetStableVelocitySampleMinSpeed(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->StableVelocitySampleMinSpeed, 0.0f) : 20.0f;
	}

	float GetStopMinEntrySpeed(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Max(Config->StopMinEntrySpeed, 0.0f) : 180.0f;
	}

	float GetTurnStartAngle(const UReclaimAnimConfig* Config)
	{
		return Config ? FMath::Clamp(Config->TurnInPlaceYawThreshold, 1.0f, 179.0f) : 90.0f;
	}

	float GetTurn180Angle(const UReclaimAnimConfig* Config)
	{
		const float StartAngle = GetTurnStartAngle(Config);
		const float Configured180Angle = Config ? FMath::Clamp(Config->Turn180YawThreshold, 1.0f, 179.0f) : 135.0f;
		return FMath::Max(StartAngle, Configured180Angle);
	}

	float GetClipDuration(const FReclaimAnimClip& Clip)
	{
		return Clip.GetDurationSeconds();
	}

	float GetGroundPhaseDuration(const UReclaimAnimConfig* Config, EReclaimLocomotionPhase Phase, EReclaimCardinalDirection Direction)
	{
		return Config ? GetClipDuration(Config->GetLocomotionClip(Phase, Direction)) : 0.0f;
	}

	bool IsActiveLifeState(const FReclaimAnimFacts& Facts)
	{
		return Facts.LifeState == EReclaimPlayerLifeState::Active;
	}

	bool HasMovementIntent(const FReclaimAnimFacts& Facts)
	{
		return Facts.bHasMoveInput || Facts.bIsAccelerating;
	}
}

void FReclaimPlayerAnimBrain::Reset()
{
	State = EReclaimAnimState::Locomotion;
	LocomotionPhase = EReclaimLocomotionPhase::Idle;
	CardinalDirection = EReclaimCardinalDirection::Forward;
	PreviousCardinalDirection = EReclaimCardinalDirection::Forward;
	VelocityCardinalDirection = EReclaimCardinalDirection::Forward;
	AccelerationCardinalDirection = EReclaimCardinalDirection::Forward;
	TurnInPlaceState = EReclaimAnimTurnInPlaceState::None;
	StateElapsedSeconds = 0.0f;
	AirTimeSeconds = 0.0f;
	StationaryTurnYaw = 0.0f;
	DesiredLocomotionDirectionDegrees = 0.0f;
	LastStableVelocityDirectionDegrees = 0.0f;
	LastStableVelocityCardinalDirection = EReclaimCardinalDirection::Forward;
	LastDesiredDirectionDegrees = 0.0f;
	LastDesiredCardinalDirection = EReclaimCardinalDirection::Forward;
	PreviousGroundSpeed = 0.0f;
	TurnCooldownRemaining = 0.0f;
	bHasStableVelocityDirection = false;
	bHasDesiredDirection = false;
	bWasFalling = false;
	bHadMoveInput = false;
}

void FReclaimPlayerAnimBrain::Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	Update(Facts, Config, 0.0f);
}

void FReclaimPlayerAnimBrain::Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config, float DeltaSeconds)
{
	const float SafeDeltaSeconds = FMath::Max(DeltaSeconds, 0.0f);
	StateElapsedSeconds += SafeDeltaSeconds;
	TurnCooldownRemaining = FMath::Max(0.0f, TurnCooldownRemaining - SafeDeltaSeconds);

	if (!Facts.bValid)
	{
		Reset();
		return;
	}

	UpdateDirectionHistory(Facts, Config);

	const float GroundSpeed = ResolveGroundSpeed(Facts);
	const bool bCanAccumulateTurnYaw = Config
		&& Config->bEnableTurnInPlace
		&& IsActiveLifeState(Facts)
		&& !Facts.bIsFalling
		&& Facts.MoveInputMagnitude <= FMath::Max(Config->TurnInputDeadZone, 0.0f)
		&& !Facts.bIsAccelerating
		&& GroundSpeed <= FMath::Max(Config->TurnMaxGroundSpeed, 0.0f);

	if (bCanAccumulateTurnYaw)
	{
		StationaryTurnYaw = FMath::Clamp(StationaryTurnYaw + FRotator::NormalizeAxis(Facts.ActorYawDelta), -180.0f, 180.0f);
	}
	else if (State != EReclaimAnimState::TurnInPlace)
	{
		StationaryTurnYaw = 0.0f;
	}

	if (Facts.bIsFalling)
	{
		AirTimeSeconds = bWasFalling ? AirTimeSeconds + SafeDeltaSeconds : 0.0f;
		if (!bWasFalling)
		{
			EnterAirState(Facts, Config);
		}
		else
		{
			UpdateAirState(Facts, Config);
		}

		UpdateHistory(Facts);
		return;
	}

	if (bWasFalling)
	{
		if (Config && AirTimeSeconds >= FMath::Max(Config->MinAirTimeForLanding, 0.0f))
		{
			SetState(EReclaimAnimState::Landing);
		}
		else
		{
			SetState(EReclaimAnimState::Locomotion);
			EnterCycleOrIdle(Facts, Config);
		}
		AirTimeSeconds = 0.0f;
	}

	UpdateGroundState(Facts, Config);
	UpdateHistory(Facts);
}

float FReclaimPlayerAnimBrain::NormalizeLocomotionAngle(float DirectionDegrees)
{
	return FRotator::NormalizeAxis(DirectionDegrees);
}

float FReclaimPlayerAnimBrain::GetCardinalBaseAngle(EReclaimCardinalDirection Direction)
{
	switch (Direction)
	{
	case EReclaimCardinalDirection::Forward:
		return 0.0f;
	case EReclaimCardinalDirection::Right:
		return 90.0f;
	case EReclaimCardinalDirection::Backward:
		return 180.0f;
	case EReclaimCardinalDirection::Left:
		return -90.0f;
	default:
		break;
	}

	return 0.0f;
}

EReclaimCardinalDirection FReclaimPlayerAnimBrain::ClassifyCardinalDirection(float DirectionDegrees)
{
	const float Normalized = NormalizeLocomotionAngle(DirectionDegrees);
	const float AbsDirection = FMath::Abs(Normalized);

	if (AbsDirection <= 45.0f)
	{
		return EReclaimCardinalDirection::Forward;
	}
	if (AbsDirection >= 135.0f)
	{
		return EReclaimCardinalDirection::Backward;
	}

	return Normalized > 0.0f ? EReclaimCardinalDirection::Right : EReclaimCardinalDirection::Left;
}

EReclaimCardinalDirection FReclaimPlayerAnimBrain::ClassifyCardinalDirectionWithHysteresis(
	float DirectionDegrees,
	EReclaimCardinalDirection CurrentDirection,
	float HysteresisDegrees)
{
	const float Normalized = NormalizeLocomotionAngle(DirectionDegrees);
	const EReclaimCardinalDirection CandidateDirection = ClassifyCardinalDirection(Normalized);
	if (CandidateDirection == CurrentDirection)
	{
		return CurrentDirection;
	}

	const float Hysteresis = FMath::Clamp(HysteresisDegrees, 0.0f, 45.0f);
	const float CurrentDistance = FMath::Abs(ComputeSignedDirectionDelta(GetCardinalBaseAngle(CurrentDirection), Normalized));
	const float CandidateDistance = FMath::Abs(ComputeSignedDirectionDelta(GetCardinalBaseAngle(CandidateDirection), Normalized));

	return CandidateDistance + Hysteresis < CurrentDistance ? CandidateDirection : CurrentDirection;
}

float FReclaimPlayerAnimBrain::ComputeSignedDirectionDelta(float FromDegrees, float ToDegrees)
{
	return NormalizeLocomotionAngle(ToDegrees - FromDegrees);
}

void FReclaimPlayerAnimBrain::SetState(EReclaimAnimState NewState)
{
	if (State == NewState)
	{
		return;
	}

	State = NewState;
	StateElapsedSeconds = 0.0f;
	if (State != EReclaimAnimState::TurnInPlace)
	{
		TurnInPlaceState = EReclaimAnimTurnInPlaceState::None;
	}
}

void FReclaimPlayerAnimBrain::SetGroundPhase(
	EReclaimLocomotionPhase NewPhase,
	EReclaimCardinalDirection NewDirection,
	float NewDesiredDirectionDegrees)
{
	const EReclaimAnimState NewState = NewPhase == EReclaimLocomotionPhase::Stop
		? EReclaimAnimState::Stop
		: EReclaimAnimState::Locomotion;
	const bool bStateChanged = State != NewState;
	const bool bPhaseChanged = LocomotionPhase != NewPhase;
	const bool bDirectionChanged = CardinalDirection != NewDirection;

	if (bDirectionChanged)
	{
		PreviousCardinalDirection = CardinalDirection;
		CardinalDirection = NewDirection;
	}

	State = NewState;
	LocomotionPhase = NewPhase;
	DesiredLocomotionDirectionDegrees = NormalizeLocomotionAngle(NewDesiredDirectionDegrees);

	if (bStateChanged || bPhaseChanged || bDirectionChanged)
	{
		StateElapsedSeconds = 0.0f;
	}

	if (NewState != EReclaimAnimState::TurnInPlace)
	{
		TurnInPlaceState = EReclaimAnimTurnInPlaceState::None;
	}
}

void FReclaimPlayerAnimBrain::UpdateHistory(const FReclaimAnimFacts& Facts)
{
	PreviousGroundSpeed = ResolveGroundSpeed(Facts);
	bWasFalling = Facts.bIsFalling;
	bHadMoveInput = HasMovementIntent(Facts);
}

void FReclaimPlayerAnimBrain::UpdateDirectionHistory(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	const float HysteresisDegrees = GetCardinalDirectionHysteresis(Config);
	const float GroundSpeed = ResolveGroundSpeed(Facts);

	if (GroundSpeed >= FMath::Max(GetMovingSpeedThreshold(Config), KINDA_SMALL_NUMBER))
	{
		VelocityCardinalDirection = ClassifyCardinalDirectionWithHysteresis(
			Facts.VelocityDirectionDegrees,
			VelocityCardinalDirection,
			HysteresisDegrees);
	}

	if (Facts.AccelerationMagnitude >= GetAccelerationThreshold(Config))
	{
		AccelerationCardinalDirection = ClassifyCardinalDirectionWithHysteresis(
			Facts.AccelerationDirectionDegrees,
			AccelerationCardinalDirection,
			HysteresisDegrees);
		LastDesiredDirectionDegrees = NormalizeLocomotionAngle(Facts.AccelerationDirectionDegrees);
		LastDesiredCardinalDirection = AccelerationCardinalDirection;
		bHasDesiredDirection = true;
	}

	if (GroundSpeed >= GetStableVelocitySampleMinSpeed(Config))
	{
		LastStableVelocityDirectionDegrees = NormalizeLocomotionAngle(Facts.VelocityDirectionDegrees);
		LastStableVelocityCardinalDirection = ClassifyCardinalDirectionWithHysteresis(
			LastStableVelocityDirectionDegrees,
			LastStableVelocityCardinalDirection,
			HysteresisDegrees);
		bHasStableVelocityDirection = true;
	}
}

void FReclaimPlayerAnimBrain::EnterAirState(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	TurnInPlaceState = EReclaimAnimTurnInPlaceState::None;
	LocomotionPhase = EReclaimLocomotionPhase::Idle;
	StationaryTurnYaw = 0.0f;

	if (Config && Facts.VerticalSpeed >= FMath::Max(Config->JumpStartMinVerticalSpeed, 0.0f))
	{
		SetState(EReclaimAnimState::JumpStart);
	}
	else
	{
		SetState(EReclaimAnimState::Falling);
	}
}

void FReclaimPlayerAnimBrain::UpdateAirState(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	if (State == EReclaimAnimState::JumpStart)
	{
		const float JumpStartDuration = Config ? GetClipDuration(Config->JumpStart) : 0.0f;
		const bool bSequenceFinished = JumpStartDuration <= KINDA_SMALL_NUMBER || StateElapsedSeconds >= JumpStartDuration;
		const bool bReachedFallVelocity = Config && Facts.VerticalSpeed <= Config->JumpStartToFallVerticalSpeed;
		if (bSequenceFinished || bReachedFallVelocity)
		{
			SetState(EReclaimAnimState::Falling);
		}
		return;
	}

	if (State != EReclaimAnimState::Falling)
	{
		SetState(EReclaimAnimState::Falling);
	}
}

void FReclaimPlayerAnimBrain::UpdateGroundState(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	if (!IsActiveLifeState(Facts))
	{
		SetGroundPhase(EReclaimLocomotionPhase::Idle, CardinalDirection, DesiredLocomotionDirectionDegrees);
		StationaryTurnYaw = 0.0f;
		return;
	}

	if (State == EReclaimAnimState::Landing)
	{
		const float LandingDuration = Config ? GetClipDuration(Config->Landing) : 0.0f;
		const bool bFinished = LandingDuration <= KINDA_SMALL_NUMBER || StateElapsedSeconds >= LandingDuration;
		const bool bInterruptedByMovement = Config
			&& Config->bAllowLandingMovementInterrupt
			&& StateElapsedSeconds >= FMath::Max(Config->LandingMovementInterruptDelay, 0.0f)
			&& Facts.bHasMoveInput;

		if (!bFinished && !bInterruptedByMovement)
		{
			return;
		}

		SetState(EReclaimAnimState::Locomotion);
	}

	if (State == EReclaimAnimState::TurnInPlace)
	{
		const float TurnDuration = Config ? GetClipDuration(Config->GetTurnClip(TurnInPlaceState)) : 0.0f;
		const bool bFinished = TurnDuration <= KINDA_SMALL_NUMBER || StateElapsedSeconds >= TurnDuration;
		if (!bFinished && !HasMovementIntent(Facts))
		{
			return;
		}

		SetState(EReclaimAnimState::Locomotion);
	}

	if (TryEnterTurnInPlace(Facts, Config))
	{
		return;
	}

	if (State == EReclaimAnimState::Locomotion && LocomotionPhase == EReclaimLocomotionPhase::Pivot)
	{
		const float PivotDuration = GetGroundPhaseDuration(Config, EReclaimLocomotionPhase::Pivot, CardinalDirection);
		const bool bFinished = PivotDuration <= KINDA_SMALL_NUMBER || StateElapsedSeconds >= PivotDuration;
		if (!bFinished)
		{
			if (TryInterruptPivot(Facts, Config))
			{
				return;
			}
			if (Facts.bIsAccelerating)
			{
				DesiredLocomotionDirectionDegrees = NormalizeLocomotionAngle(Facts.AccelerationDirectionDegrees);
			}
			return;
		}

		EnterCycleOrIdle(Facts, Config);
		return;
	}

	if (State == EReclaimAnimState::Stop)
	{
		const float StopDuration = GetGroundPhaseDuration(Config, EReclaimLocomotionPhase::Stop, CardinalDirection);
		const bool bFinished = StopDuration <= KINDA_SMALL_NUMBER || StateElapsedSeconds >= StopDuration;
		if (!bFinished && !HasMovementIntent(Facts))
		{
			return;
		}

		SetState(EReclaimAnimState::Locomotion);
	}

	if (TryEnterPivot(Facts, Config))
	{
		return;
	}

	if (TryEnterStop(Facts, Config))
	{
		return;
	}

	if (State == EReclaimAnimState::Locomotion && LocomotionPhase == EReclaimLocomotionPhase::Start)
	{
		const float StartDuration = GetGroundPhaseDuration(Config, EReclaimLocomotionPhase::Start, CardinalDirection);
		const bool bFinished = StartDuration <= KINDA_SMALL_NUMBER || StateElapsedSeconds >= StartDuration;
		if (!bFinished)
		{
			if (Facts.bIsAccelerating)
			{
				DesiredLocomotionDirectionDegrees = NormalizeLocomotionAngle(Facts.AccelerationDirectionDegrees);
			}
			return;
		}

		EnterCycleOrIdle(Facts, Config);
		return;
	}

	if (TryEnterStart(Facts, Config))
	{
		return;
	}

	EnterCycleOrIdle(Facts, Config);
}

void FReclaimPlayerAnimBrain::EnterCycleOrIdle(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	const float GroundSpeed = ResolveGroundSpeed(Facts);
	const bool bMoving = GroundSpeed > GetMovingSpeedThreshold(Config);

	if (bMoving)
	{
		SetGroundPhase(EReclaimLocomotionPhase::Cycle, VelocityCardinalDirection, Facts.VelocityDirectionDegrees);
		return;
	}

	if (Facts.bIsAccelerating)
	{
		SetGroundPhase(EReclaimLocomotionPhase::Cycle, AccelerationCardinalDirection, Facts.AccelerationDirectionDegrees);
		return;
	}

	if (Facts.bHasMoveInput && bHasDesiredDirection)
	{
		SetGroundPhase(EReclaimLocomotionPhase::Cycle, LastDesiredCardinalDirection, LastDesiredDirectionDegrees);
		return;
	}

	const EReclaimCardinalDirection IdleDirection = bHasStableVelocityDirection ? LastStableVelocityCardinalDirection : CardinalDirection;
	const float IdleDesiredDirection = bHasStableVelocityDirection ? LastStableVelocityDirectionDegrees : DesiredLocomotionDirectionDegrees;
	SetGroundPhase(EReclaimLocomotionPhase::Idle, IdleDirection, IdleDesiredDirection);
}

bool FReclaimPlayerAnimBrain::TryEnterStart(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	if (!Config
		|| !Config->bEnableStartAnimations
		|| !IsActiveLifeState(Facts)
		|| Facts.bIsFalling
		|| State != EReclaimAnimState::Locomotion
		|| LocomotionPhase != EReclaimLocomotionPhase::Idle
		|| !Facts.bIsAccelerating
		|| !Facts.bHasMoveInput
		|| ResolveGroundSpeed(Facts) > GetStartMinGroundSpeed(Config))
	{
		return false;
	}

	SetGroundPhase(EReclaimLocomotionPhase::Start, AccelerationCardinalDirection, Facts.AccelerationDirectionDegrees);
	return true;
}

bool FReclaimPlayerAnimBrain::TryEnterPivot(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	if (State != EReclaimAnimState::Locomotion
		|| LocomotionPhase == EReclaimLocomotionPhase::Pivot
		|| !ShouldEnterPivot(Facts, Config))
	{
		return false;
	}

	SetGroundPhase(EReclaimLocomotionPhase::Pivot, AccelerationCardinalDirection, Facts.AccelerationDirectionDegrees);
	return true;
}

bool FReclaimPlayerAnimBrain::TryInterruptPivot(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	if (State != EReclaimAnimState::Locomotion
		|| LocomotionPhase != EReclaimLocomotionPhase::Pivot
		|| StateElapsedSeconds < GetPivotMinInterruptElapsed(Config)
		|| !ShouldEnterPivot(Facts, Config)
		|| AccelerationCardinalDirection == CardinalDirection)
	{
		return false;
	}

	const float DesiredDelta = FMath::Abs(ComputeSignedDirectionDelta(DesiredLocomotionDirectionDegrees, Facts.AccelerationDirectionDegrees));
	if (DesiredDelta < GetPivotInterruptAngleThreshold(Config))
	{
		return false;
	}

	SetGroundPhase(EReclaimLocomotionPhase::Pivot, AccelerationCardinalDirection, Facts.AccelerationDirectionDegrees);
	return true;
}

bool FReclaimPlayerAnimBrain::TryEnterStop(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	if (!Config
		|| !Config->bEnableStopAnimations
		|| !IsActiveLifeState(Facts)
		|| bWasFalling
		|| HasMovementIntent(Facts)
		|| PreviousGroundSpeed < GetStopMinEntrySpeed(Config)
		|| (!bHadMoveInput && !bHasStableVelocityDirection))
	{
		return false;
	}

	const EReclaimCardinalDirection CandidateDirection = bHasStableVelocityDirection
		? LastStableVelocityCardinalDirection
		: ClassifyCardinalDirection(Facts.VelocityDirectionDegrees);
	const float DesiredDirection = bHasStableVelocityDirection ? LastStableVelocityDirectionDegrees : Facts.VelocityDirectionDegrees;
	SetGroundPhase(EReclaimLocomotionPhase::Stop, CandidateDirection, DesiredDirection);
	return true;
}

bool FReclaimPlayerAnimBrain::TryEnterTurnInPlace(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	const float GroundSpeed = ResolveGroundSpeed(Facts);
	if (!Config
		|| !Config->bEnableTurnInPlace
		|| !IsActiveLifeState(Facts)
		|| Facts.bIsFalling
		|| Facts.MoveInputMagnitude > FMath::Max(Config->TurnInputDeadZone, 0.0f)
		|| Facts.bIsAccelerating
		|| GroundSpeed > FMath::Max(Config->TurnMaxGroundSpeed, 0.0f)
		|| TurnCooldownRemaining > 0.0f
		|| FMath::Abs(StationaryTurnYaw) < GetTurnStartAngle(Config))
	{
		return false;
	}

	TurnInPlaceState = ClassifyTurnInPlace(StationaryTurnYaw, Config);
	SetState(EReclaimAnimState::TurnInPlace);
	LocomotionPhase = EReclaimLocomotionPhase::Idle;
	StationaryTurnYaw = 0.0f;
	TurnCooldownRemaining = FMath::Max(Config->TurnRetriggerDelay, 0.0f);
	return true;
}

EReclaimAnimTurnInPlaceState FReclaimPlayerAnimBrain::ClassifyTurnInPlace(float YawDegrees, const UReclaimAnimConfig* Config)
{
	const float Normalized = NormalizeLocomotionAngle(YawDegrees);
	const bool bTurnRight = Normalized > 0.0f;
	const bool bUse180 = FMath::Abs(Normalized) >= GetTurn180Angle(Config);

	if (bUse180)
	{
		return bTurnRight ? EReclaimAnimTurnInPlaceState::Right180 : EReclaimAnimTurnInPlaceState::Left180;
	}

	return bTurnRight ? EReclaimAnimTurnInPlaceState::Right90 : EReclaimAnimTurnInPlaceState::Left90;
}

bool FReclaimPlayerAnimBrain::ShouldEnterPivot(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	if (!Config
		|| !Config->bEnablePivotAnimations
		|| !IsActiveLifeState(Facts)
		|| Facts.bIsFalling
		|| !Facts.bHasMoveInput
		|| !Facts.bIsAccelerating
		|| ResolveGroundSpeed(Facts) < GetPivotMinGroundSpeed(Config)
		|| Facts.AccelerationMagnitude < GetPivotMinAcceleration(Config))
	{
		return false;
	}

	const float DirectionDelta = FMath::Abs(ComputeSignedDirectionDelta(Facts.VelocityDirectionDegrees, Facts.AccelerationDirectionDegrees));
	return DirectionDelta >= GetPivotAngleThreshold(Config);
}

void FReclaimEnemyAnimBrain::Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	const float StopThreshold = GetStopSpeedThreshold(Config);
	const float GroundSpeed = ResolveGroundSpeed(Facts);
	State = Facts.bIsFalling ? EReclaimAnimState::Falling : (GroundSpeed <= StopThreshold ? EReclaimAnimState::Stop : EReclaimAnimState::Locomotion);
}
