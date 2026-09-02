#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ReclaimMainMenuPlayerController.generated.h"

class UUserWidget;

UCLASS(Blueprintable)
class RECLAIM_API AReclaimMainMenuPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AReclaimMainMenuPlayerController();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reclaim|MainMenu")
    TSubclassOf<UUserWidget> MainMenuWidgetClass;

private:
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> MainMenuWidget;
};
