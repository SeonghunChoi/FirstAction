#include "UI/EnemyStatusWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/CombatAttributeSet.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatTags.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FirstActionPlayerController.h"

namespace
{
	UProgressBar* MakeBar(UWidgetTree* Tree, const FLinearColor& Fill, float Height, UVerticalBox* Parent, float TopPadding)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(110.f);
		Box->SetHeightOverride(Height);
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Style.FillImage.TintColor = FSlateColor(FLinearColor::White);
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetPercent(1.f);
		Box->AddChild(Bar);
		if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Box))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Center);
			BoxSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
		return Bar;
	}
}

TSharedRef<SWidget> UEnemyStatusWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 마름모 표시: 정사각형 이미지를 45도 돌린다
		Marker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Marker"));
		Marker->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
		Marker->SetRenderTransformAngle(45.f);
		Marker->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		if (UVerticalBoxSlot* MarkerSlot = Root->AddChildToVerticalBox(Marker))
		{
			MarkerSlot->SetHorizontalAlignment(HAlign_Center);
			MarkerSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		HealthBar = MakeBar(WidgetTree, FLinearColor(0.85f, 0.12f, 0.1f, 1.f), 7.f, Root, 0.f);
		PoiseBar = MakeBar(WidgetTree, FLinearColor(1.f, 0.78f, 0.2f, 1.f), 3.f, Root, 2.f);
	}
	return Super::RebuildWidget();
}

void UEnemyStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ACombatCharacterBase* Owner = Character.Get();
	const UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!ASC || ASC->HasMatchingGameplayTag(CombatTags::State_Dead))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);

	const auto Ratio = [ASC](const FGameplayAttribute& Value, const FGameplayAttribute& Max)
	{
		const float MaxValue = ASC->GetNumericAttribute(Max);
		return MaxValue > 0.f ? FMath::Clamp(ASC->GetNumericAttribute(Value) / MaxValue, 0.f, 1.f) : 0.f;
	};
	HealthBar->SetPercent(Ratio(UCombatAttributeSet::GetHealthAttribute(), UCombatAttributeSet::GetMaxHealthAttribute()));
	PoiseBar->SetPercent(Ratio(UCombatAttributeSet::GetPoiseAttribute(), UCombatAttributeSet::GetMaxPoiseAttribute()));

	// 조준·락온 표시
	Time += InDeltaTime;
	const AFirstActionPlayerController* PC = Cast<AFirstActionPlayerController>(GetOwningPlayer());
	const bool bLocked = PC && PC->GetLockedTarget() == Owner;
	const bool bAimed = !bLocked && PC && PC->GetAimTarget() == Owner;
	if (bLocked || bAimed)
	{
		Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
		const float Size = bLocked ? 20.f : 12.f;
		Marker->SetDesiredSizeOverride(FVector2D(Size, Size));
		Marker->SetColorAndOpacity(bLocked ? FLinearColor(1.f, 0.45f, 0.1f, 1.f) : FLinearColor(1.f, 1.f, 1.f, 0.9f));
		// 위아래로 살짝 흔들려 눈에 띄게
		Marker->SetRenderTranslation(FVector2D(0.f, FMath::Sin(Time * 6.f) * (bLocked ? 3.f : 2.f)));
	}
	else
	{
		Marker->SetVisibility(ESlateVisibility::Hidden);
	}
}
