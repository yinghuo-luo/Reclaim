// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimRoleReservationRules.generated.h"

UCLASS()
class RECLAIM_API UReclaimRoleReservationRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Reclaim|Lobby")
	static EReclaimRoleRequestResult EvaluateRoleRequest(EReclaimRole CurrentRole, EReclaimRole DesiredRole, bool bReady, bool bInLobby, bool bPlayerFound, bool bDesiredRoleOccupiedByOther);
};
