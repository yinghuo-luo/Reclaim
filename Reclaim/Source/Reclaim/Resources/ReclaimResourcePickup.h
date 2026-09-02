// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimResourcePickup.generated.h"

class USphereComponent;

UCLASS()
class RECLAIM_API AReclaimResourcePickup : public AActor
{
	GENERATED_BODY()

public:
	AReclaimResourcePickup();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Resources")
	void Collect();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Resources")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reclaim|Resources")
	FReclaimResourceBundle ResourceValue;
};
