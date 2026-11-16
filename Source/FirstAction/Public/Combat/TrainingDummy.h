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

private:
	void OnHealthChanged(const FOnAttributeChangeData& ChangeData);
	void Refill();

	FTimerHandle RefillTimer;
	FDelegateHandle HealthChangedHandle;
};
