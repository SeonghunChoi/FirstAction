#include "UI/ControlHintWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "FirstActionPlayerController.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UControlHintWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* Back = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Back"));
		Back->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Back->SetPadding(FMargin(16.f, 6.f));
		if (UOverlaySlot* BackSlot = Root->AddChildToOverlay(Back))
		{
			BackSlot->SetHorizontalAlignment(HAlign_Center);
			BackSlot->SetVerticalAlignment(VAlign_Bottom);
			BackSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
		}

		Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		Back->SetContent(Row);
	}
	return Super::RebuildWidget();
}

void UControlHintWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AFirstActionPlayerController* PC = Cast<AFirstActionPlayerController>(GetOwningPlayer());
	const int32 Device = PC && PC->IsUsingGamepad() ? 1 : 0;
	if (Device != ShownDevice)
	{
		Rebuild(Device == 1);
	}
}

void UControlHintWidget::Rebuild(bool bGamepad)
{
	AFirstActionPlayerController* PC = Cast<AFirstActionPlayerController>(GetOwningPlayer());
	if (!PC || !Row)
	{
		return;
	}
	ShownDevice = bGamepad ? 1 : 0;
	Row->ClearChildren();

	for (const FControlBinding& Binding : PC->GetControlBindings())
	{
		const FText& Key = bGamepad ? Binding.Gamepad : Binding.Keyboard;
		if (!Binding.bButton || Key.IsEmpty())
		{
			continue;
		}

		UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		KeyText->SetText(Key);
		KeyText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
		KeyText->SetColorAndOpacity(FLinearColor(1.f, 0.78f, 0.3f, 1.f));
		if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyText))
		{
			KeySlot->SetPadding(FMargin(14.f, 0.f, 5.f, 0.f));
			KeySlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ActionText->SetText(Binding.Action);
		ActionText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 15));
		ActionText->SetColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f, 1.f));
		if (UHorizontalBoxSlot* ActionSlot = Row->AddChildToHorizontalBox(ActionText))
		{
			ActionSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}
