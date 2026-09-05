// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ReclaimPlayerCharacter.h"

#include "AbilitySystem/ReclaimAbilitySystemComponent.h"
#include "AbilitySystem/ReclaimAbilityDefinition.h"
#include "AbilitySystem/ReclaimAttributeSet.h"
#include "AbilitySystem/ReclaimKineticBarrierActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ReclaimHealthShieldComponent.h"
#include "Components/ReclaimInteractionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ReclaimWeaponComponent.h"
#include "Core/ReclaimGameplayTags.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameModes/ReclaimMissionGameState.h"
#include "Player/ReclaimPlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Player/ReclaimPlayerState.h"
#include "Roles/ReclaimRoleDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogReclaimPlayerCharacter, Log, All);

namespace
{
	FString RoleToDebugString(EReclaimRole Role)
	{
		if (const UEnum* RoleEnum = StaticEnum<EReclaimRole>())
		{
			return RoleEnum->GetNameStringByValue(static_cast<int64>(Role));
		}

		return FString::FromInt(static_cast<int32>(Role));
	}

	FString NetRoleToDebugString(ENetRole NetRole)
	{
		switch (NetRole)
		{
		case ROLE_Authority:
			return TEXT("Authority");
		case ROLE_AutonomousProxy:
			return TEXT("AutonomousProxy");
		case ROLE_SimulatedProxy:
			return TEXT("SimulatedProxy");
		case ROLE_None:
		default:
			return TEXT("None");
		}
	}
}

AReclaimPlayerCharacter::AReclaimPlayerCharacter()
{
	bReplicates = true;
	//bReplicateMovement  = true;
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;
	MovementComponent->MaxWalkSpeed = 600.0f;
	MovementComponent->JumpZVelocity = 600.0f;
	MovementComponent->AirControl = 0.35f;
	MovementComponent->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;

	SetNetUpdateFrequency(60.f);
	SetMinNetUpdateFrequency(30.f);

	FirstPersonArmsMeshA = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArmsMesh01"));
	FirstPersonArmsMeshA->SetupAttachment(GetMesh());
	FirstPersonArmsMeshA->SetOnlyOwnerSee(true);
	FirstPersonArmsMeshA->SetOwnerNoSee(false);
	FirstPersonArmsMeshA->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonArmsMeshA->SetCastShadow(false);

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonArmsMeshA);
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	if (USkeletalMeshComponent* ThirdPersonMesh = GetMesh())
	{
		ThirdPersonMesh->SetOwnerNoSee(true);
		ThirdPersonMesh->SetOnlyOwnerSee(false);
		ThirdPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	AbilitySystemComponent = CreateDefaultSubobject<UReclaimAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UReclaimAttributeSet>(TEXT("AttributeSet"));
	HealthShieldComponent = CreateDefaultSubobject<UReclaimHealthShieldComponent>(TEXT("HealthShieldComponent"));
	WeaponComponent = CreateDefaultSubobject<UReclaimWeaponComponent>(TEXT("WeaponComponent"));
	InteractionComponent = CreateDefaultSubobject<UReclaimInteractionComponent>(TEXT("InteractionComponent"));
}

UAbilitySystemComponent* AReclaimPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AReclaimPlayerCharacter::CanReceiveRepairFrom_Implementation(AActor* SourceActor) const
{
	const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState();
	return HasAuthority()
		&& SourceActor != nullptr
		&& ReclaimPlayerState
		&& ReclaimPlayerState->GetLifeState() == EReclaimPlayerLifeState::Active
		&& HealthShieldComponent
		&& (HealthShieldComponent->GetHealth() < HealthShieldComponent->GetMaxHealth() || HealthShieldComponent->GetShield() < HealthShieldComponent->GetMaxShield());
}

float AReclaimPlayerCharacter::ApplyRepair_Server_Implementation(AActor* SourceActor, float HealthAmount, float ShieldAmount)
{
	if (!HasAuthority() || !HealthShieldComponent)
	{
		return 0.0f;
	}

	FReclaimRepairApplicationResult Result;
	return HealthShieldComponent->ApplyRepair_Server(HealthAmount, ShieldAmount, SourceActor, Result) ? Result.HealthRestored + Result.ShieldRestored : 0.0f;
}

void AReclaimPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		DefaultMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
	}

	RefreshPerspectiveVisibility();
	if (const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState())
	{
		ApplyReplicatedLifeState(ReclaimPlayerState->GetLifeState());
	}
	else
	{
		ApplyReplicatedLifeState(EReclaimPlayerLifeState::Active);
	}
	LogM1NetworkDebugState(TEXT("BeginPlay"));
}

void AReclaimPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReclaimPlayerCharacter, GrantedRoleAbilityDefinitions);
}

void AReclaimPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	RefreshPerspectiveVisibility();
	if (const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState())
	{
		ApplyReplicatedLifeState(ReclaimPlayerState->GetLifeState());
	}
	LogM1NetworkDebugState(TEXT("PossessedBy"));
}

void AReclaimPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	RefreshPerspectiveVisibility();
	if (const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState())
	{
		ApplyReplicatedLifeState(ReclaimPlayerState->GetLifeState());
	}
	LogM1NetworkDebugState(TEXT("OnRep_PlayerState"));
}

void AReclaimPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	if (IsLocallyControlled() && GetWorld() && GetWorld()->GetGameState<AReclaimMissionGameState>())
	{
		if (AReclaimPlayerController* ReclaimPlayerController = Cast<AReclaimPlayerController>(GetController()))
		{
			ReclaimPlayerController->EnterMissionInputMode();
		}
	}

	AddDefaultInputMappingContext();
	RefreshPerspectiveVisibility();
	LogM1NetworkDebugState(TEXT("PawnClientRestart"));
}

void AReclaimPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	bool bBoundEnhancedMove = false;
	bool bBoundEnhancedLook = false;
	bool bBoundEnhancedJump = false;
	bool bBoundEnhancedFire = false;
	bool bBoundEnhancedReload = false;
	bool bBoundEnhancedSwitchWeapon = false;
	bool bBoundEnhancedAbilityOne = false;
	bool bBoundEnhancedAbilityTwo = false;
	bool bBoundEnhancedInteract = false;

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AReclaimPlayerCharacter::Move);
			bBoundEnhancedMove = true;
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AReclaimPlayerCharacter::Look);
			bBoundEnhancedLook = true;
		}

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AReclaimPlayerCharacter::StartJump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AReclaimPlayerCharacter::StopJump);
			bBoundEnhancedJump = true;
		}

		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AReclaimPlayerCharacter::FirePressed);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AReclaimPlayerCharacter::FireReleased);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Canceled, this, &AReclaimPlayerCharacter::FireReleased);
			bBoundEnhancedFire = true;
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AReclaimPlayerCharacter::ReloadWeapon);
			bBoundEnhancedReload = true;
		}

		if (SwitchWeaponAction)
		{
			EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AReclaimPlayerCharacter::SwitchWeapon);
			bBoundEnhancedSwitchWeapon = true;
		}

		if (AbilityOneAction)
		{
			EnhancedInputComponent->BindAction(AbilityOneAction, ETriggerEvent::Started, this, &AReclaimPlayerCharacter::AbilityOnePressed);
			bBoundEnhancedAbilityOne = true;
		}

		if (AbilityTwoAction)
		{
			EnhancedInputComponent->BindAction(AbilityTwoAction, ETriggerEvent::Started, this, &AReclaimPlayerCharacter::AbilityTwoPressed);
			bBoundEnhancedAbilityTwo = true;
		}

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AReclaimPlayerCharacter::InteractPressed);
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AReclaimPlayerCharacter::InteractReleased);
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Canceled, this, &AReclaimPlayerCharacter::InteractReleased);
			bBoundEnhancedInteract = true;
		}
	}

	if (!bBoundEnhancedMove)
	{
		PlayerInputComponent->BindAxis(FName(TEXT("Reclaim_MoveForward")), this, &AReclaimPlayerCharacter::MoveForward);
		PlayerInputComponent->BindAxis(FName(TEXT("Reclaim_MoveRight")), this, &AReclaimPlayerCharacter::MoveRight);
	}

	if (!bBoundEnhancedLook)
	{
		PlayerInputComponent->BindAxis(FName(TEXT("Reclaim_Turn")), this, &AReclaimPlayerCharacter::Turn);
		PlayerInputComponent->BindAxis(FName(TEXT("Reclaim_LookUp")), this, &AReclaimPlayerCharacter::LookUp);
	}

	if (!bBoundEnhancedJump)
	{
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Jump")), IE_Pressed, this, &AReclaimPlayerCharacter::StartJump);
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Jump")), IE_Released, this, &AReclaimPlayerCharacter::StopJump);
	}

	if (!bBoundEnhancedFire)
	{
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Fire")), IE_Pressed, this, &AReclaimPlayerCharacter::FirePressed);
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Fire")), IE_Released, this, &AReclaimPlayerCharacter::FireReleased);
	}

	if (!bBoundEnhancedReload)
	{
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Reload")), IE_Pressed, this, &AReclaimPlayerCharacter::ReloadWeapon);
	}

	if (!bBoundEnhancedSwitchWeapon)
	{
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_SwitchWeapon")), IE_Pressed, this, &AReclaimPlayerCharacter::SwitchWeapon);
	}

	if (!bBoundEnhancedAbilityOne)
	{
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Ability1")), IE_Pressed, this, &AReclaimPlayerCharacter::AbilityOnePressed);
	}

	if (!bBoundEnhancedAbilityTwo)
	{
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Ability2")), IE_Pressed, this, &AReclaimPlayerCharacter::AbilityTwoPressed);
	}

	if (!bBoundEnhancedInteract)
	{
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Interact")), IE_Pressed, this, &AReclaimPlayerCharacter::InteractPressed);
		PlayerInputComponent->BindAction(FName(TEXT("Reclaim_Interact")), IE_Released, this, &AReclaimPlayerCharacter::InteractReleased);
	}
}

AReclaimPlayerState* AReclaimPlayerCharacter::GetReclaimPlayerState() const
{
	return GetPlayerState<AReclaimPlayerState>();
}

EReclaimRole AReclaimPlayerCharacter::GetSelectedRoleFromPlayerState() const
{
	const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState();
	return ReclaimPlayerState ? ReclaimPlayerState->GetSelectedRole() : EReclaimRole::None;
}

int32 AReclaimPlayerCharacter::GetPlayerSlotFromPlayerState() const
{
	const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState();
	return ReclaimPlayerState ? ReclaimPlayerState->GetPlayerSlot() : INDEX_NONE;
}

void AReclaimPlayerCharacter::GetCombatAim(FVector& OutAimOrigin, FVector& OutAimDirection) const
{
	OutAimOrigin = FVector::ZeroVector;
	OutAimDirection = FVector::ForwardVector;

	if (IsLocallyControlled() && FirstPersonCameraComponent)
	{
		OutAimOrigin = FirstPersonCameraComponent->GetComponentLocation();
		OutAimDirection = FirstPersonCameraComponent->GetForwardVector().GetSafeNormal();
		return;
	}

	if (Controller)
	{
		FRotator ViewRotation = FRotator::ZeroRotator;
		Controller->GetPlayerViewPoint(OutAimOrigin, ViewRotation);
		OutAimDirection = ViewRotation.Vector().GetSafeNormal();
		return;
	}

	FRotator EyeRotation = FRotator::ZeroRotator;
	GetActorEyesViewPoint(OutAimOrigin, EyeRotation);
	OutAimDirection = EyeRotation.Vector().GetSafeNormal();
}

void AReclaimPlayerCharacter::GrantRoleAbilities_Server(UReclaimRoleDefinition* RoleDefinition)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	ClearRoleAbilities_Server();
	GrantedRoleAbilityDefinitions.Reset();

	if (!RoleDefinition)
	{
		return;
	}

	GrantedRoleAbilityDefinitions.Reserve(RoleDefinition->CoreAbilities.Num());
	for (int32 AbilitySlot = 0; AbilitySlot < RoleDefinition->CoreAbilities.Num(); ++AbilitySlot)
	{
		UReclaimAbilityDefinition* AbilityDefinition = RoleDefinition->CoreAbilities[AbilitySlot];
		GrantedRoleAbilityDefinitions.Add(AbilityDefinition);
		if (!AbilityDefinition || !AbilityDefinition->AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(AbilityDefinition->AbilityClass, 1, AbilitySlot, AbilityDefinition);
		AbilitySpec.DynamicAbilityTags.AppendTags(AbilityDefinition->AbilityTags);
		GrantedRoleAbilityHandles.Add(AbilitySystemComponent->GiveAbility(AbilitySpec));
		AbilitySystemComponent->RefreshAbilityRuntimeState_Server(AbilityDefinition);
	}
}

void AReclaimPlayerCharacter::ClearRoleAbilities_Server()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : GrantedRoleAbilityHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(Handle);
		}
	}

	GrantedRoleAbilityHandles.Reset();
	GrantedRoleAbilityDefinitions.Reset();
}

void AReclaimPlayerCharacter::InitializeMissionPawn_Server(UReclaimRoleDefinition* RoleDefinition)
{
	if (!HasAuthority())
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (HealthShieldComponent)
	{
		HealthShieldComponent->ResetAttributesToConfiguredDefaults_Server();
	}

	if (WeaponComponent)
	{
		WeaponComponent->EnsureInitialWeaponEquipped_Server();
	}

	GrantRoleAbilities_Server(RoleDefinition);

	if (const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState())
	{
		ApplyReplicatedLifeState(ReclaimPlayerState->GetLifeState());
	}
}

void AReclaimPlayerCharacter::PrepareForRedeployCleanup_Server()
{
	if (!HasAuthority())
	{
		return;
	}

	EndActiveKineticBarrier_Server();
	ClearRoleAbilities_Server();
	ApplyReplicatedLifeState(EReclaimPlayerLifeState::Redeploying);
}

void AReclaimPlayerCharacter::RequestActivateRoleAbilityBySlot(int32 AbilitySlot)
{
	const FReclaimAbilityActivationPayload Payload = BuildAbilityActivationPayload(AbilitySlot);
	Server_RequestActivateRoleAbility(AbilitySlot, Payload);
}

UReclaimAbilityDefinition* AReclaimPlayerCharacter::GetGrantedAbilityDefinitionBySlot(int32 AbilitySlot) const
{
	return GrantedRoleAbilityDefinitions.IsValidIndex(AbilitySlot) ? GrantedRoleAbilityDefinitions[AbilitySlot] : nullptr;
}

FGameplayTag AReclaimPlayerCharacter::GetGrantedAbilityTagBySlot(int32 AbilitySlot) const
{
	const UReclaimAbilityDefinition* AbilityDefinition = GetGrantedAbilityDefinitionBySlot(AbilitySlot);
	return AbilityDefinition ? AbilityDefinition->GetPrimaryAbilityTag() : FGameplayTag();
}

bool AReclaimPlayerCharacter::ConsumePendingAbilityActivationPayload(FReclaimAbilityActivationPayload& OutPayload)
{
	if (!bHasPendingAbilityActivationPayload)
	{
		return false;
	}

	OutPayload = PendingAbilityActivationPayload;
	bHasPendingAbilityActivationPayload = false;
	return true;
}

void AReclaimPlayerCharacter::ApplyReplicatedLifeState(EReclaimPlayerLifeState NewLifeState)
{
	SetLifeStateGameplayTag(ReclaimGameplayTags::State_Downed, NewLifeState == EReclaimPlayerLifeState::Downed);
	SetLifeStateGameplayTag(ReclaimGameplayTags::State_Redeploying, NewLifeState == EReclaimPlayerLifeState::Redeploying);
	const bool bCombatDisabled = NewLifeState != EReclaimPlayerLifeState::Active;
	SetLifeStateGameplayTag(ReclaimGameplayTags::State_CannotFire, bCombatDisabled);
	SetLifeStateGameplayTag(ReclaimGameplayTags::State_CannotUseAbility, bCombatDisabled);

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent)
	{
		if (NewLifeState == EReclaimPlayerLifeState::Active)
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
			MovementComponent->MaxWalkSpeed = DefaultMaxWalkSpeed > 0.0f ? DefaultMaxWalkSpeed : 600.0f;
		}
		else if (NewLifeState == EReclaimPlayerLifeState::Downed)
		{
			StopJumping();
			MovementComponent->SetMovementMode(MOVE_Walking);
			MovementComponent->MaxWalkSpeed = DownedMaxWalkSpeed;
		}
		else
		{
			StopJumping();
			MovementComponent->DisableMovement();
		}
	}

	const bool bCollisionEnabled = NewLifeState == EReclaimPlayerLifeState::Active || NewLifeState == EReclaimPlayerLifeState::Downed;
	SetActorEnableCollision(bCollisionEnabled);
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(bCollisionEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}

	if (HasAuthority() && bCombatDisabled)
	{
		if (InteractionComponent)
		{
			InteractionComponent->CancelRevive_Server();
		}
		if (NewLifeState == EReclaimPlayerLifeState::Destroyed || NewLifeState == EReclaimPlayerLifeState::Redeploying)
		{
			EndActiveKineticBarrier_Server();
		}
	}

	OnLifeStateChanged(NewLifeState);
}

void AReclaimPlayerCharacter::Server_RequestActivateRoleAbility_Implementation(int32 AbilitySlot, FReclaimAbilityActivationPayload Payload)
{
	if (!HasAuthority() || !AbilitySystemComponent || !GrantedRoleAbilityHandles.IsValidIndex(AbilitySlot))
	{
		return;
	}

	const AController* OwningController = GetController();
	if (!OwningController || OwningController->GetPawn() != this)
	{
		return;
	}

	const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState();
	if (!ReclaimPlayerState || ReclaimPlayerState->GetLifeState() != EReclaimPlayerLifeState::Active)
	{
		return;
	}

	const FGameplayAbilitySpecHandle Handle = GrantedRoleAbilityHandles[AbilitySlot];
	if (!Handle.IsValid())
	{
		return;
	}

	Payload.AbilitySlot = AbilitySlot;
	PendingAbilityActivationPayload = Payload;
	bHasPendingAbilityActivationPayload = true;

	if (!AbilitySystemComponent->TryActivateAbility(Handle))
	{
		bHasPendingAbilityActivationPayload = false;
	}
}

void AReclaimPlayerCharacter::Multicast_RoleAbilityConfirmed_Implementation(FGameplayTag AbilityTag, FVector_NetQuantize Location)
{
	OnRoleAbilityConfirmed(AbilityTag, Location);
}

FString AReclaimPlayerCharacter::GetM1NetworkDebugString() const
{
#if UE_BUILD_SHIPPING
	return FString();
#else
	const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState();
	const EReclaimRole SelectedRole = ReclaimPlayerState ? ReclaimPlayerState->GetSelectedRole() : EReclaimRole::None;
	const int32 PlayerSlot = ReclaimPlayerState ? ReclaimPlayerState->GetPlayerSlot() : INDEX_NONE;

	return FString::Printf(
		TEXT("Authority=%s LocalRole=%s RemoteRole=%s SelectedRole=%s PlayerSlot=%d IsLocallyControlled=%s PawnClass=%s"),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		*NetRoleToDebugString(GetLocalRole()),
		*NetRoleToDebugString(GetRemoteRole()),
		*RoleToDebugString(SelectedRole),
		PlayerSlot,
		IsLocallyControlled() ? TEXT("true") : TEXT("false"),
		*GetClass()->GetName());
#endif
}

void AReclaimPlayerCharacter::AddDefaultInputMappingContext() const
{
	if (!DefaultInputMappingContext)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
	if (InputSubsystem)
	{
		InputSubsystem->AddMappingContext(DefaultInputMappingContext, InputMappingPriority);
	}
}

void AReclaimPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	MoveForward(MovementVector.Y);
	MoveRight(MovementVector.X);
}

void AReclaimPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	Turn(LookAxisVector.X);
	LookUp(LookAxisVector.Y);
}

void AReclaimPlayerCharacter::MoveForward(float Value)
{
	if (!Controller || FMath::IsNearlyZero(Value) || !CanAcceptMovementInput())
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AReclaimPlayerCharacter::MoveRight(float Value)
{
	if (!Controller || FMath::IsNearlyZero(Value) || !CanAcceptMovementInput())
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AReclaimPlayerCharacter::Turn(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddControllerYawInput(Value);
	}
}

void AReclaimPlayerCharacter::LookUp(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddControllerPitchInput(Value);
	}
}

void AReclaimPlayerCharacter::StartJump()
{
	if (!IsLifeStateActive())
	{
		return;
	}

	Jump();
}

void AReclaimPlayerCharacter::StopJump()
{
	StopJumping();
}

void AReclaimPlayerCharacter::FirePressed()
{
	if (WeaponComponent && IsLifeStateActive())
	{
		WeaponComponent->RequestFirePressed();
	}
}

void AReclaimPlayerCharacter::FireReleased()
{
	if (WeaponComponent)
	{
		WeaponComponent->RequestFireReleased();
	}
}

void AReclaimPlayerCharacter::ReloadWeapon()
{
	if (WeaponComponent && IsLifeStateActive())
	{
		WeaponComponent->RequestReload();
	}
}

void AReclaimPlayerCharacter::SwitchWeapon()
{
	if (WeaponComponent && IsLifeStateActive())
	{
		WeaponComponent->RequestNextWeapon();
	}
}

void AReclaimPlayerCharacter::AbilityOnePressed()
{
	if (IsLifeStateActive())
	{
		RequestActivateRoleAbilityBySlot(0);
	}
}

void AReclaimPlayerCharacter::AbilityTwoPressed()
{
	if (IsLifeStateActive())
	{
		RequestActivateRoleAbilityBySlot(1);
	}
}

void AReclaimPlayerCharacter::InteractPressed()
{
	if (!InteractionComponent || !IsLifeStateActive())
	{
		return;
	}

	InteractionComponent->RequestBeginRevive(FindInteractionTarget(300.0f));
}

void AReclaimPlayerCharacter::InteractReleased()
{
	if (InteractionComponent)
	{
		InteractionComponent->RequestCancelRevive();
	}
}

FReclaimAbilityActivationPayload AReclaimPlayerCharacter::BuildAbilityActivationPayload(int32 AbilitySlot) const
{
	FReclaimAbilityActivationPayload Payload;
	Payload.AbilitySlot = AbilitySlot;
	FVector AimOrigin = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	GetCombatAim(AimOrigin, AimDirection);
	Payload.AimOrigin = AimOrigin;
	Payload.AimDirection = AimDirection.GetSafeNormal();

	const UReclaimAbilityDefinition* AbilityDefinition = GetGrantedAbilityDefinitionBySlot(AbilitySlot);
	const float TraceRange = AbilityDefinition && AbilityDefinition->Range > 0.0f ? AbilityDefinition->Range : AbilityTargetTraceRange;
	const FVector TraceStart = Payload.AimOrigin;
	const FVector TraceEnd = TraceStart + FVector(Payload.AimDirection).GetSafeNormal() * FMath::Max(0.0f, TraceRange);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimAbilityTargetTrace), true, this);
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;
	const UWorld* World = GetWorld();
	if (World && World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		Payload.TargetActor = Hit.GetActor();
		Payload.TargetLocation = Hit.ImpactPoint;
		Payload.bHasTargetLocation = true;
	}
	else
	{
		Payload.TargetLocation = TraceEnd;
		Payload.bHasTargetLocation = true;
	}

	return Payload;
}

AActor* AReclaimPlayerCharacter::FindInteractionTarget(float Range) const
{
	FVector AimOrigin = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	GetCombatAim(AimOrigin, AimDirection);

	const FVector TraceEnd = AimOrigin + AimDirection.GetSafeNormal() * FMath::Max(0.0f, Range);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReclaimInteractTrace), true, this);
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;
	const UWorld* World = GetWorld();
	if (World && World->LineTraceSingleByChannel(Hit, AimOrigin, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Hit.GetActor();
	}

	return nullptr;
}

bool AReclaimPlayerCharacter::IsLifeStateActive() const
{
	const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState();
	return !ReclaimPlayerState || ReclaimPlayerState->GetLifeState() == EReclaimPlayerLifeState::Active;
}

bool AReclaimPlayerCharacter::CanAcceptMovementInput() const
{
	const AReclaimPlayerState* ReclaimPlayerState = GetReclaimPlayerState();
	if (!ReclaimPlayerState)
	{
		return true;
	}

	const EReclaimPlayerLifeState LifeState = ReclaimPlayerState->GetLifeState();
	return LifeState == EReclaimPlayerLifeState::Active || LifeState == EReclaimPlayerLifeState::Downed;
}

void AReclaimPlayerCharacter::RegisterKineticBarrier_Server(AReclaimKineticBarrierActor* BarrierActor)
{
	if (!HasAuthority() || !BarrierActor)
	{
		return;
	}

	if (ActiveKineticBarrierActor && ActiveKineticBarrierActor != BarrierActor)
	{
		ActiveKineticBarrierActor->EndBarrier_Server();
	}

	ActiveKineticBarrierActor = BarrierActor;
	SetLifeStateGameplayTag(ReclaimGameplayTags::State_Shielded, true);
}

void AReclaimPlayerCharacter::UnregisterKineticBarrier_Server(AReclaimKineticBarrierActor* BarrierActor)
{
	if (!HasAuthority() || ActiveKineticBarrierActor != BarrierActor)
	{
		return;
	}

	ActiveKineticBarrierActor = nullptr;
	SetLifeStateGameplayTag(ReclaimGameplayTags::State_Shielded, false);
}

bool AReclaimPlayerCharacter::TryAbsorbDamageWithKineticBarrier_Server(AActor* DamageInstigator, FGameplayTag DamageTypeTag, float IncomingDamage, float& OutRemainingDamage)
{
	OutRemainingDamage = FMath::Max(0.0f, IncomingDamage);
	if (!HasAuthority() || !ActiveKineticBarrierActor || OutRemainingDamage <= 0.0f)
	{
		return false;
	}

	return ActiveKineticBarrierActor->TryAbsorbDamage_Server(DamageInstigator, DamageTypeTag, OutRemainingDamage, OutRemainingDamage);
}

void AReclaimPlayerCharacter::EndActiveKineticBarrier_Server()
{
	if (HasAuthority() && ActiveKineticBarrierActor)
	{
		AReclaimKineticBarrierActor* BarrierToEnd = ActiveKineticBarrierActor;
		ActiveKineticBarrierActor = nullptr;
		BarrierToEnd->EndBarrier_Server();
		SetLifeStateGameplayTag(ReclaimGameplayTags::State_Shielded, false);
	}
}

void AReclaimPlayerCharacter::SetLifeStateGameplayTag(FGameplayTag Tag, bool bEnabled)
{
	if (!AbilitySystemComponent || !Tag.IsValid())
	{
		return;
	}

	AbilitySystemComponent->SetLooseGameplayTagCount(Tag, bEnabled ? 1 : 0);
}

void AReclaimPlayerCharacter::RefreshPerspectiveVisibility()
{
	const bool bLocalOwner = IsLocallyControlled();

	if (FirstPersonCameraComponent)
	{
		FirstPersonCameraComponent->SetActive(bLocalOwner);
	}

	if (USkeletalMeshComponent* ThirdPersonMesh = GetMesh())
	{
		ThirdPersonMesh->SetOwnerNoSee(bLocalOwner);
		ThirdPersonMesh->SetOnlyOwnerSee(false);
		ThirdPersonMesh->SetVisibility(true, true);
	}

	if (FirstPersonArmsMeshA)
	{
		FirstPersonArmsMeshA->SetOwnerNoSee(false);
		FirstPersonArmsMeshA->SetOnlyOwnerSee(true);
		FirstPersonArmsMeshA->SetVisibility(bLocalOwner, true);
	}

	OnPerspectiveModeChanged(bLocalOwner);
}

void AReclaimPlayerCharacter::LogM1NetworkDebugState(const TCHAR* Context) const
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogReclaimPlayerCharacter, Verbose, TEXT("[%s] %s"), Context ? Context : TEXT("M1"), *GetM1NetworkDebugString());
#endif
}

void AReclaimPlayerCharacter::OnPerspectiveModeChanged_Implementation(bool bIsLocalOwner)
{
}

void AReclaimPlayerCharacter::OnRoleAbilityConfirmed_Implementation(FGameplayTag AbilityTag, FVector Location)
{
}

void AReclaimPlayerCharacter::OnLifeStateChanged_Implementation(EReclaimPlayerLifeState NewLifeState)
{
}
