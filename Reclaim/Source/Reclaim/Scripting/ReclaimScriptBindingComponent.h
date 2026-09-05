#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReclaimScriptBindingComponent.generated.h"

class UDataAsset;

UENUM(BlueprintType)
enum class EReclaimScriptExecutionPolicy : uint8
{
    EveryInstance UMETA(DisplayName = "Every Instance"),
    AuthorityOnly UMETA(DisplayName = "Authority Only"),
    NonAuthorityOnly UMETA(DisplayName = "Non Authority Only")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FReclaimScriptCommandDelegate,
    FString, CommandName,
    UObject*, ContextObject,
    FString, PayloadJson);

UCLASS(ClassGroup = (Reclaim), meta = (BlueprintSpawnableComponent))
class RECLAIM_API UReclaimScriptBindingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UReclaimScriptBindingComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reclaim|Script")
    FName ScriptId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reclaim|Script")
    TObjectPtr<UDataAsset> ScriptConfig = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reclaim|Script")
    EReclaimScriptExecutionPolicy ExecutionPolicy = EReclaimScriptExecutionPolicy::AuthorityOnly;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reclaim|Script")
    bool bAutoRegister = true;

    UPROPERTY(BlueprintAssignable, Category = "Reclaim|Script")
    FReclaimScriptCommandDelegate OnScriptCommand;

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void RegisterScriptBinding();

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void UnregisterScriptBinding();

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void SendScriptEvent(
        const FString& EventName,
        UObject* ContextObject,
        const FString& PayloadJson);

    UFUNCTION(BlueprintCallable, Category = "Reclaim|Script")
    void EmitScriptCommand(
        const FString& CommandName,
        UObject* ContextObject,
        const FString& PayloadJson);

    UFUNCTION(BlueprintPure, Category = "Reclaim|Script")
    FString GetScriptIdString() const { return ScriptId.ToString(); }

    UFUNCTION(BlueprintPure, Category = "Reclaim|Script")
    UDataAsset* GetScriptConfig() const { return ScriptConfig.Get(); }

    UFUNCTION(BlueprintPure, Category = "Reclaim|Script")
    bool IsScriptBindingRegistered() const { return bRegistered; }

    UFUNCTION(BlueprintPure, Category = "Reclaim|Script")
    bool ShouldRunInThisInstance() const;

private:
    bool bRegistered = false;
};
