#include "UI/PlayerStatusWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/CombatAttributeSet.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"

namespace
{
	UProgressBar* MakePlayerBar(UWidgetTree* Tree, const FLinearColor& Fill, float BackgroundAlpha)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, BackgroundAlpha));
		Style.FillImage.TintColor = FSlateColor(FLinearColor::White);
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetPercent(1.f);
		return Bar;
	}
}

TSharedRef<SWidget> UPlayerStatusWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		if (UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column))
		{
			ColumnSlot->SetHorizontalAlignment(HAlign_Left);
			ColumnSlot->SetVerticalAlignment(VAlign_Top);
			ColumnSlot->SetPadding(FMargin(40.f, 36.f, 0.f, 0.f));
		}

		HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
		HealthText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		HealthText->SetColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f, 1.f));
		HealthText->SetShadowOffset(FVector2D(1.f, 1.f));
		if (UVerticalBoxSlot* TextSlot = Column->AddChildToVerticalBox(HealthText))
		{
			TextSlot->SetPadding(FMargin(2.f, 0.f, 0.f, 4.f));
		}

		// 체력: 뒤에 밝은 잔상 바, 앞에 빨간 바를 겹친다
		USizeBox* HealthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		HealthBox->SetWidthOverride(340.f);
		HealthBox->SetHeightOverride(14.f);
		UOverlay* HealthStack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		HealthTrail = MakePlayerBar(WidgetTree, FLinearColor(1.f, 0.85f, 0.6f, 0.9f), 0.55f);
		HealthBar = MakePlayerBar(WidgetTree, FLinearColor(0.82f, 0.1f, 0.08f, 1.f), 0.f);
		for (UProgressBar* Bar : { HealthTrail.Get(), HealthBar.Get() })
		{
			if (UOverlaySlot* BarSlot = HealthStack->AddChildToOverlay(Bar))
			{
				BarSlot->SetHorizontalAlignment(HAlign_Fill);
				BarSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		HealthBox->AddChild(HealthStack);
		Column->AddChildToVerticalBox(HealthBox);

		USizeBox* PoiseBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PoiseBox->SetWidthOverride(340.f);
		PoiseBox->SetHeightOverride(5.f);
		PoiseBar = MakePlayerBar(WidgetTree, FLinearColor(1.f, 0.78f, 0.2f, 1.f), 0.55f);
		PoiseBox->AddChild(PoiseBar);
		if (UVerticalBoxSlot* PoiseSlot = Column->AddChildToVerticalBox(PoiseBox))
		{
			PoiseSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		}
	}
	return Super::RebuildWidget();
}

void UPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(GetOwningPlayerPawn());
	const UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(UCombatAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UCombatAttributeSet::GetMaxHealthAttribute());
	const float Poise = ASC->GetNumericAttribute(UCombatAttributeSet::GetPoiseAttribute());
	const float MaxPoise = ASC->GetNumericAttribute(UCombatAttributeSet::GetMaxPoiseAttribute());
	const float HealthPercent = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;

	HealthBar->SetPercent(HealthPercent);
	PoiseBar->SetPercent(MaxPoise > 0.f ? FMath::Clamp(Poise / MaxPoise, 0.f, 1.f) : 0.f);
	HealthText->SetText(FText::FromString(FString::Printf(TEXT("HP  %d / %d"), FMath::CeilToInt(Health), FMath::RoundToInt(MaxHealth))));

	// 잔상: 줄면 0.5초 머물렀다가 따라 내려간다. 늘면(다시 하기 등) 바로 맞춘다
	if (HealthPercent < LastHealthPercent)
	{
		TrailHold = 0.5f;
	}
	LastHealthPercent = HealthPercent;
	if (HealthPercent >= TrailPercent)
	{
		TrailPercent = HealthPercent;
	}
	else if ((TrailHold -= InDeltaTime) <= 0.f)
	{
		TrailPercent = FMath::FInterpConstantTo(TrailPercent, HealthPercent, InDeltaTime, 0.6f);
	}
	HealthTrail->SetPercent(TrailPercent);
}
