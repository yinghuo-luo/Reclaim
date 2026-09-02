// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/ReclaimAnimBrain.h"
#include "ReclaimAnimInstance.generated.h"

UCLASS()
class RECLAIM_API UReclaimAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Animation")
	TObjectPtr<UReclaimAnimConfig> AnimConfig = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Animation")
	FReclaimPlayerAnimBrain PlayerBrain;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Animation")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Animation")
	bool bIsFalling = false;
};
