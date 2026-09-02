// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/ReclaimAnimConfig.h"
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
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsFalling = false;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimPlayerAnimBrain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimAnimState State = EReclaimAnimState::Locomotion;

	void Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimEnemyAnimBrain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReclaimAnimState State = EReclaimAnimState::Locomotion;

	void Update(const FReclaimAnimFacts& Facts, const UReclaimAnimConfig* Config);
};
