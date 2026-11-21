#include "UI/GameMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "FirstActionPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

namespace GameMenuStyle
{
	const FLinearColor Normal(0.78f, 0.78f, 0.80f, 1.f);
	const FLinearColor Selected(1.f, 0.78f, 0.30f, 1.f);
	const FLinearColor TitleColor(1.f, 1.f, 1.f, 1.f);

	FSlateFontInfo Font(const char* Typeface, int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}
}

// ---------------------------------------------------------------- 버튼

void UGameMenuButton::Setup(UGameMenuWidget* InOwner, EGameMenuAction InAction, UTextBlock* InLabel)
{
	OwnerMenu = InOwner;
	Action = InAction;
	Label = InLabel;
	OnClicked.AddDynamic(this, &UGameMenuButton::HandleClicked);
}

void UGameMenuButton::HandleClicked()
{
	if (OwnerMenu)
	{
		OwnerMenu->NotifyAction(Action);
	}
}

// ---------------------------------------------------------------- 메뉴

void UGameMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// 버튼이 없는 화면(로딩)에서도 포커스를 받아 키 입력(일시정지 닫기)을 받는다
	SetIsFocusable(true);
}

TSharedRef<SWidget> UGameMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGameMenuWidget::BuildTree()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	Background->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	if (UOverlaySlot* BgSlot = Root->AddChildToOverlay(Background))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	if (UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Left);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
		ColumnSlot->SetPadding(FMargin(140.f, 0.f, 0.f, 0.f));
	}

	Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetFont(GameMenuStyle::Font("Bold", 64));
	Title->SetColorAndOpacity(GameMenuStyle::TitleColor);
	Column->AddChildToVerticalBox(Title);

	Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
	Subtitle->SetFont(GameMenuStyle::Font("Regular", 22));
	Subtitle->SetColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.f));
	if (UVerticalBoxSlot* SubSlot = Column->AddChildToVerticalBox(Subtitle))
	{
		SubSlot->SetPadding(FMargin(4.f, 4.f, 0.f, 36.f));
	}

	ControlsTable = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Controls"));
	if (UVerticalBoxSlot* TableSlot = Column->AddChildToVerticalBox(ControlsTable))
	{
		TableSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 24.f));
	}

	ButtonBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Buttons"));
	Column->AddChildToVerticalBox(ButtonBox);
}

void UGameMenuWidget::BuildControlsTable()
{
	ControlsTable->ClearChildren();
	AFirstActionPlayerController* PC = Cast<AFirstActionPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return;
	}

	const auto AddRow = [this](const FText& A, const FText& B, const FText& C, bool bHeader)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const FText Cells[] = { A, B, C };
		const float Widths[] = { 260.f, 220.f, 260.f };
		for (int32 i = 0; i < 3; ++i)
		{
			USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Cell->SetWidthOverride(Widths[i]);
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Text->SetText(Cells[i]);
			Text->SetFont(GameMenuStyle::Font(bHeader ? "Bold" : (i == 0 ? "Regular" : "Bold"), bHeader ? 16 : 20));
			Text->SetColorAndOpacity(bHeader ? FLinearColor(0.6f, 0.6f, 0.6f, 1.f) : (i == 0 ? GameMenuStyle::Normal : GameMenuStyle::Selected));
			Cell->AddChild(Text);
			Row->AddChildToHorizontalBox(Cell);
		}
		if (UVerticalBoxSlot* RowSlot = ControlsTable->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.f, bHeader ? 0.f : 5.f));
		}
	};

	AddRow(FText::GetEmpty(), NSLOCTEXT("GameMenu", "Gamepad", "GAMEPAD"), NSLOCTEXT("GameMenu", "Keyboard", "KEYBOARD / MOUSE"), true);
	for (const FControlBinding& Binding : PC->GetControlBindings())
	{
		AddRow(Binding.Action, Binding.Gamepad, Binding.Keyboard, false);
	}
}

void UGameMenuWidget::AddButton(const FText& Text, EGameMenuAction Action)
{
	UGameMenuButton* Button = WidgetTree->ConstructWidget<UGameMenuButton>(UGameMenuButton::StaticClass());
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(Text);
	Label->SetFont(GameMenuStyle::Font("Bold", 30));
	Label->SetColorAndOpacity(GameMenuStyle::Normal);
	Button->AddChild(Label);

	// 배경 없는 글자 버튼. 고른 버튼은 글자 색으로 표시한다(NativeTick)
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.TintColor = FSlateColor(FLinearColor::Transparent);
	Style.Hovered.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.06f));
	Style.Pressed.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.12f));
	Style.NormalPadding = FMargin(6.f, 4.f);
	Style.PressedPadding = FMargin(6.f, 4.f);
	Button->SetStyle(Style);
	Button->Setup(this, Action, Label);

	if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(0.f, 6.f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Left);
	}
	Buttons.Add(Button);
}

void UGameMenuWidget::SetMode(EGameMenuMode NewMode)
{
	TakeWidget();   // 트리를 먼저 만든다
	Mode = NewMode;
	LoadingTime = 0.f;
	ButtonBox->ClearChildren();
	Buttons.Reset();
	ControlsTable->ClearChildren();
	ControlsTable->SetVisibility(Mode == EGameMenuMode::Controls ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	float Dim = 0.5f;
	switch (Mode)
	{
	case EGameMenuMode::Loading:
		Dim = 1.f;
		Title->SetText(NSLOCTEXT("GameMenu", "Loading", "LOADING"));
		Subtitle->SetText(FText::GetEmpty());
		break;
	case EGameMenuMode::Main:
		Dim = 0.35f;
		Title->SetText(NSLOCTEXT("GameMenu", "GameTitle", "FIRST ACTION"));
		Subtitle->SetText(NSLOCTEXT("GameMenu", "MainSub", "Third-person melee combat"));
		AddButton(NSLOCTEXT("GameMenu", "Start", "START"), EGameMenuAction::Start);
		AddButton(NSLOCTEXT("GameMenu", "ControlsButton", "CONTROLS"), EGameMenuAction::ShowControls);
		AddButton(NSLOCTEXT("GameMenu", "Quit", "QUIT"), EGameMenuAction::Quit);
		break;
	case EGameMenuMode::Controls:
		Dim = 0.7f;
		Title->SetText(NSLOCTEXT("GameMenu", "ControlsTitle", "CONTROLS"));
		Subtitle->SetText(FText::GetEmpty());
		BuildControlsTable();
		AddButton(NSLOCTEXT("GameMenu", "Back", "BACK"), EGameMenuAction::Back);
		break;
	case EGameMenuMode::Pause:
		Dim = 0.6f;
		Title->SetText(NSLOCTEXT("GameMenu", "Paused", "PAUSED"));
		Subtitle->SetText(FText::GetEmpty());
		AddButton(NSLOCTEXT("GameMenu", "Resume", "RESUME"), EGameMenuAction::Resume);
		AddButton(NSLOCTEXT("GameMenu", "RetryStage", "RETRY STAGE"), EGameMenuAction::RetryStage);
		AddButton(NSLOCTEXT("GameMenu", "ControlsButton", "CONTROLS"), EGameMenuAction::ShowControls);
		AddButton(NSLOCTEXT("GameMenu", "MainMenu", "MAIN MENU"), EGameMenuAction::MainMenu);
		AddButton(NSLOCTEXT("GameMenu", "Quit", "QUIT"), EGameMenuAction::Quit);
		break;
	case EGameMenuMode::Dead:
		Dim = 0.55f;
		Title->SetText(NSLOCTEXT("GameMenu", "Dead", "YOU DIED"));
		Subtitle->SetText(FText::GetEmpty());
		AddButton(NSLOCTEXT("GameMenu", "Retry", "RETRY"), EGameMenuAction::RetryStage);
		AddButton(NSLOCTEXT("GameMenu", "Restart", "RESTART FROM BEGINNING"), EGameMenuAction::RestartFromBeginning);
		AddButton(NSLOCTEXT("GameMenu", "MainMenu", "MAIN MENU"), EGameMenuAction::MainMenu);
		break;
	case EGameMenuMode::Cleared:
		Dim = 0.55f;
		Title->SetText(NSLOCTEXT("GameMenu", "Cleared", "ALL STAGES CLEARED"));
		Subtitle->SetText(FText::GetEmpty());
		AddButton(NSLOCTEXT("GameMenu", "PlayAgain", "PLAY AGAIN"), EGameMenuAction::RestartFromBeginning);
		AddButton(NSLOCTEXT("GameMenu", "MainMenu", "MAIN MENU"), EGameMenuAction::MainMenu);
		AddButton(NSLOCTEXT("GameMenu", "Quit", "QUIT"), EGameMenuAction::Quit);
		break;
	default:
		break;
	}

	Background->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, Dim));
	SetVisibility(Mode == EGameMenuMode::Hidden ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UGameMenuWidget::FocusFirstButton()
{
	if (Buttons.Num() > 0 && GetOwningPlayer())
	{
		Buttons[0]->SetUserFocus(GetOwningPlayer());
		Buttons[0]->SetKeyboardFocus();
	}
	else
	{
		SetKeyboardFocus();
	}
}

void UGameMenuWidget::NotifyAction(EGameMenuAction Action)
{
	// 조작법 화면 오가기는 위젯 안에서
	if (Action == EGameMenuAction::ShowControls)
	{
		ModeBeforeControls = Mode;
		SetMode(EGameMenuMode::Controls);
		FocusFirstButton();
		return;
	}
	if (Action == EGameMenuAction::Back)
	{
		SetMode(ModeBeforeControls);
		FocusFirstButton();
		return;
	}
	OnAction.Broadcast(Action);
}

void UGameMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Mode == EGameMenuMode::Loading)
	{
		// 점 세 개가 차례로 늘어나는 로딩 표시
		LoadingTime += InDeltaTime;
		const int32 Dots = static_cast<int32>(LoadingTime * 2.f) % 4;
		Title->SetText(FText::FromString(FString(TEXT("LOADING")) + FString::ChrN(Dots, TEXT('.'))));
		return;
	}

	// 포커스(게임패드) 또는 마우스가 올라간 버튼을 강조한다
	bool bAnyFocused = false;
	for (UGameMenuButton* Button : Buttons)
	{
		bAnyFocused |= Button->HasKeyboardFocus();
	}
	for (UGameMenuButton* Button : Buttons)
	{
		const bool bSelected = bAnyFocused ? Button->HasKeyboardFocus() : Button->IsHovered();
		if (UTextBlock* Label = Button->GetLabel())
		{
			Label->SetColorAndOpacity(bSelected ? GameMenuStyle::Selected : GameMenuStyle::Normal);
		}
	}

	// 포커스를 잃으면(마우스 클릭으로 빈 곳 등) 게임패드로 다시 고를 수 있게 첫 버튼으로
	if (!bAnyFocused && Buttons.Num() > 0 && !HasAnyUserFocus() && !IsHovered())
	{
		FocusFirstButton();
	}
}

FReply UGameMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bBack = Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::P;
	if (bBack && Mode == EGameMenuMode::Pause)
	{
		NotifyAction(EGameMenuAction::Resume);
		return FReply::Handled();
	}
	if (bBack && Mode == EGameMenuMode::Controls)
	{
		NotifyAction(EGameMenuAction::Back);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
