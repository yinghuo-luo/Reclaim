// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/ReclaimSaveSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Save/ReclaimSaveGame.h"

UReclaimSaveGame* UReclaimSaveSubsystem::LoadOrCreateLocalSave()
{
	if (CurrentSave)
	{
		return CurrentSave;
	}

	if (USaveGame* LoadedSave = UGameplayStatics::LoadGameFromSlot(SlotName, 0))
	{
		CurrentSave = Cast<UReclaimSaveGame>(LoadedSave);
	}

	if (!CurrentSave)
	{
		CurrentSave = Cast<UReclaimSaveGame>(UGameplayStatics::CreateSaveGameObject(UReclaimSaveGame::StaticClass()));
	}

	return CurrentSave;
}

bool UReclaimSaveSubsystem::SaveLocal()
{
	return CurrentSave && UGameplayStatics::SaveGameToSlot(CurrentSave, SlotName, 0);
}
