// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/GameMenuWidget.h"
#include "UI/ControlHintWidget.h"
#include "FirstActionPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UStageBannerWidget;
class ARouteGuide;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 *  게임 흐름(9-4 이후): 게임 모드가 정한 메뉴 화면(로딩, 시작, 일시정지, 사망, 클리어)과 구간 알림을 띄우고,
 *  메뉴에서 고른 동작을 게임 모드에 넘긴다. 일시정지 입력(IA_Pause)을 받는다
 */
UCLASS(abstract)
class AFirstActionPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 메뉴 화면 바꾸기. Hidden이면 게임 입력으로 돌아간다
	void ShowMenu(EGameMenuMode Mode);

	// 화면 위쪽에 잠깐 뜨는 알림 (구간 시작·클리어)
	void ShowBanner(const FText& Text, float Duration = 2.5f);

	// 다음 구간 길 안내 (9-4 이후, ARouteGuide)
	void ShowRouteGuide(const FVector& Destination);
	void HideRouteGuide();

	// 타겟팅 표시 (9-4 이후): 다음 공격이 향할 적(락온 중이 아니면)과 락온 대상. 매 프레임 PlayerTick에서 정한다
	AActor* GetAimTarget() const { return AimTarget.Get(); }
	AActor* GetLockedTarget() const { return LockedTarget.Get(); }

	// 조작법 (9-4 이후): 입력 매핑에서 만든 동작별 키. 마지막으로 쓴 장치가 게임패드인가
	const TArray<FControlBinding>& GetControlBindings();
	bool IsUsingGamepad() const { return bUsingGamepad; }

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	// 일시정지 메뉴 열기 (패드 Start, Esc, P)
	UPROPERTY(EditDefaultsOnly, Category = "Input|Game Flow")
	TObjectPtr<UInputAction> PauseAction;

	// 조준 표시를 찾는 거리 (몸 간격). 공격의 워핑 최대 거리와 비슷하게
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow|Targeting", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float AimIndicatorDistance = 500.f;

	// 길 안내 액터 클래스 (메시·머티리얼을 기본값에 둔 블루프린트 자식)
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	TSubclassOf<ARouteGuide> RouteGuideClass;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;

	// 마지막으로 쓴 장치를 기록한다 (조작법 힌트 표기)
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	void HandlePausePressed();
	void HandleMenuAction(EGameMenuAction Action);

	UPROPERTY()
	TObjectPtr<UGameMenuWidget> MenuWidget;

	UPROPERTY()
	TObjectPtr<UStageBannerWidget> BannerWidget;

	UPROPERTY()
	TObjectPtr<UControlHintWidget> HintWidget;

	UPROPERTY()
	TObjectPtr<class UPlayerStatusWidget> StatusWidget;

	UPROPERTY()
	TObjectPtr<ARouteGuide> RouteGuide;

	TWeakObjectPtr<AActor> AimTarget;
	TWeakObjectPtr<AActor> LockedTarget;
	TArray<FControlBinding> ControlBindings;
	bool bControlBindingsBuilt = false;
	bool bUsingGamepad = true;
};
