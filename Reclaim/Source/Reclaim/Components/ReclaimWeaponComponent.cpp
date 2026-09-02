// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/ReclaimWeaponComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/ReclaimKineticBarrierActor.h"
#include "AbilitySystemComponent.h"
#include "Characters/ReclaimPlayerCharacter.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Core/ReclaimCombatMath.h"
#include "Core/ReclaimGameplayTags.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameModes/ReclaimMissionGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"
#include "Weapons/ReclaimProjectile.h"
#include "Weapons/ReclaimWeaponDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogReclaimWeaponComponent, Log, All);

namespace
{
	bool IsFireEnabledMissionPhase(EReclaimMissionPhase Phase)
	{
		switch (Phase)
		{
		case EReclaimMissionPhase::Landing:
		case EReclaimMissionPhase::Node1:
		case EReclaimMissionPhase::Node2:
		case EReclaimMissionPhase::Node3:
		case EReclaimMissionPhase::RootNest:
		case EReclaimMissionPhase::ExtractionUnlocked:
		case EReclaimMissionPhase::Extracting:
			return true;
		default:
			return false;
		}
	}

	struct FReclaimPendingWeaponHit
	{
		AActor* HitActor = nullptr;
		FVector ImpactPoint = FVector::ZeroVector;
		FVector ImpactNormal = FVector::UpVector;
		int32 PelletHits = 0;
		float PendingDamage = 0.0f;
	};

	FString WeaponStateToString(EReclaimWeaponRuntimeState State)
	{
		if (const UEnum* StateEnum = StaticEnum<EReclaimWeaponRuntimeState>())
		{
			return StateEnum->GetNameStringByValue(static_cast<int64>(State));
		}

		return FString::FromInt(static_cast<int32>(State));
	}
}

UReclaimWeaponComponent::UReclaimWeaponComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UReclaimWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority() && !EquippedWeaponDefinition && InitialWeaponDefinitions.Num() > 0)
	{
		EquipWeaponDefinition_Server(InitialWeaponDefinitions[0]);
	}
}

void UReclaimWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocalFireTimerHandle);
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UReclaimWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UReclaimWeaponComponent, EquippedWeaponDefinition);
	DOREPLIFETIME(UReclaimWeaponComponent, WeaponState);
	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimWeaponComponent, AmmoInMagazine, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UReclaimWeaponComponent, ReserveAmmo, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME(UReclaimWeaponComponent, LastAcceptedShotSequence);
	DOREPLIFETIME(UReclaimWeaponComponent, LastAcceptedShotSeed);
}

void UReclaimWeaponComponent::RequestFire(const FVector_NetQuantize& AimOrigin, const FVector_NetQuantizeNormal& AimDirection, int32 ShotSequence)
{
	OnLocalFireRequested();
	Server_RequestFire(AimOrigin, AimDirection, ShotSequence);
}

void UReclaimWeaponComponent::RequestFirePressed()
{
	bLocalWantsToFire = true;
	RequestNextLocalShot();

	UWorld* World = GetWorld();
	if (!World || !EquippedWeaponDefinition || World->GetTimerManager().IsTimerActive(LocalFireTimerHandle))
	{
		return;
	}

	const float RepeatInterval = FMath::Max(0.01f, EquippedWeaponDefinition->FireInterval);
	World->GetTimerManager().SetTimer(LocalFireTimerHandle, this, &UReclaimWeaponComponent::RequestNextLocalShot, RepeatInterval, true);
}

void UReclaimWeaponComponent::RequestFireReleased()
{
	bLocalWantsToFire = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocalFireTimerHandle);
	}
}

void UReclaimWeaponComponent::RequestReload()
{
	RequestFireReleased();
	Server_RequestReload();
}

void UReclaimWeaponComponent::RequestEquipWeaponByIndex(int32 WeaponIndex)
{
	Server_RequestEquipWeaponByIndex(WeaponIndex);
}

void UReclaimWeaponComponent::RequestNextWeapon()
{
	if (InitialWeaponDefinitions.Num() <= 0)
	{
		return;
	}

	const int32 CurrentIndex = InitialWeaponDefinitions.IndexOfByKey(EquippedWeaponDefinition);
	const int32 NextIndex = CurrentIndex == INDEX_NONE ? 0 : (CurrentIndex + 1) % InitialWeaponDefinitions.Num();
	RequestEquipWeaponByIndex(NextIndex);
}

void UReclaimWeaponComponent::Server_RequestFire_Implementation(FVector_NetQuantize AimOrigin, FVector_NetQuantizeNormal AimDirection, int32 ShotSequence)
{
	FString FailureReason;
	if (!CanAcceptFireRequest(AimOrigin, AimDirection, ShotSequence, &FailureReason))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogReclaimWeaponComponent, VeryVerbose, TEXT("Fire rejected for %s: %s"), *GetNameSafe(GetOwner()), *FailureReason);
#endif
		return;
	}

	ResolveAcceptedFire_Server(AimOrigin, AimDirection, ShotSequence);
}

void UReclaimWeaponComponent::Server_RequestReload_Implementation()
{
	if (!CanAcceptReloadRequest())
	{
		return;
	}

	SetWeaponState_Server(EReclaimWeaponRuntimeState::Reloading);
	Multicast_ReloadStarted();

	const float ReloadDuration = EquippedWeaponDefinition ? FMath::Max(0.0f, EquippedWeaponDefinition->ReloadDuration) : 0.0f;
	if (ReloadDuration <= 0.0f)
	{
		CompleteReload_Server();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UReclaimWeaponComponent::CompleteReload_Server, ReloadDuration, false);
	}
}

void UReclaimWeaponComponent::Server_RequestEquipWeaponByIndex_Implementation(int32 WeaponIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !InitialWeaponDefinitions.IsValidIndex(WeaponIndex))
	{
		return;
	}

	EquipWeaponDefinition_Server(InitialWeaponDefinitions[WeaponIndex]);
}

bool UReclaimWeaponComponent::CanFire() const
{
	const UWorld* World = GetWorld();
	const AReclaimMissionGameState* MissionGameState = World ? World->GetGameState<AReclaimMissionGameState>() : nullptr;
	return GetOwner()
		&& EquippedWeaponDefinition
		&& WeaponState != EReclaimWeaponRuntimeState::Unequipped
		&& WeaponState != EReclaimWeaponRuntimeState::Reloading
		&& FReclaimCombatMath::CanSpendAmmo(AmmoInMagazine, EquippedWeaponDefinition->AmmoPerShot)
		&& IsOwnerAllowedToFire()
		&& MissionGameState
		&& IsFireEnabledMissionPhase(MissionGameState->GetMissionPhase());
}

bool UReclaimWeaponComponent::CanReload() const
{
	return GetOwner()
		&& EquippedWeaponDefinition
		&& WeaponState != EReclaimWeaponRuntimeState::Unequipped
		&& WeaponState != EReclaimWeaponRuntimeState::Reloading
		&& IsOwnerAllowedToFire()
		&& FReclaimCombatMath::CanReload(AmmoInMagazine, ReserveAmmo, EquippedWeaponDefinition->MagazineSize);
}

bool UReclaimWeaponComponent::EquipWeaponDefinition_Server(UReclaimWeaponDefinition* NewWeaponDefinition)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	CancelReload_Server();
	EquippedWeaponDefinition = NewWeaponDefinition;
	LastAcceptedShotSequence = INDEX_NONE;
	LastAcceptedShotSeed = 0;

	if (!EquippedWeaponDefinition)
	{
		AmmoInMagazine = 0;
		ReserveAmmo = 0;
		SetWeaponState_Server(EReclaimWeaponRuntimeState::Unequipped);
		BroadcastAmmoChanged();
		OnWeaponEquipped(nullptr);
		return false;
	}

	AmmoInMagazine = FMath::Max(0, EquippedWeaponDefinition->MagazineSize);
	ReserveAmmo = FMath::Max(0, EquippedWeaponDefinition->ReserveAmmo);
	SetWeaponState_Server(AmmoInMagazine > 0 ? EReclaimWeaponRuntimeState::Idle : EReclaimWeaponRuntimeState::Empty);
	BroadcastAmmoChanged();
	OnWeaponEquipped(EquippedWeaponDefinition);
	return true;
}

bool UReclaimWeaponComponent::EnsureInitialWeaponEquipped_Server()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	if (EquippedWeaponDefinition)
	{
		return true;
	}

	if (InitialWeaponDefinitions.Num() <= 0 || !InitialWeaponDefinitions[0])
	{
		return false;
	}

	return EquipWeaponDefinition_Server(InitialWeaponDefinitions[0]);
}

FString UReclaimWeaponComponent::GetCombatDebugString() const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	const FName WeaponId = EquippedWeaponDefinition ? EquippedWeaponDefinition->WeaponId : NAME_None;
	return FString::Printf(
		TEXT("Authority=%s Weapon=%s Mag=%d Reserve=%d State=%s LastSeq=%d ShotSeed=%d"),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"),
		*WeaponId.ToString(),
		AmmoInMagazine,
		ReserveAmmo,
		*WeaponStateToString(WeaponState),
		LastAcceptedShotSequence,
		LastAcceptedShotSeed);
#endif
}

void UReclaimWeaponComponent::Multicast_ShotConfirmed_Implementation(const FReclaimShotConfirmation& Shot)
{
	OnShotConfirmedDelegate.Broadcast(Shot);
	OnShotConfirmed(Shot);
}

void UReclaimWeaponComponent::Multicast_ReloadStarted_Implementation()
{
	OnReloadStarted();
}

void UReclaimWeaponComponent::Multicast_ReloadCompleted_Implementation()
{
	OnReloadCompleted();
}

void UReclaimWeaponComponent::OnRep_EquippedWeaponDefinition()
{
	OnWeaponEquipped(EquippedWeaponDefinition);
}

void UReclaimWeaponComponent::OnRep_WeaponState(EReclaimWeaponRuntimeState OldState)
{
	if (OldState != WeaponState)
	{
		OnWeaponStateChangedDelegate.Broadcast(WeaponState);
		OnWeaponStateChanged(WeaponState);
	}
}

void UReclaimWeaponComponent::OnRep_AmmoInMagazine(int32 OldAmmoInMagazine)
{
	if (OldAmmoInMagazine != AmmoInMagazine)
	{
		BroadcastAmmoChanged();
	}
}

void UReclaimWeaponComponent::OnRep_ReserveAmmo(int32 OldReserveAmmo)
{
	if (OldReserveAmmo != ReserveAmmo)
	{
		BroadcastAmmoChanged();
	}
}

bool UReclaimWeaponComponent::CanAcceptFireRequest(const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence, FString* OutFailureReason) const
{
	auto Reject = [OutFailureReason](const TCHAR* Reason)
	{
		if (OutFailureReason)
		{
			*OutFailureReason = Reason;
		}
		return false;
	};

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return Reject(TEXT("not authority"));
	}

	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	const AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (!OwnerPawn || !OwnerController || OwnerController->GetPawn() != OwnerPawn)
	{
		return Reject(TEXT("invalid owner pawn/controller"));
	}

	if (!CanFire())
	{
		return Reject(TEXT("cannot fire"));
	}

	if (ShotSequence <= LastAcceptedShotSequence)
	{
		return Reject(TEXT("stale shot sequence"));
	}

	if (AimOrigin.ContainsNaN() || AimDirection.ContainsNaN() || AimDirection.IsNearlyZero())
	{
		return Reject(TEXT("invalid aim"));
	}

	if (!IsAimRequestReasonable(AimOrigin, AimDirection))
	{
		return Reject(TEXT("unreasonable aim"));
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	if (!FReclaimCombatMath::IsFireIntervalReady(CurrentTime, LastAcceptedFireServerTime, EquippedWeaponDefinition->FireInterval))
	{
		return Reject(TEXT("fire interval"));
	}

	return true;
}

bool UReclaimWeaponComponent::CanAcceptReloadRequest() const
{
	return GetOwner()
		&& GetOwner()->HasAuthority()
		&& CanReload();
}

bool UReclaimWeaponComponent::IsOwnerAllowedToFire() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AReclaimPlayerState* ReclaimPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	if (ReclaimPlayerState && ReclaimPlayerState->GetLifeState() != EReclaimPlayerLifeState::Active)
	{
		return false;
	}

	const UReclaimHealthShieldComponent* HealthShield = GetOwner() ? GetOwner()->FindComponentByClass<UReclaimHealthShieldComponent>() : nullptr;
	if (HealthShield && HealthShield->IsDefeated())
	{
		return false;
	}

	const UAbilitySystemComponent* AbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (AbilitySystem && AbilitySystem->HasMatchingGameplayTag(ReclaimGameplayTags::State_CannotFire))
	{
		return false;
	}

	return true;
}

bool UReclaimWeaponComponent::IsAimRequestReasonable(const FVector& AimOrigin, const FVector& AimDirection) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (!OwnerPawn || !OwnerController)
	{
		return false;
	}

	FVector ServerViewOrigin = FVector::ZeroVector;
	FRotator ServerViewRotation = FRotator::ZeroRotator;
	OwnerController->GetPlayerViewPoint(ServerViewOrigin, ServerViewRotation);

	if (MaxAcceptedAimOriginDistance > 0.0f && FVector::DistSquared(ServerViewOrigin, AimOrigin) > FMath::Square(MaxAcceptedAimOriginDistance))
	{
		return false;
	}

	const FVector NormalizedAim = AimDirection.GetSafeNormal();
	const FVector ServerForward = ServerViewRotation.Vector().GetSafeNormal();
	if (NormalizedAim.IsNearlyZero() || ServerForward.IsNearlyZero())
	{
		return false;
	}

	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(MaxAcceptedAimAngleDegrees));
	return FVector::DotProduct(NormalizedAim, ServerForward) >= MinimumDot;
}

void UReclaimWeaponComponent::GetCurrentAim(FVector& OutAimOrigin, FVector& OutAimDirection) const
{
	OutAimOrigin = FVector::ZeroVector;
	OutAimDirection = FVector::ForwardVector;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (OwnerController)
	{
		FRotator ViewRotation = FRotator::ZeroRotator;
		OwnerController->GetPlayerViewPoint(OutAimOrigin, ViewRotation);
		OutAimDirection = ViewRotation.Vector().GetSafeNormal();
		return;
	}

	if (OwnerPawn)
	{
		FRotator EyeRotation = FRotator::ZeroRotator;
		OwnerPawn->GetActorEyesViewPoint(OutAimOrigin, EyeRotation);
		OutAimDirection = EyeRotation.Vector().GetSafeNormal();
	}
}

void UReclaimWeaponComponent::RequestNextLocalShot()
{
	if (!bLocalWantsToFire || !CanFire())
	{
		return;
	}

	FVector AimOrigin = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	GetCurrentAim(AimOrigin, AimDirection);
	RequestFire(AimOrigin, AimDirection.GetSafeNormal(), ++LocalShotSequence);
}

void UReclaimWeaponComponent::ResolveAcceptedFire_Server(const FVector& AimOrigin, const FVector& AimDirection, int32 ShotSequence)
{
	check(GetOwner());
	check(GetOwner()->HasAuthority());
	check(EquippedWeaponDefinition);

	LastAcceptedShotSequence = ShotSequence;
	LastAcceptedFireServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	LastAcceptedShotSeed = FReclaimCombatMath::MakeWeaponShotSeed(GetMissionRunSeed(), GetOwnerPlayerSlot(), EquippedWeaponDefinition->WeaponSeedSalt, ShotSequence);
	AmmoInMagazine = FReclaimCombatMath::ApplyAmmoCostClamped(AmmoInMagazine, EquippedWeaponDefinition->AmmoPerShot);

	FReclaimShotConfirmation Shot;
	Shot.WeaponId = EquippedWeaponDefinition->WeaponId;
	Shot.ShotSequence = ShotSequence;
	Shot.ShotSeed = LastAcceptedShotSeed;
	Shot.AimOrigin = AimOrigin;
	Shot.AimDirection = AimDirection.GetSafeNormal();
	Shot.PelletCount = FMath::Max(1, EquippedWeaponDefinition->PelletCount);

	if (EquippedWeaponDefinition->FireModel == EReclaimWeaponFireModel::Hitscan)
	{
		ResolveHitscanFire_Server(AimOrigin, AimDirection, Shot);
	}
	else if (EquippedWeaponDefinition->FireModel == EReclaimWeaponFireModel::Projectile)
	{
		UWorld* World = GetWorld();
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		TSubclassOf<AReclaimProjectile> ProjectileClass = EquippedWeaponDefinition->ProjectileClass
			? EquippedWeaponDefinition->ProjectileClass
			: TSubclassOf<AReclaimProjectile>(AReclaimProjectile::StaticClass());
		if (World && OwnerPawn)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = GetOwner();
			SpawnParameters.Instigator = OwnerPawn;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			if (AReclaimProjectile* Projectile = World->SpawnActor<AReclaimProjectile>(ProjectileClass, AimOrigin, AimDirection.GetSafeNormal().Rotation(), SpawnParameters))
			{
				Projectile->InitializeDamageProjectile_Server(EquippedWeaponDefinition->Damage, EquippedWeaponDefinition->DamageTypeTag, GetOwner(), 1400.0f, 8.0f);
			}
		}
	}

	SetWeaponState_Server(AmmoInMagazine > 0 ? EReclaimWeaponRuntimeState::Idle : EReclaimWeaponRuntimeState::Empty);
	BroadcastAmmoChanged();
	Multicast_ShotConfirmed(Shot);
}

void UReclaimWeaponComponent::ResolveHitscanFire_Server(const FVector& AimOrigin, const FVector& AimDirection, FReclaimShotConfirmation& InOutShot) const
{
	UWorld* World = GetWorld();
	if (!World || !EquippedWeaponDefinition)
	{
		return;
	}

	TArray<FVector> PelletDirections;
	FReclaimCombatMath::BuildDeterministicSpreadDirections(AimDirection, EquippedWeaponDefinition->SpreadDegrees, EquippedWeaponDefinition->PelletCount, InOutShot.ShotSeed, PelletDirections);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimWeaponHitscan), true, GetOwner());
	QueryParams.AddIgnoredActor(GetOwner());
	if (const AReclaimPlayerCharacter* OwnerCharacter = Cast<AReclaimPlayerCharacter>(GetOwner()))
	{
		if (AReclaimKineticBarrierActor* OwnedBarrier = OwnerCharacter->GetActiveKineticBarrierActor())
		{
			QueryParams.AddIgnoredActor(OwnedBarrier);
		}
	}

	TArray<FReclaimPendingWeaponHit> PendingHits;
	const float Range = FMath::Max(0.0f, EquippedWeaponDefinition->Range);
	const float DamagePerPellet = FMath::Max(0.0f, EquippedWeaponDefinition->Damage) / static_cast<float>(FMath::Max(1, EquippedWeaponDefinition->PelletCount));
	for (const FVector& PelletDirection : PelletDirections)
	{
		FHitResult Hit;
		const FVector TraceEnd = AimOrigin + PelletDirection.GetSafeNormal() * Range;
		if (!World->LineTraceSingleByChannel(Hit, AimOrigin, TraceEnd, EquippedWeaponDefinition->TraceChannel, QueryParams))
		{
			continue;
		}

		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == GetOwner())
		{
			continue;
		}

		FReclaimPendingWeaponHit* ExistingHit = PendingHits.FindByPredicate([HitActor](const FReclaimPendingWeaponHit& Candidate)
		{
			return Candidate.HitActor == HitActor;
		});

		if (!ExistingHit)
		{
			FReclaimPendingWeaponHit& NewHit = PendingHits.AddDefaulted_GetRef();
			NewHit.HitActor = HitActor;
			NewHit.ImpactPoint = Hit.ImpactPoint;
			NewHit.ImpactNormal = Hit.ImpactNormal;
			ExistingHit = &NewHit;
		}

		++ExistingHit->PelletHits;
		ExistingHit->PendingDamage += DamagePerPellet;
	}

	for (const FReclaimPendingWeaponHit& PendingHit : PendingHits)
	{
		FReclaimShotHitSummary Summary;
		Summary.HitActor = PendingHit.HitActor;
		Summary.ImpactPoint = PendingHit.ImpactPoint;
		Summary.ImpactNormal = PendingHit.ImpactNormal.GetSafeNormal();
		Summary.PelletHits = PendingHit.PelletHits;

		if (PendingHit.HitActor)
		{
			if (AReclaimKineticBarrierActor* Barrier = Cast<AReclaimKineticBarrierActor>(PendingHit.HitActor))
			{
				float RemainingDamage = PendingHit.PendingDamage;
				if (Barrier->TryAbsorbDamage_Server(GetOwner(), EquippedWeaponDefinition->DamageTypeTag, PendingHit.PendingDamage, RemainingDamage))
				{
					Summary.DamageApplied = PendingHit.PendingDamage - RemainingDamage;
					if (RemainingDamage > 0.0f)
					{
						if (AActor* ProtectedActor = Barrier->GetOwner())
						{
							if (UReclaimHealthShieldComponent* ProtectedHealthShield = ProtectedActor->FindComponentByClass<UReclaimHealthShieldComponent>())
							{
								FReclaimDamageApplicationResult DamageResult;
								if (ProtectedHealthShield->ApplyDamage_Server(RemainingDamage, GetOwner(), EquippedWeaponDefinition->DamageTypeTag, DamageResult))
								{
									Summary.DamageApplied += DamageResult.ShieldDamage + DamageResult.HealthDamage;
									Summary.bTargetDefeated = DamageResult.bTargetDefeated;
								}
							}
						}
					}
				}
			}
			else if (UReclaimHealthShieldComponent* HealthShield = PendingHit.HitActor->FindComponentByClass<UReclaimHealthShieldComponent>())
			{
				FReclaimDamageApplicationResult DamageResult;
				if (HealthShield->ApplyDamage_Server(PendingHit.PendingDamage, GetOwner(), EquippedWeaponDefinition->DamageTypeTag, DamageResult))
				{
					Summary.DamageApplied = DamageResult.ShieldDamage + DamageResult.HealthDamage;
					Summary.bTargetDefeated = DamageResult.bTargetDefeated;
				}
			}
		}

		InOutShot.HitSummaries.Add(Summary);
	}
}

void UReclaimWeaponComponent::CompleteReload_Server()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !EquippedWeaponDefinition || WeaponState != EReclaimWeaponRuntimeState::Reloading)
	{
		return;
	}

	const FReclaimReloadMathResult ReloadResult = FReclaimCombatMath::ComputeReload(AmmoInMagazine, ReserveAmmo, EquippedWeaponDefinition->MagazineSize);
	AmmoInMagazine = ReloadResult.NewMagazineAmmo;
	ReserveAmmo = ReloadResult.NewReserveAmmo;
	SetWeaponState_Server(AmmoInMagazine > 0 ? EReclaimWeaponRuntimeState::Idle : EReclaimWeaponRuntimeState::Empty);
	BroadcastAmmoChanged();
	Multicast_ReloadCompleted();
}

void UReclaimWeaponComponent::CancelReload_Server()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	if (GetOwner() && GetOwner()->HasAuthority() && WeaponState == EReclaimWeaponRuntimeState::Reloading)
	{
		SetWeaponState_Server(AmmoInMagazine > 0 ? EReclaimWeaponRuntimeState::Idle : EReclaimWeaponRuntimeState::Empty);
	}
}

void UReclaimWeaponComponent::SetWeaponState_Server(EReclaimWeaponRuntimeState NewState)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || WeaponState == NewState)
	{
		return;
	}

	WeaponState = NewState;
	OnWeaponStateChangedDelegate.Broadcast(WeaponState);
	OnWeaponStateChanged(WeaponState);
}

void UReclaimWeaponComponent::BroadcastAmmoChanged()
{
	OnAmmoChangedDelegate.Broadcast(AmmoInMagazine, ReserveAmmo);
	OnAmmoChanged(AmmoInMagazine, ReserveAmmo);
}

int32 UReclaimWeaponComponent::GetOwnerPlayerSlot() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AReclaimPlayerState* ReclaimPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AReclaimPlayerState>() : nullptr;
	return ReclaimPlayerState ? ReclaimPlayerState->GetPlayerSlot() : INDEX_NONE;
}

int32 UReclaimWeaponComponent::GetMissionRunSeed() const
{
	const UWorld* World = GetWorld();
	const AReclaimMissionGameState* MissionGameState = World ? World->GetGameState<AReclaimMissionGameState>() : nullptr;
	return MissionGameState ? MissionGameState->GetRunSeed() : 0;
}

void UReclaimWeaponComponent::OnLocalFireRequested_Implementation()
{
}

void UReclaimWeaponComponent::OnShotConfirmed_Implementation(const FReclaimShotConfirmation& Shot)
{
}

void UReclaimWeaponComponent::OnReloadStarted_Implementation()
{
}

void UReclaimWeaponComponent::OnReloadCompleted_Implementation()
{
}

void UReclaimWeaponComponent::OnWeaponEquipped_Implementation(UReclaimWeaponDefinition* NewWeaponDefinition)
{
}

void UReclaimWeaponComponent::OnWeaponStateChanged_Implementation(EReclaimWeaponRuntimeState NewState)
{
}

void UReclaimWeaponComponent::OnAmmoChanged_Implementation(int32 NewMagazineAmmo, int32 NewReserveAmmo)
{
}
