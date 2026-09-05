#include "Player/ReclaimMainMenuPlayerController.h"

#include "Blueprint/UserWidget.h"

AReclaimMainMenuPlayerController::AReclaimMainMenuPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AReclaimMainMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    if (!MainMenuWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Reclaim MainMenu] MainMenuWidgetClass is not configured."));
        return;
    }

    if (!MainMenuWidget)
    {
        MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
    }

    if (!MainMenuWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[Reclaim MainMenu] Failed to create MainMenu widget."));
        return;
    }

    if (!MainMenuWidget->IsInViewport())
    {
        MainMenuWidget->AddToViewport(1000);
    }

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}
