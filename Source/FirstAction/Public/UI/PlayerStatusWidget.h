// 플레이어 체력·경직치 표시 (9-4 이후). 화면 왼쪽 위. 플레이 중에만 보인다(컨트롤러가 메뉴와 함께 켜고 끈다).
// 지금 조종 중인 폰의 ASC 속성을 매 프레임 읽는다. 맞은 직후 줄어든 만큼은 잠시 밝게 남았다가 따라 내려간다
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class FIRSTACTION_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UProgressBar> HealthTrail;   // 줄어든 만큼 잠시 남는 밝은 바

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UProgressBar> PoiseBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthText;

	float TrailPercent = 1.f;
	float TrailHold = 0.f;
	float LastHealthPercent = 1.f;
};
