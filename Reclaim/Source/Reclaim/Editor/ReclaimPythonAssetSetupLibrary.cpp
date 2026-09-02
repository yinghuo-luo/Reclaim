// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/ReclaimPythonAssetSetupLibrary.h"

#if WITH_EDITOR
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ContentWidget.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "WidgetBlueprint.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogReclaimAssetSetup, Log, All);

#if WITH_EDITOR
namespace
{
	void EnsureWidgetGuid(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, bool& bChanged)
	{
		if (!WidgetBlueprint || !Widget)
		{
			return;
		}

#if WITH_EDITORONLY_DATA
		if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			WidgetBlueprint->OnVariableAdded(Widget->GetFName());
			bChanged = true;
		}
#endif
	}

	UPanelWidget* EnsureRootPanel(UWidgetBlueprint* WidgetBlueprint, bool& bChanged)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return nullptr;
		}

		if (UPanelWidget* ExistingPanel = Cast<UPanelWidget>(WidgetBlueprint->WidgetTree->RootWidget))
		{
			EnsureWidgetGuid(WidgetBlueprint, ExistingPanel, bChanged);
			return ExistingPanel;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();

		UWidget* ExistingRoot = WidgetBlueprint->WidgetTree->RootWidget;
		UCanvasPanel* NewRoot = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		if (!NewRoot)
		{
			return nullptr;
		}

		NewRoot->SetDisplayLabel(TEXT("RootCanvas"));
		EnsureWidgetGuid(WidgetBlueprint, NewRoot, bChanged);
		WidgetBlueprint->WidgetTree->RootWidget = NewRoot;
		if (ExistingRoot)
		{
			ExistingRoot->RemoveFromParent();
			NewRoot->AddChild(ExistingRoot);
			EnsureWidgetGuid(WidgetBlueprint, ExistingRoot, bChanged);
		}

		bChanged = true;
		return NewRoot;
	}

	void MarkAsVariable(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, bool& bChanged)
	{
		if (!WidgetBlueprint || !Widget)
		{
			return;
		}

		if (!Widget->bIsVariable)
		{
			Widget->Modify();
			Widget->bIsVariable = true;
			bChanged = true;
		}

		EnsureWidgetGuid(WidgetBlueprint, Widget, bChanged);
	}

	void EnsureAllWidgetGuids(UWidgetBlueprint* WidgetBlueprint, bool& bChanged)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		WidgetBlueprint->WidgetTree->ForEachWidget([WidgetBlueprint, &bChanged](UWidget* Widget)
		{
			EnsureWidgetGuid(WidgetBlueprint, Widget, bChanged);
		});
	}

	void AddWidgetToPanel(UPanelWidget* RootPanel, UWidget* Widget, int32 PlacementIndex)
	{
		if (!RootPanel || !Widget || Widget->GetParent())
		{
			return;
		}

		if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(RootPanel))
		{
			if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Widget))
			{
				const bool bProgressBar = Widget->IsA<UProgressBar>();
				CanvasSlot->SetAutoSize(!bProgressBar);
				CanvasSlot->SetPosition(FVector2D(32.0f, 32.0f + static_cast<float>(PlacementIndex) * 44.0f));
				CanvasSlot->SetSize(bProgressBar ? FVector2D(320.0f, 18.0f) : FVector2D(260.0f, 36.0f));
			}
			return;
		}

		RootPanel->AddChild(Widget);
	}

	template<typename WidgetType>
	bool EnsureNamedWidget(UWidgetBlueprint* WidgetBlueprint, UPanelWidget* RootPanel, FName WidgetName, int32& PlacementIndex, bool& bChanged)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !RootPanel || WidgetName.IsNone())
		{
			return false;
		}

		if (UWidget* ExistingWidget = WidgetBlueprint->WidgetTree->FindWidget(WidgetName))
		{
			if (!ExistingWidget->IsA<WidgetType>())
			{
				UE_LOG(LogReclaimAssetSetup, Error, TEXT("Widget %s exists but is %s, expected %s."),
					*WidgetName.ToString(),
					*ExistingWidget->GetClass()->GetName(),
					*WidgetType::StaticClass()->GetName());
				return false;
			}

			MarkAsVariable(WidgetBlueprint, ExistingWidget, bChanged);
			return true;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();

		WidgetType* CreatedWidget = WidgetBlueprint->WidgetTree->ConstructWidget<WidgetType>(WidgetType::StaticClass(), WidgetName);
		if (!CreatedWidget)
		{
			UE_LOG(LogReclaimAssetSetup, Error, TEXT("Could not create required widget %s."), *WidgetName.ToString());
			return false;
		}

		CreatedWidget->SetDisplayLabel(WidgetName.ToString());
		MarkAsVariable(WidgetBlueprint, CreatedWidget, bChanged);
		AddWidgetToPanel(RootPanel, CreatedWidget, PlacementIndex++);
		bChanged = true;
		return true;
	}

	bool EnsureTextBlock(UWidgetBlueprint* WidgetBlueprint, UPanelWidget* RootPanel, FName WidgetName, int32& PlacementIndex, bool& bChanged)
	{
		const bool bResult = EnsureNamedWidget<UTextBlock>(WidgetBlueprint, RootPanel, WidgetName, PlacementIndex, bChanged);
		if (bResult)
		{
			if (UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->FindWidget<UTextBlock>(WidgetName))
			{
				TextBlock->SetText(FText::FromName(WidgetName));
			}
		}
		return bResult;
	}

	bool EnsureButton(UWidgetBlueprint* WidgetBlueprint, UPanelWidget* RootPanel, FName WidgetName, int32& PlacementIndex, bool& bChanged)
	{
		const bool bResult = EnsureNamedWidget<UButton>(WidgetBlueprint, RootPanel, WidgetName, PlacementIndex, bChanged);
		if (bResult)
		{
			if (UButton* Button = WidgetBlueprint->WidgetTree->FindWidget<UButton>(WidgetName))
			{
				if (Button->GetChildrenCount() == 0)
				{
					const FName LabelName(*FString::Printf(TEXT("%s_Label"), *WidgetName.ToString()));
					UTextBlock* Label = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
					if (Label)
					{
						Label->SetText(FText::FromName(WidgetName));
						Label->SetDisplayLabel(LabelName.ToString());
						EnsureWidgetGuid(WidgetBlueprint, Label, bChanged);
						Button->SetContent(Label);
						bChanged = true;
					}
				}
			}
		}
		return bResult;
	}

	bool EnsureProgressBar(UWidgetBlueprint* WidgetBlueprint, UPanelWidget* RootPanel, FName WidgetName, int32& PlacementIndex, bool& bChanged)
	{
		const bool bResult = EnsureNamedWidget<UProgressBar>(WidgetBlueprint, RootPanel, WidgetName, PlacementIndex, bChanged);
		if (bResult)
		{
			if (UProgressBar* ProgressBar = WidgetBlueprint->WidgetTree->FindWidget<UProgressBar>(WidgetName))
			{
				ProgressBar->SetPercent(1.0f);
			}
		}
		return bResult;
	}
}
#endif

FGameplayTag UReclaimPythonAssetSetupLibrary::MakeGameplayTagForAssetSetup(FName TagName, bool bErrorIfNotFound)
{
	if (TagName.IsNone())
	{
		return FGameplayTag();
	}

	return FGameplayTag::RequestGameplayTag(TagName, bErrorIfNotFound);
}

FGameplayTagContainer UReclaimPythonAssetSetupLibrary::MakeGameplayTagContainerForAssetSetup(const TArray<FName>& TagNames, bool bErrorIfNotFound)
{
	FGameplayTagContainer Container;
	for (const FName& TagName : TagNames)
	{
		const FGameplayTag Tag = MakeGameplayTagForAssetSetup(TagName, bErrorIfNotFound);
		if (Tag.IsValid())
		{
			Container.AddTag(Tag);
		}
	}

	return Container;
}

bool UReclaimPythonAssetSetupLibrary::EnsureWidgetBlueprintControls(UObject* WidgetBlueprintAsset, const TArray<FName>& ButtonNames, const TArray<FName>& TextBlockNames, const TArray<FName>& ProgressBarNames)
{
#if WITH_EDITOR
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(WidgetBlueprintAsset);
	if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
	{
		UE_LOG(LogReclaimAssetSetup, Error, TEXT("EnsureWidgetBlueprintControls expected a WidgetBlueprint asset."));
		return false;
	}

	bool bChanged = false;
	UPanelWidget* RootPanel = EnsureRootPanel(WidgetBlueprint, bChanged);
	if (!RootPanel)
	{
		UE_LOG(LogReclaimAssetSetup, Error, TEXT("Could not ensure a root panel for %s."), *WidgetBlueprint->GetName());
		return false;
	}

	int32 PlacementIndex = RootPanel->GetChildrenCount();
	for (const FName& ButtonName : ButtonNames)
	{
		if (!EnsureButton(WidgetBlueprint, RootPanel, ButtonName, PlacementIndex, bChanged))
		{
			return false;
		}
	}

	for (const FName& TextBlockName : TextBlockNames)
	{
		if (!EnsureTextBlock(WidgetBlueprint, RootPanel, TextBlockName, PlacementIndex, bChanged))
		{
			return false;
		}
	}

	for (const FName& ProgressBarName : ProgressBarNames)
	{
		if (!EnsureProgressBar(WidgetBlueprint, RootPanel, ProgressBarName, PlacementIndex, bChanged))
		{
			return false;
		}
	}

	EnsureAllWidgetGuids(WidgetBlueprint, bChanged);

	if (bChanged)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
	}

	return true;
#else
	return false;
#endif
}
