#include "Scripting/ReclaimScriptBindingComponent.h"

#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Scripting/ReclaimScriptRuntimeSubsystem.h"

UReclaimScriptBindingComponent::UReclaimScriptBindingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UReclaimScriptBindingComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRegister)
    {
        RegisterScriptBinding();
    }
}

void UReclaimScriptBindingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bRegistered)
    {
        SendScriptEvent(TEXT("Lifecycle.EndPlay"), GetOwner(), TEXT("{}"));
        UnregisterScriptBinding();
    }

    Super::EndPlay(EndPlayReason);
}

bool UReclaimScriptBindingComponent::ShouldRunInThisInstance() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    switch (ExecutionPolicy)
    {
    case EReclaimScriptExecutionPolicy::EveryInstance:
        return true;
    case EReclaimScriptExecutionPolicy::AuthorityOnly:
        return Owner->HasAuthority();
    case EReclaimScriptExecutionPolicy::NonAuthorityOnly:
        return !Owner->HasAuthority();
    default:
        return false;
    }
}

void UReclaimScriptBindingComponent::RegisterScriptBinding()
{
    if (bRegistered || !ShouldRunInThisInstance())
    {
        return;
    }

    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UReclaimScriptRuntimeSubsystem* Runtime =
        GameInstance ? GameInstance->GetSubsystem<UReclaimScriptRuntimeSubsystem>() : nullptr;

    if (!Runtime)
    {
        return;
    }

    bRegistered = true;
    Runtime->RegisterBinding(this);
    Runtime->DispatchBindingEvent(this, TEXT("Lifecycle.BeginPlay"), GetOwner(), TEXT("{}"));
}

void UReclaimScriptBindingComponent::UnregisterScriptBinding()
{
    if (!bRegistered)
    {
        return;
    }

    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UReclaimScriptRuntimeSubsystem* Runtime =
        GameInstance ? GameInstance->GetSubsystem<UReclaimScriptRuntimeSubsystem>() : nullptr;

    if (Runtime)
    {
        Runtime->UnregisterBinding(this);
    }

    bRegistered = false;
}

void UReclaimScriptBindingComponent::SendScriptEvent(
    const FString& EventName,
    UObject* ContextObject,
    const FString& PayloadJson)
{
    if (!bRegistered || EventName.IsEmpty())
    {
        return;
    }

    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UReclaimScriptRuntimeSubsystem* Runtime =
        GameInstance ? GameInstance->GetSubsystem<UReclaimScriptRuntimeSubsystem>() : nullptr;

    if (Runtime)
    {
        Runtime->DispatchBindingEvent(this, EventName, ContextObject, PayloadJson);
    }
}

void UReclaimScriptBindingComponent::EmitScriptCommand(
    const FString& CommandName,
    UObject* ContextObject,
    const FString& PayloadJson)
{
    if (!bRegistered || CommandName.IsEmpty())
    {
        return;
    }

    OnScriptCommand.Broadcast(CommandName, ContextObject, PayloadJson);
}
