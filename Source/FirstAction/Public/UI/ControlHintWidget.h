// 조작법 (9-4 이후)
// FControlBinding: 입력 매핑에서 만든 "동작 - 게임패드 키 - 키보드/마우스 키" 한 줄. 동작 이름은 입력 액션의 Action Description.
// UControlHintWidget: 플레이 중 화면 아래 한 줄 힌트 ([X] Light Attack ...). 마지막으로 쓴 장치(패드/키보드)에 맞춰 표기가 바뀐다
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControlHintWidget.generated.h"

class UHorizontalBox;

struct FControlBinding
{
	FText Action;
	FText Gamepad;
	FText Keyboard;
	bool bButton = true;   // 버튼 동작(힌트 줄에 보인다). 이동·카메라 같은 축은 조작법 화면에만
};

UCLASS()
class FIRSTACTION_API UControlHintWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Rebuild(bool bGamepad);

	UPROPERTY()
	TObjectPtr<UHorizontalBox> Row;

	int32 ShownDevice = -1;   // -1 아직 없음, 0 키보드, 1 패드
};
