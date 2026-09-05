// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/ReclaimAnimConfig.h"

#include "Animation/AnimSequenceBase.h"

namespace
{
	bool IsClipConfigured(const FReclaimAnimClip& Clip)
	{
		return Clip.Sequence != nullptr;
	}

	void ValidateClip(const TCHAR* ClipName, const FReclaimAnimClip& Clip, TArray<FString>& OutErrors)
	{
		if (!IsClipConfigured(Clip))
		{
			OutErrors.Add(FString::Printf(TEXT("%s is required but has no sequence configured."), ClipName));
		}
	}

	void ValidateDirectionalPhase(EReclaimLocomotionPhase Phase, const UReclaimAnimConfig& Config, TArray<FString>& OutErrors)
	{
		switch (Phase)
		{
		case EReclaimLocomotionPhase::Start:
			ValidateClip(TEXT("ForwardLocomotion.Start"), Config.ForwardLocomotion.Start, OutErrors);
			ValidateClip(TEXT("BackwardLocomotion.Start"), Config.BackwardLocomotion.Start, OutErrors);
			ValidateClip(TEXT("LeftLocomotion.Start"), Config.LeftLocomotion.Start, OutErrors);
			ValidateClip(TEXT("RightLocomotion.Start"), Config.RightLocomotion.Start, OutErrors);
			return;
		case EReclaimLocomotionPhase::Cycle:
			ValidateClip(TEXT("ForwardLocomotion.Cycle"), Config.ForwardLocomotion.Cycle, OutErrors);
			ValidateClip(TEXT("BackwardLocomotion.Cycle"), Config.BackwardLocomotion.Cycle, OutErrors);
			ValidateClip(TEXT("LeftLocomotion.Cycle"), Config.LeftLocomotion.Cycle, OutErrors);
			ValidateClip(TEXT("RightLocomotion.Cycle"), Config.RightLocomotion.Cycle, OutErrors);
			return;
		case EReclaimLocomotionPhase::Pivot:
			ValidateClip(TEXT("ForwardLocomotion.Pivot"), Config.ForwardLocomotion.Pivot, OutErrors);
			ValidateClip(TEXT("BackwardLocomotion.Pivot"), Config.BackwardLocomotion.Pivot, OutErrors);
			ValidateClip(TEXT("LeftLocomotion.Pivot"), Config.LeftLocomotion.Pivot, OutErrors);
			ValidateClip(TEXT("RightLocomotion.Pivot"), Config.RightLocomotion.Pivot, OutErrors);
			return;
		case EReclaimLocomotionPhase::Stop:
			ValidateClip(TEXT("ForwardLocomotion.Stop"), Config.ForwardLocomotion.Stop, OutErrors);
			ValidateClip(TEXT("BackwardLocomotion.Stop"), Config.BackwardLocomotion.Stop, OutErrors);
			ValidateClip(TEXT("LeftLocomotion.Stop"), Config.LeftLocomotion.Stop, OutErrors);
			ValidateClip(TEXT("RightLocomotion.Stop"), Config.RightLocomotion.Stop, OutErrors);
			return;
		case EReclaimLocomotionPhase::Idle:
		default:
			break;
		}
	}
}

float FReclaimAnimClip::GetDurationSeconds() const
{
	if (!Sequence)
	{
		return 0.0f;
	}

	return Sequence->GetPlayLength() / FMath::Max(PlayRate, 0.01f);
}

const FReclaimDirectionalLocomotionClips& UReclaimAnimConfig::GetDirectionalLocomotionClips(EReclaimCardinalDirection Direction) const
{
	switch (Direction)
	{
	case EReclaimCardinalDirection::Forward:
		return ForwardLocomotion;
	case EReclaimCardinalDirection::Backward:
		return BackwardLocomotion;
	case EReclaimCardinalDirection::Left:
		return LeftLocomotion;
	case EReclaimCardinalDirection::Right:
		return RightLocomotion;
	default:
		break;
	}

	static const FReclaimDirectionalLocomotionClips EmptyClips;
	return EmptyClips;
}

const FReclaimAnimClip& UReclaimAnimConfig::GetLocomotionClip(EReclaimLocomotionPhase Phase, EReclaimCardinalDirection Direction) const
{
	const FReclaimDirectionalLocomotionClips& DirectionalClips = GetDirectionalLocomotionClips(Direction);

	switch (Phase)
	{
	case EReclaimLocomotionPhase::Idle:
		return Idle;
	case EReclaimLocomotionPhase::Start:
		return DirectionalClips.Start;
	case EReclaimLocomotionPhase::Cycle:
		return DirectionalClips.Cycle;
	case EReclaimLocomotionPhase::Pivot:
		return DirectionalClips.Pivot;
	case EReclaimLocomotionPhase::Stop:
		return DirectionalClips.Stop;
	default:
		break;
	}

	static const FReclaimAnimClip EmptyClip;
	return EmptyClip;
}

const FReclaimAnimClip& UReclaimAnimConfig::GetTurnClip(EReclaimAnimTurnInPlaceState TurnState) const
{
	switch (TurnState)
	{
	case EReclaimAnimTurnInPlaceState::Left90:
		return TurnLeft90;
	case EReclaimAnimTurnInPlaceState::Right90:
		return TurnRight90;
	case EReclaimAnimTurnInPlaceState::Left180:
		return TurnLeft180;
	case EReclaimAnimTurnInPlaceState::Right180:
		return TurnRight180;
	case EReclaimAnimTurnInPlaceState::None:
	default:
		break;
	}

	static const FReclaimAnimClip EmptyClip;
	return EmptyClip;
}

bool UReclaimAnimConfig::ValidateConfig(TArray<FString>& OutErrors) const
{
	OutErrors.Reset();

	ValidateClip(TEXT("Idle"), Idle, OutErrors);
	ValidateDirectionalPhase(EReclaimLocomotionPhase::Cycle, *this, OutErrors);

	if (bEnableStartAnimations)
	{
		ValidateDirectionalPhase(EReclaimLocomotionPhase::Start, *this, OutErrors);
	}

	if (bEnablePivotAnimations)
	{
		ValidateDirectionalPhase(EReclaimLocomotionPhase::Pivot, *this, OutErrors);
	}

	if (bEnableStopAnimations)
	{
		ValidateDirectionalPhase(EReclaimLocomotionPhase::Stop, *this, OutErrors);
	}

	if (Turn180YawThreshold < TurnInPlaceYawThreshold)
	{
		OutErrors.Add(TEXT("Turn180YawThreshold must be greater than or equal to TurnInPlaceYawThreshold."));
	}

	if (PivotInterruptAngleThresholdDegrees < PivotAngleThresholdDegrees)
	{
		OutErrors.Add(TEXT("PivotInterruptAngleThresholdDegrees must be greater than or equal to PivotAngleThresholdDegrees."));
	}

	if (LeanMaxAngle > 0.0f && LeanYawRateForMaxAngle <= KINDA_SMALL_NUMBER)
	{
		OutErrors.Add(TEXT("LeanYawRateForMaxAngle must be greater than zero when lean is enabled."));
	}

	if (bEnableTurnInPlace)
	{
		ValidateClip(TEXT("TurnLeft90"), TurnLeft90, OutErrors);
		ValidateClip(TEXT("TurnRight90"), TurnRight90, OutErrors);
		ValidateClip(TEXT("TurnLeft180"), TurnLeft180, OutErrors);
		ValidateClip(TEXT("TurnRight180"), TurnRight180, OutErrors);
	}

	ValidateClip(TEXT("JumpStart"), JumpStart, OutErrors);
	ValidateClip(TEXT("FallLoop"), FallLoop, OutErrors);
	ValidateClip(TEXT("Landing"), Landing, OutErrors);

	return OutErrors.IsEmpty();
}
