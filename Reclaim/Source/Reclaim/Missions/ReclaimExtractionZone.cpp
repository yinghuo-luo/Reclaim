// Copyright Epic Games, Inc. All Rights Reserved.

#include "Missions/ReclaimExtractionZone.h"

#include "Components/BoxComponent.h"

AReclaimExtractionZone::AReclaimExtractionZone()
{
	bReplicates = true;
	ExtractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionBounds"));
	SetRootComponent(ExtractionBounds);
}
