// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimDebugSubsystem.generated.h"

class APlayerController;

UCLASS()
class RECLAIM_API UReclaimDebugSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetRoleDebugString() const;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetMissionDebugString() const;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetCombatDebugString(APlayerController* PlayerController) const;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetThreatDebugString() const;
};
