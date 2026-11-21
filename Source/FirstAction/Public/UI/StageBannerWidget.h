// 구간 시작·클리어 알림 (게임 흐름, 9-4 이후)
// 화면 위쪽에 "STAGE 2 / 4", "STAGE CLEAR" 같은 글자를 잠깐 띄우고 사라진다. 입력은 받지 않는다.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageBannerWidget.generated.h"

class UTextBlock;

UCLASS()
class FIRSTACTION_API UStageBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowBanner(const FText& Text, float Duration = 2.5f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> Label;

	float Remaining = 0.f;
	float Total = 0.f;
};
