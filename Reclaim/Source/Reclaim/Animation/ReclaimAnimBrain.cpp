// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/ReclaimAnimBrain.h"

void FReclaimPlayerAnimBrain::Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	const float StopThreshold = Config ? Config->StopSpeedThreshold : 10.0f;
	State = Facts.bIsFalling ? EReclaimAnimState::Falling : (Facts.Speed <= StopThreshold ? EReclaimAnimState::Stop : EReclaimAnimState::Locomotion);
}

void FReclaimEnemyAnimBrain::Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config)
{
	const float StopThreshold = Config ? Config->StopSpeedThreshold : 10.0f;
	State = Facts.bIsFalling ? EReclaimAnimState::Falling : (Facts.Speed <= StopThreshold ? EReclaimAnimState::Stop : EReclaimAnimState::Locomotion);
}
