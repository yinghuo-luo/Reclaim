// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/ReclaimFabricationService.h"

bool UReclaimFabricationService::CanSpend(const FReclaimResourceBundle& Resources, const FReclaimResourceBundle& Cost)
{
	return Resources.CanSpend(Cost);
}

FName UReclaimFabricationService::RollFabricationItem(int32 Seed, const TArray<FReclaimFabricationEntry>& Entries)
{
	int32 TotalWeight = 0;
	for (const FReclaimFabricationEntry& Entry : Entries)
	{
		if (!Entry.ItemId.IsNone())
		{
			TotalWeight += FMath::Max(0, Entry.Weight);
		}
	}

	if (TotalWeight <= 0)
	{
		return NAME_None;
	}

	FRandomStream Stream(Seed);
	int32 Roll = Stream.RandRange(1, TotalWeight);
	for (const FReclaimFabricationEntry& Entry : Entries)
	{
		if (Entry.ItemId.IsNone())
		{
			continue;
		}

		Roll -= FMath::Max(0, Entry.Weight);
		if (Roll <= 0)
		{
			return Entry.ItemId;
		}
	}

	return NAME_None;
}
