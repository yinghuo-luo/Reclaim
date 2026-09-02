// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReclaimExtractionZone.generated.h"

class UBoxComponent;

UCLASS()
class RECLAIM_API AReclaimExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	AReclaimExtractionZone();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Extraction")
	TObjectPtr<UBoxComponent> ExtractionBounds;
};
