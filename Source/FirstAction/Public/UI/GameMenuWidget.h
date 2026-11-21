// 게임 메뉴 화면 (게임 흐름, 9-4 이후)
// 로딩, 시작 메뉴, 일시정지, 사망, 클리어를 한 위젯이 모드로 바꿔 보여 준다.
// 위젯 블루프린트 없이 C++에서 위젯 트리를 만든다. 깃 버전과 배포판이 같은 화면을 쓰고, 에셋 편집 없이 고칠 수 있다.
// 버튼은 게임패드(십자키·스틱 이동, A 선택)와 마우스 모두로 고른다. 고른 결과는 컨트롤러로 넘긴다.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GameMenuWidget.generated.h"

class UTextBlock;
class UImage;
class UVerticalBox;
class UGameMenuWidget;

UENUM(BlueprintType)
enum class EGameMenuMode : uint8
{
	Hidden,
	Loading,
	Main,
	Pause,
	Dead,
	Cleared,
	Controls    // 조작법 표 (시작·일시정지 메뉴에서 열고 BACK으로 돌아간다)
};

UENUM(BlueprintType)
enum class EGameMenuAction : uint8
{
	Start,              // 시작 메뉴: 게임 시작
	Resume,             // 일시정지 풀기
	RetryStage,         // 지금 구간 입구에서 다시
	RestartFromBeginning,
	MainMenu,
	Quit,
	ShowControls,       // 위젯 안에서 처리 (조작법 화면)
	Back                // 위젯 안에서 처리 (조작법 화면에서 돌아가기)
};

// 메뉴 버튼 하나. 눌리면 자기 동작을 위젯에 알린다
UCLASS()
class FIRSTACTION_API UGameMenuButton : public UButton
{
	GENERATED_BODY()

public:
	void Setup(UGameMenuWidget* InOwner, EGameMenuAction InAction, UTextBlock* InLabel);
	UTextBlock* GetLabel() const { return Label; }

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<UGameMenuWidget> OwnerMenu;

	UPROPERTY()
	TObjectPtr<UTextBlock> Label;

	EGameMenuAction Action = EGameMenuAction::Start;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameMenuAction, EGameMenuAction /*Action*/);

UCLASS()
class FIRSTACTION_API UGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMode(EGameMenuMode NewMode);
	EGameMenuMode GetMode() const { return Mode; }

	// 첫 버튼에 포커스 (게임패드 조작 시작점)
	void FocusFirstButton();

	void NotifyAction(EGameMenuAction Action);
	FOnGameMenuAction OnAction;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildTree();
	void AddButton(const FText& Text, EGameMenuAction Action);
	void BuildControlsTable();

	UPROPERTY()
	TObjectPtr<UImage> Background;

	UPROPERTY()
	TObjectPtr<UTextBlock> Title;

	UPROPERTY()
	TObjectPtr<UTextBlock> Subtitle;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ButtonBox;

	UPROPERTY()
	TArray<TObjectPtr<UGameMenuButton>> Buttons;

	// 조작법 표 (조작법 화면에서만 보인다)
	UPROPERTY()
	TObjectPtr<UVerticalBox> ControlsTable;

	EGameMenuMode Mode = EGameMenuMode::Hidden;
	EGameMenuMode ModeBeforeControls = EGameMenuMode::Main;
	float LoadingTime = 0.f;
};
