// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "ReclaimRolePlayerCharacters.generated.h"

UCLASS(Blueprintable)
class RECLAIM_API AReclaimVanguardPlayerCharacter : public AReclaimPlayerCharacter
{
	GENERATED_BODY()

public:
	AReclaimVanguardPlayerCharacter();
};

UCLASS(Blueprintable)
class RECLAIM_API AReclaimRangerPlayerCharacter : public AReclaimPlayerCharacter
{
	GENERATED_BODY()

public:
	AReclaimRangerPlayerCharacter();
};

UCLASS(Blueprintable)
class RECLAIM_API AReclaimEngineerPlayerCharacter : public AReclaimPlayerCharacter
{
	GENERATED_BODY()

public:
	AReclaimEngineerPlayerCharacter();
};

UCLASS(Blueprintable)
class RECLAIM_API AReclaimWardenPlayerCharacter : public AReclaimPlayerCharacter
{
	GENERATED_BODY()

public:
	AReclaimWardenPlayerCharacter();
};
