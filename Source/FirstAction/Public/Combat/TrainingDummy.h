// 훈련용 더미. 플레이어·적과 같은 기반 클래스라 같은 판정, 피해, (Step 6) 질량과 반응을 그대로 받는다. (Step 5-5)
// 무기는 들지 않고(WeaponMesh 비움), 움직이지 않는다.
// 체력이 0이 되면 RefillDelay 뒤 시작 이펙트를 다시 적용해 채운다 → 같은 실험을 반복할 수 있다.
// 경직치는 그로기 규칙(GA_HitReact가 그로기 끝에 회복, 6-7)이 채우므로 더미가 따로 보지 않는다.

// 템플릿 Combat 변형에 같은 이름의 ACombatDummy(CombatDummy.h)가 있어, 헤더·클래스 이름이 겹치지 않도록 TrainingDummy로 짓는다

#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatCharacterBase.h"
#include "TrainingDummy.generated.h"

struct FOnAttributeChangeData;

// 훈련 더미의 시험용 방어 (6-12). 적 AI(Step 7) 전에 가드·패리 규칙을 플레이어 공격으로 시험하기 위한 것
// 더미는 AI처럼 입력 태그로 어빌리티 활성화만 요청한다(규칙 1). 정면 각도 판정이 있으므로 더미를 플레이어 쪽으로 돌려 배치한다
UENUM()
enum class ETrainingDummyDefense : uint8
{
	// 방어하지 않음
	None,
	// 가드를 계속 올리고 있음. 가드가 깨지면(그로기) 다시 올린다 → 막는 판정(6-11) 시험
	HoldGuard,
	// 일정 간격으로 가드를 올렸다 바로 내림. 올리는 순간(Window.Parry)에 맞으면 플레이어 공격이 패리된다 → 패리(6-12) 시험
	RepeatGuard,
	// 일정 간격으로 정면으로 구름. 구르기 앞부분(Window.JustDodge)에 맞으면 저스트 회피, 무적 뒷부분이면 일반 회피 → 6-9, 6-13 시험
	RepeatDodge
};

UCLASS()
class FIRSTACTION_API ATrainingDummy : public ACombatCharacterBase
{
	GENERATED_BODY()

public:
	ATrainingDummy();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 체력이 0이 된 뒤 다시 채우기까지의 시간 (튜닝값, 블루프린트 기본값에서 정한다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Dummy", meta = (ClampMin = "0.0", Units = "s"))
	float RefillDelay = 0.f;

	// 시험용 방어 방식. 레벨에 놓은 더미마다 정한다 (규칙 9: 배치별 초기값)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Combat|Dummy")
	ETrainingDummyDefense DefenseMode = ETrainingDummyDefense::None;

	// 방어 동작 간격 (초). HoldGuard는 가드가 깨졌는지 확인하는 간격, RepeatGuard는 가드를 다시 올리는 간격
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Combat|Dummy", meta = (ClampMin = "0.1", Units = "s"))
	float DefenseInterval = 1.5f;

private:
	void OnHealthChanged(const FOnAttributeChangeData& ChangeData);
	void Refill();

	// DefenseMode에 따라 가드 입력을 요청한다
	void TickDefense();

	FTimerHandle RefillTimer;
	FTimerHandle DefenseTimer;
	FDelegateHandle HealthChangedHandle;
};
