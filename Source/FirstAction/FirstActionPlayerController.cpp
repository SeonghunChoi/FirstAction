// Copyright Epic Games, Inc. All Rights Reserved.


#include "FirstActionPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "FirstAction.h"
#include "FirstActionGameMode.h"
#include "FirstActionCharacter.h"
#include "Combat/CombatTags.h"
#include "Combat/LockOnComponent.h"
#include "AbilitySystemComponent.h"
#include "Game/RouteGuide.h"
#include "InputAction.h"
#include "InputKeyEventArgs.h"
#include "UI/StageBannerWidget.h"
#include "UI/PlayerStatusWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace
{
	// 조작법 표기용 짧은 키 이름
	FString ShortKeyName(const FKey& Key)
	{
		static const TMap<FName, FString> Names = {
			{ EKeys::Gamepad_FaceButton_Bottom.GetFName(), TEXT("A") },
			{ EKeys::Gamepad_FaceButton_Right.GetFName(), TEXT("B") },
			{ EKeys::Gamepad_FaceButton_Left.GetFName(), TEXT("X") },
			{ EKeys::Gamepad_FaceButton_Top.GetFName(), TEXT("Y") },
			{ EKeys::Gamepad_RightShoulder.GetFName(), TEXT("RB") },
			{ EKeys::Gamepad_LeftShoulder.GetFName(), TEXT("LB") },
			{ EKeys::Gamepad_RightTrigger.GetFName(), TEXT("RT") },
			{ EKeys::Gamepad_LeftTrigger.GetFName(), TEXT("LT") },
			{ EKeys::Gamepad_RightTriggerAxis.GetFName(), TEXT("RT") },
			{ EKeys::Gamepad_LeftTriggerAxis.GetFName(), TEXT("LT") },
			{ EKeys::Gamepad_RightThumbstick.GetFName(), TEXT("R3") },
			{ EKeys::Gamepad_LeftThumbstick.GetFName(), TEXT("L3") },
			{ EKeys::Gamepad_Special_Right.GetFName(), TEXT("Menu") },
			{ EKeys::Gamepad_Special_Left.GetFName(), TEXT("View") },
			{ EKeys::Gamepad_Left2D.GetFName(), TEXT("Left Stick") },
			{ EKeys::Gamepad_Right2D.GetFName(), TEXT("Right Stick") },
			{ EKeys::LeftMouseButton.GetFName(), TEXT("LMB") },
			{ EKeys::RightMouseButton.GetFName(), TEXT("RMB") },
			{ EKeys::MiddleMouseButton.GetFName(), TEXT("MMB") },
			{ EKeys::Mouse2D.GetFName(), TEXT("Mouse") },
			{ EKeys::SpaceBar.GetFName(), TEXT("Space") },
			{ EKeys::LeftShift.GetFName(), TEXT("Shift") },
			{ EKeys::Escape.GetFName(), TEXT("Esc") },
		};
		if (const FString* Found = Names.Find(Key.GetFName()))
		{
			return *Found;
		}
		return Key.GetDisplayName(false).ToString();
	}

	// 같은 동작의 여러 키를 한 줄로. W A S D와 방향키는 묶는다
	FText JoinKeys(const TArray<FKey>& Keys)
	{
		TArray<FString> Parts;
		const auto Has = [&Keys](std::initializer_list<FKey> Set)
		{
			for (const FKey& K : Set)
			{
				if (!Keys.Contains(K))
				{
					return false;
				}
			}
			return true;
		};
		TArray<FKey> Rest = Keys;
		if (Has({ EKeys::W, EKeys::A, EKeys::S, EKeys::D }))
		{
			Parts.Add(TEXT("WASD"));
			Rest.RemoveAll([](const FKey& K) { return K == EKeys::W || K == EKeys::A || K == EKeys::S || K == EKeys::D; });
		}
		if (Has({ EKeys::Up, EKeys::Down, EKeys::Left, EKeys::Right }))
		{
			Parts.Add(TEXT("Arrows"));
			Rest.RemoveAll([](const FKey& K) { return K == EKeys::Up || K == EKeys::Down || K == EKeys::Left || K == EKeys::Right; });
		}
		for (const FKey& K : Rest)
		{
			Parts.AddUnique(ShortKeyName(K));
		}
		return FText::FromString(FString::Join(Parts, TEXT(" / ")));
	}
}

void AFirstActionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogFirstAction, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AFirstActionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// 일시정지 (게임 흐름). 메뉴가 떠 있는 동안은 UI 입력이라 여기 오지 않고, 메뉴 위젯이 닫기 키를 받는다
		if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (PauseAction)
			{
				EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AFirstActionPlayerController::HandlePausePressed);
			}
		}
	}
}

bool AFirstActionPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// ---------------------------------------------------------------- 게임 흐름 화면

void AFirstActionPlayerController::ShowMenu(EGameMenuMode Mode)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!MenuWidget)
	{
		MenuWidget = CreateWidget<UGameMenuWidget>(this, UGameMenuWidget::StaticClass());
		MenuWidget->OnAction.AddUObject(this, &AFirstActionPlayerController::HandleMenuAction);
		MenuWidget->AddToPlayerScreen(100);
	}
	MenuWidget->SetMode(Mode);

	// 플레이 중 조작 힌트 줄 (메뉴가 떠 있으면 숨긴다)
	if (!HintWidget)
	{
		HintWidget = CreateWidget<UControlHintWidget>(this, UControlHintWidget::StaticClass());
		HintWidget->AddToPlayerScreen(40);
	}
	HintWidget->SetVisibility(Mode == EGameMenuMode::Hidden ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	// 플레이어 체력·경직치 (플레이 중에만)
	if (!StatusWidget)
	{
		StatusWidget = CreateWidget<UPlayerStatusWidget>(this, UPlayerStatusWidget::StaticClass());
		StatusWidget->AddToPlayerScreen(30);
	}
	StatusWidget->SetVisibility(Mode == EGameMenuMode::Hidden ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (Mode == EGameMenuMode::Hidden)
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		return;
	}

	// 메뉴가 떠 있는 동안 게임은 입력을 받지 않는다(공격 등). 로딩 중에는 커서도 숨긴다
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(Mode != EGameMenuMode::Loading);
	MenuWidget->FocusFirstButton();
}

void AFirstActionPlayerController::ShowBanner(const FText& Text, float Duration)
{
	if (!IsLocalPlayerController())
	{
		return;
	}
	if (!BannerWidget)
	{
		BannerWidget = CreateWidget<UStageBannerWidget>(this, UStageBannerWidget::StaticClass());
		BannerWidget->AddToPlayerScreen(50);
	}
	BannerWidget->ShowBanner(Text, Duration);
}

void AFirstActionPlayerController::ShowRouteGuide(const FVector& Destination)
{
	if (!IsLocalPlayerController())
	{
		return;
	}
	if (!RouteGuide)
	{
		UClass* GuideClass = RouteGuideClass ? RouteGuideClass.Get() : ARouteGuide::StaticClass();
		FActorSpawnParameters Params;
		Params.Owner = this;
		RouteGuide = GetWorld()->SpawnActor<ARouteGuide>(GuideClass, FTransform::Identity, Params);
	}
	if (RouteGuide)
	{
		RouteGuide->ShowRoute(Destination);
	}
}

void AFirstActionPlayerController::HideRouteGuide()
{
	if (RouteGuide)
	{
		RouteGuide->HideRoute();
	}
}

void AFirstActionPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 타겟팅 표시용: 락온 대상, 아니면 지금 공격하면 향할 적 (스틱 방향·정면 원뿔, 캐릭터의 대상 고르기 그대로)
	AimTarget.Reset();
	LockedTarget.Reset();
	const AFirstActionCharacter* PlayerCharacter = Cast<AFirstActionCharacter>(GetPawn());
	const UAbilitySystemComponent* ASC = PlayerCharacter ? PlayerCharacter->GetAbilitySystemComponent() : nullptr;
	if (!ASC || ASC->HasMatchingGameplayTag(CombatTags::State_Dead))
	{
		return;
	}
	if (const ULockOnComponent* LockOn = PlayerCharacter->FindComponentByClass<ULockOnComponent>(); LockOn && LockOn->IsLockedOn())
	{
		LockedTarget = LockOn->GetLockTarget();
		return;
	}
	AimTarget = PlayerCharacter->FindAttackTarget(AimIndicatorDistance);
}

bool AFirstActionPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	// 패드 스틱의 아주 작은 흔들림(드리프트)은 장치 전환으로 보지 않는다
	const bool bAxis = Params.Key.IsAxis1D() || Params.Key.IsAxis2D();
	if (Params.IsGamepad())
	{
		if (!bAxis || FMath::Abs(Params.AmountDepressed) > 0.3f)
		{
			bUsingGamepad = true;
		}
	}
	else if (!bAxis || Params.Key == EKeys::Mouse2D)
	{
		bUsingGamepad = false;
	}
	return Super::InputKey(Params);
}

const TArray<FControlBinding>& AFirstActionPlayerController::GetControlBindings()
{
	if (bControlBindingsBuilt)
	{
		return ControlBindings;
	}
	bControlBindingsBuilt = true;

	// 등록한 입력 매핑에서 Action Description이 있는 동작만 모은다. 같은 설명(예: 카메라 = 패드 오른쪽 스틱 + 마우스)은 한 줄로
	TArray<UInputMappingContext*> Contexts = DefaultMappingContexts;
	Contexts.Append(MobileExcludedMappingContexts);

	struct FCollected { FText Action; TArray<FKey> Pad; TArray<FKey> Keys; bool bButton = true; };
	TArray<FCollected> Collected;
	for (const UInputMappingContext* Context : Contexts)
	{
		if (!Context)
		{
			continue;
		}
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			const UInputAction* Action = Mapping.Action;
			if (!Action || Action->ActionDescription.IsEmpty())
			{
				continue;
			}
			FCollected* Entry = Collected.FindByPredicate([Action](const FCollected& C) { return C.Action.EqualTo(Action->ActionDescription); });
			if (!Entry)
			{
				Entry = &Collected.AddDefaulted_GetRef();
				Entry->Action = Action->ActionDescription;
				Entry->bButton = Action->ValueType == EInputActionValueType::Boolean;
			}
			(Mapping.Key.IsGamepadKey() ? Entry->Pad : Entry->Keys).AddUnique(Mapping.Key);
		}
	}

	for (const FCollected& C : Collected)
	{
		FControlBinding& Binding = ControlBindings.AddDefaulted_GetRef();
		Binding.Action = C.Action;
		Binding.Gamepad = JoinKeys(C.Pad);
		Binding.Keyboard = JoinKeys(C.Keys);
		Binding.bButton = C.bButton;
	}
	return ControlBindings;
}

void AFirstActionPlayerController::HandlePausePressed()
{
	AFirstActionGameMode* GameMode = GetWorld()->GetAuthGameMode<AFirstActionGameMode>();
	if (!GameMode || GameMode->GetFlowState() != EGameFlowState::Playing || IsPaused())
	{
		return;
	}
	GameMode->SetGamePaused(true);
	ShowMenu(EGameMenuMode::Pause);
}

void AFirstActionPlayerController::HandleMenuAction(EGameMenuAction Action)
{
	AFirstActionGameMode* GameMode = GetWorld()->GetAuthGameMode<AFirstActionGameMode>();
	if (!GameMode)
	{
		return;
	}

	switch (Action)
	{
	case EGameMenuAction::Start:
		GameMode->StartGame();
		break;
	case EGameMenuAction::Resume:
		GameMode->SetGamePaused(false);
		ShowMenu(EGameMenuMode::Hidden);
		break;
	case EGameMenuAction::RetryStage:
		GameMode->RetryStage();
		break;
	case EGameMenuAction::RestartFromBeginning:
		GameMode->RestartFromBeginning();
		break;
	case EGameMenuAction::MainMenu:
		GameMode->ReturnToMainMenu();
		break;
	case EGameMenuAction::Quit:
		GameMode->QuitGame();
		break;
	default:
		break;   // 조작법 화면 오가기는 위젯이 처리
	}
}
