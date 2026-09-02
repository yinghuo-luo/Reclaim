// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/ReclaimAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UReclaimAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UReclaimAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;

	GroundSpeed = Character ? Character->GetVelocity().Size2D() : 0.0f;
	bIsFalling = Movement ? Movement->IsFalling() : false;

	FReclaimAnimFacts Facts;
	Facts.Speed = GroundSpeed;
	Facts.bIsFalling = bIsFalling;
	PlayerBrain.Update(Facts, AnimConfig);
}
