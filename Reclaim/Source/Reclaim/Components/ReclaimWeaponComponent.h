// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReclaimTypes.h"
#include "TimerManager.h"
#include "ReclaimWeaponComponent.generated.h"

class AActor;
class UReclaimWeaponDefinition;

UENUM(BlueprintType)
enum class EReclaimWeaponRuntimeState : uint8
{
	Unequipped,
	Idle,
	Reloading,
	Empty
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimShotHitSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	TObjectPtr<AActor> HitActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	FVector_NetQuantize ImpactPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	FVector_NetQuantizeNormal ImpactNormal = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 PelletHits = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	float DamageApplied = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	bool bTargetDefeated = false;
};

USTRUCT(BlueprintType)
struct RECLAIM_API FReclaimShotConfirmation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	FName WeaponId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 ShotSequence = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 ShotSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	FVector_NetQuantize AimOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	FVector_NetQuantizeNormal AimDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 PelletCount = 1;

	UPROPERTY(BlueprintReadOnly, Category="Reclaim|Weapon")
	TArray<FReclaimShotHitSummary> HitSummaries;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReclaimShotConfirmedSignature, const FReclaimShotConfirmation&, Shot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReclaimWeaponStateChangedSignature, EReclaimWeaponRuntimeState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReclaimAmmoChangedSignature, int32, MagazineAmmo, int32, ReserveAmmo);

UCLASS(ClassGroup=(Reclaim), meta=(BlueprintSpawnableComponent))
class RECLAIM_API UReclaimWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReclaimWeaponComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category="Reclaim|Weapon")
	void RequestFire(const FVector_NetQuantize& AimOrigin, const FVector_NetQuantizeNormal& AimDirection, int32 ShotSequence);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Weapon")
	void RequestFirePressed();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Weapon")
	void RequestFireReleased();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Weapon")
	void RequestReload();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Weapon")
	void RequestEquipWeaponByIndex(int32 WeaponIndex);

	UFUNCTION(BlueprintCallable, Category="Reclaim|Weapon")
	void RequestNextWeapon();

	UFUNCTION(Server, Unreliable)
	void Server_RequestFire(FVector_NetQuantize AimOrigin, FVector_NetQuantizeNormal AimDirection, int32 ShotSequence);

	UFUNCTION(Server, Reliable)
	void Server_RequestReload();

	UFUNCTION(Server, Reliable)
	void Server_RequestEquipWeaponByIndex(int32 WeaponIndex);

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	bool CanReload() const;

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	UReclaimWeaponDefinition* GetEquippedWeaponDefinition() const { return EquippedWeaponDefinition; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	EReclaimWeaponRuntimeState GetWeaponState() const { return WeaponState; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	int32 GetLastAcceptedShotSequence() const { return LastAcceptedShotSequence; }

	UFUNCTION(BlueprintPure, Category="Reclaim|Weapon")
	int32 GetLastAcceptedShotSeed() const { return LastAcceptedShotSeed; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Weapon")
	bool EquipWeaponDefinition_Server(UReclaimWeaponDefinition* NewWeaponDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Reclaim|Weapon")
	bool EnsureInitialWeaponEquipped_Server();

	UFUNCTION(BlueprintCallable, Category="Reclaim|Debug")
	FString GetCombatDebugString() const;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Weapon")
	FReclaimShotConfirmedSignature OnShotConfirmedDelegate;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Weapon")
	FReclaimWeaponStateChangedSignature OnWeaponStateChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category="Reclaim|Weapon")
	FReclaimAmmoChangedSignature OnAmmoChangedDelegate;

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnLocalFireRequested();

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnShotConfirmed(const FReclaimShotConfirmation& Shot);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnReloadStarted();

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnReloadCompleted();

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnWeaponEquipped(UReclaimWeaponDefinition* NewWeaponDefinition);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnWeaponStateChanged(EReclaimWeaponRuntimeState NewState);

	UFUNCTION(BlueprintNativeEvent, Category="Reclaim|Presentation")
	void OnAmmoChanged(int32 NewMagazineAmmo, int32 NewReserveAmmo);

protected:
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ShotConfirmed(const FReclaimShotConfirmation& Shot);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ReloadStarted();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ReloadCompleted();

	UFUNCTION()
	void OnRep_EquippedWeaponDefinition();

	UFUNCTION()
	void OnRep_WeaponState(EReclaimWeaponRuntimeState OldState);

	UFUNCTION()
	void OnRep_AmmoInMagazine(int32 OldAmmoInMagazine);

	UFUNCTION()
	void OnRep_ReserveAmmo(int32 OldReserveAmmo);

	bool CanAcceptFireRequest(const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence, FString* OutFailureReason = nullptr) const;
	bool CanAcceptReloadRequest() const;
	bool IsOwnerAllowedToFire() const;
	bool IsAimRequestReasonable(const FVector& AimOrigin, const FVector& AimDirection) const;
	void GetCurrentAim(FVector& OutAimOrigin, FVector& OutAimDirection) const;
	void RequestNextLocalShot();
	void ResolveAcceptedFire_Server(const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence);
	void ResolveHitscanFire_Server(const FVector& AimOrigin, const FVector& AimDirection, FReclaimShotConfirmation& InOutShot) const;
	bool EquipWeaponIndex_Server(int32 WeaponIndex);
	void EnsureWeaponAmmoState_Server(int32 WeaponIndex);
	void SaveEquippedAmmoState_Server();
	void CompleteReload_Server();
	void CancelReload_Server();
	void SetWeaponState_Server(EReclaimWeaponRuntimeState NewState);
	void BroadcastAmmoChanged();
	int32 GetOwnerPlayerSlot() const;
	int32 GetMissionRunSeed() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Loadout")
	TArray<TObjectPtr<UReclaimWeaponDefinition>> InitialWeaponDefinitions;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeaponDefinition, BlueprintReadOnly, Category="Reclaim|Weapon")
	TObjectPtr<UReclaimWeaponDefinition> EquippedWeaponDefinition = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponState, BlueprintReadOnly, Category="Reclaim|Weapon")
	EReclaimWeaponRuntimeState WeaponState = EReclaimWeaponRuntimeState::Unequipped;

	UPROPERTY(ReplicatedUsing=OnRep_AmmoInMagazine, BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 AmmoInMagazine = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ReserveAmmo, BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 ReserveAmmo = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 LastAcceptedShotSequence = INDEX_NONE;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Reclaim|Weapon")
	int32 LastAcceptedShotSeed = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Validation", meta=(ClampMin="0"))
	float MaxAcceptedAimOriginDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reclaim|Weapon|Validation", meta=(ClampMin="0", ClampMax="180"))
	float MaxAcceptedAimAngleDegrees = 60.0f;

	float LastAcceptedFireServerTime = -FLT_MAX;
	int32 EquippedWeaponIndex = INDEX_NONE;
	TArray<int32> MagazineAmmoByWeaponIndex;
	TArray<int32> ReserveAmmoByWeaponIndex;
	TArray<bool> bWeaponAmmoInitializedByIndex;
	int32 LocalShotSequence = INDEX_NONE;
	bool bLocalWantsToFire = false;
	FTimerHandle LocalFireTimerHandle;
	FTimerHandle ReloadTimerHandle;
};
