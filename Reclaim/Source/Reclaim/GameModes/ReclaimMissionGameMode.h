// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Core/ReclaimTypes.h"
#include "ReclaimMissionGameMode.generated.h"

class AReclaimPlayerCharacter;
class AReclaimPlayerState;
class UReclaimRoleDefinition;

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimRolePawnClass
{
	GENERATED_BODY()

	FReclaimRolePawnClass() = default;
	FReclaimRolePawnClass(EReclaimRole InRole, TSubclassOf<AReclaimPlayerCharacter> InPawnClass);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission")
	EReclaimRole Role = EReclaimRole::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission")
	TSubclassOf<AReclaimPlayerCharacter> PawnClass;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimRoleDefinitionMapping
{
	GENERATED_BODY()

	FReclaimRoleDefinitionMapping() = default;
	FReclaimRoleDefinitionMapping(EReclaimRole InRole, TSoftObjectPtr<UReclaimRoleDefinition> InRoleDefinition);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission")
	EReclaimRole Role = EReclaimRole::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission")
	TSoftObjectPtr<UReclaimRoleDefinition> RoleDefinition;
};

UCLASS()
class RECLAIM_API AReclaimMissionGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AReclaimMissionGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UFUNCTION(BlueprintPure, Category="Reclaim|Mission")
	TSubclassOf<AReclaimPlayerCharacter> ResolvePawnClassForRole(EReclaimRole InRole) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Mission")
	UReclaimRoleDefinition* ResolveRoleDefinitionForRole(EReclaimRole InRole) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Mission")
	void HandlePlayerDestroyed(AReclaimPlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Mission")
	bool TryRedeployPlayer(AReclaimPlayerState* PlayerState);

protected:
	void PreparePlayerForMission(AController* Controller) const;
	EReclaimRole ResolveRoleForController(const AController* Controller) const;
	bool EnsurePlayerPawnClassForMission_Server(AController* Controller);
	bool InitializePlayerPawnForMission_Server(AController* Controller);
	AController* FindControllerForPlayerState(AReclaimPlayerState* PlayerState) const;
	void CleanupOldPawnForRedeploy_Server(AController* Controller) const;
	TSubclassOf<AReclaimPlayerCharacter> ResolveBlueprintPawnClassForRole(EReclaimRole InRole) const;
	TSubclassOf<AReclaimPlayerCharacter> ResolveNativePawnClassForRole(EReclaimRole InRole) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission|Spawn")
	TArray<FReclaimRolePawnClass> RolePawnClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission|Roles")
	TArray<FReclaimRoleDefinitionMapping> RoleDefinitions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Mission|Spawn")
	TSubclassOf<AReclaimPlayerCharacter> FallbackPlayerCharacterClass;
};
