// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Character.h"
#include "Core/ReclaimTypes.h"
#include "Interfaces/ReclaimTargetContracts.h"
#include "ReclaimPlayerCharacter.generated.h"

class AReclaimPlayerState;
class AReclaimKineticBarrierActor;
class UReclaimAbilityDefinition;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USkeletalMeshComponent;
class UReclaimAbilitySystemComponent;
class UReclaimAttributeSet;
class UReclaimHealthShieldComponent;
class UReclaimInteractionComponent;
class UReclaimWeaponComponent;
class UReclaimRoleDefinition;
struct FInputActionValue;

UCLASS()
class RECLAIM_API AReclaimPlayerCharacter : public ACharacter, public IAbilitySystemInterface, public IReclaimRepairableInterface
{
	GENERATED_BODY()

public:
	AReclaimPlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual bool CanReceiveRepairFrom_Implementation(AActor* SourceActor) const override;
	virtual float ApplyRepair_Server_Implementation(AActor* SourceActor, float HealthAmount, float ShieldAmount) override;

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	USkeletalMeshComponent* GetThirdPersonMeshComponent() const { return GetMesh(); }

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	USkeletalMeshComponent* GetFirstPersonArmsMeshComponent() const { return FirstPersonArmsMeshA; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	AReclaimPlayerState* GetReclaimPlayerState() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	EReclaimRole GetSelectedRoleFromPlayerState() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	int32 GetPlayerSlotFromPlayerState() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	bool IsServerAuthority() const { return HasAuthority(); }

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	UReclaimHealthShieldComponent* GetHealthShieldComponent() const { return HealthShieldComponent; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	UReclaimWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Character")
	UReclaimInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

	UFUNCTION(BlueprintCallable, Category="Reclaim|Character")
	void GetCombatAim(FVector& OutAimOrigin, FVector& OutAimDirection) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Ability")
	void GrantRoleAbilities_Server(UReclaimRoleDefinition* RoleDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Ability")
	void ClearRoleAbilities_Server();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Character")
	void InitializeMissionPawn_Server(UReclaimRoleDefinition* RoleDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Character")
	void PrepareForRedeployCleanup_Server();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Ability")
	void RequestActivateRoleAbilityBySlot(int32 AbilitySlot);

	UFUNCTION(BlueprintPure, Category="Reclaim|Ability")
	UReclaimAbilityDefinition* GetGrantedAbilityDefinitionBySlot(int32 AbilitySlot) const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Ability")
	FGameplayTag GetGrantedAbilityTagBySlot(int32 AbilitySlot) const;

	bool ConsumePendingAbilityActivationPayload(FReclaimAbilityActivationPayload& OutPayload);
	void ApplyReplicatedLifeState(EReclaimPlayerLifeState NewLifeState);

	AReclaimKineticBarrierActor* GetActiveKineticBarrierActor() const { return ActiveKineticBarrierActor; }

	void RegisterKineticBarrier_Server(AReclaimKineticBarrierActor* BarrierActor);
	void UnregisterKineticBarrier_Server(AReclaimKineticBarrierActor* BarrierActor);
	bool TryAbsorbDamageWithKineticBarrier_Server(AActor* DamageInstigator, FGameplayTag DamageTypeTag, float IncomingDamage, float& OutRemainingDamage);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_RoleAbilityConfirmed(FGameplayTag AbilityTag, FVector_NetQuantize Location);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnRoleAbilityConfirmed(FGameplayTag AbilityTag, FVector Location);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnLifeStateChanged(EReclaimPlayerLifeState NewLifeState);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetM1NetworkDebugString() const;

protected:
	void AddDefaultInputMappingContext() const;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void StartJump();
	void StopJump();
	void FirePressed();
	void FireReleased();
	void ReloadWeapon();
	void SwitchWeapon();
	void AbilityOnePressed();
	void AbilityTwoPressed();
	void InteractPressed();
	void InteractReleased();
	void RefreshPerspectiveVisibility();
	void LogM1NetworkDebugState(const TCHAR* Context) const;
	FReclaimAbilityActivationPayload BuildAbilityActivationPayload(int32 AbilitySlot) const;
	AActor* FindInteractionTarget(float Range) const;
	bool IsLifeStateActive() const;
	bool CanAcceptMovementInput() const;
	void SetLifeStateGameplayTag(FGameplayTag Tag, bool bEnabled);
	void EndActiveKineticBarrier_Server();

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnPerspectiveModeChanged(bool bIsLocalOwner);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Camera")
	TObjectPtr<USkeletalMeshComponent> FirstPersonArmsMeshA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> SwitchWeaponAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> AbilityOneAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> AbilityTwoAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Input")
	int32 InputMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Life", meta=(ClampMin="0"))
	float DownedMaxWalkSpeed = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Ability", meta=(ClampMin="0"))
	float AbilityTargetTraceRange = 2500.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimHealthShieldComponent> HealthShieldComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimWeaponComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reclaim|Components")
	TObjectPtr<UReclaimInteractionComponent> InteractionComponent;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Ability")
	TArray<TObjectPtr<UReclaimAbilityDefinition>> GrantedRoleAbilityDefinitions;

	TArray<FGameplayAbilitySpecHandle> GrantedRoleAbilityHandles;
	FReclaimAbilityActivationPayload PendingAbilityActivationPayload;
	bool bHasPendingAbilityActivationPayload = false;
	float DefaultMaxWalkSpeed = 600.0f;

	UPROPERTY(Transient)
	TObjectPtr<AReclaimKineticBarrierActor> ActiveKineticBarrierActor = nullptr;

	UFUNCTION(Server, Reliable)
	void Server_RequestActivateRoleAbility(int32 AbilitySlot, FReclaimAbilityActivationPayload Payload);
};
