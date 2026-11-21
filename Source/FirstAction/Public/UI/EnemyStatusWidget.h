// 적 머리 위 상태 표시 (9-4 이후 타겟팅 UI)
// 체력·경직치 바와 조준 표시. 다음 공격이 향할 적(조준)은 작은 흰 마름모, 락온 대상은 큰 주황 마름모.
// AEnemyCharacter의 위젯 컴포넌트(화면 공간)가 띄운다. 조준·락온 대상은 플레이어 컨트롤러가 매 프레임 정한 값을 읽기만 한다
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyStatusWidget.generated.h"

class UProgressBar;
class UImage;
class ACombatCharacterBase;

UCLASS()
class FIRSTACTION_API UEnemyStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCharacter(ACombatCharacterBase* InCharacter) { Character = InCharacter; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UImage> Marker;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UProgressBar> PoiseBar;

	TWeakObjectPtr<ACombatCharacterBase> Character;
	float Time = 0.f;
};
