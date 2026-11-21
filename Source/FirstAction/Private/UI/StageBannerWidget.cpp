#include "UI/StageBannerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UStageBannerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
		Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 44));
		Label->SetShadowOffset(FVector2D(2.f, 2.f));
		Label->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		Label->SetJustification(ETextJustify::Center);
		if (UOverlaySlot* LabelSlot = Root->AddChildToOverlay(Label))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Top);
			LabelSlot->SetPadding(FMargin(0.f, 120.f, 0.f, 0.f));
		}
	}
	return Super::RebuildWidget();
}

void UStageBannerWidget::ShowBanner(const FText& Text, float Duration)
{
	TakeWidget();
	Label->SetText(Text);
	Total = FMath::Max(Duration, 0.1f);
	Remaining = Total;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UStageBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Remaining <= 0.f)
	{
		return;
	}
	Remaining -= InDeltaTime;

	// 처음 0.3초 나타나고 마지막 0.6초 사라진다
	const float Elapsed = Total - Remaining;
	const float Alpha = FMath::Clamp(FMath::Min(Elapsed / 0.3f, Remaining / 0.6f), 0.f, 1.f);
	Label->SetColorAndOpacity(FLinearColor(1.f, 0.85f, 0.45f, Alpha));
	if (Remaining <= 0.f)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
