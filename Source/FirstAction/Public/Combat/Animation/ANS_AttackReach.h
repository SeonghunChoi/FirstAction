// 휘두름 하나의 "멈출 간격"을 몽타주에 적는 데이터 전용 노티파이 스테이트. (6-8에서 발견한 문제의 수정)
// 칼이 닿는 거리는 휘두름마다 다르고, 그것을 아는 곳은 애니메이션이다(결정: 휘두름마다 애니메이션에, 별도 노티파이로).
//
// 사용법: 공격 몽타주의 Motion Warping 구간이 시작되는 시점을 덮도록 이 노티파이를 둔다.
// GA_MeleeAttack이 워핑 구간이 시작될 때 같은 시점의 Attack Reach를 찾아, 대상과 몸 간격 StopGap에서 멈추게 워핑 목표를 정한다.
// 없으면 지금처럼 몸이 닿는 거리(간격 0)까지 붙는다. 실행 중에는 아무것도 하지 않는다(NotifyBegin/End 없음).

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_AttackReach.generated.h"

UCLASS(meta = (DisplayName = "Attack Reach"))
class FIRSTACTION_API UANS_AttackReach : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 노티파이 트랙에 간격이 보이게 한다
	virtual FString GetNotifyName_Implementation() const override;

	// 워핑이 멈출 대상과의 몸 간격 (cm, 캡슐 표면끼리). 이미 이보다 가까우면 물러나지 않고 제자리에서 휘두른다
	// 몽타주에 놓을 때마다 따로 정하므로 EditAnywhere (ANS_TagWindow와 같음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack Reach", meta = (ClampMin = "0.0", Units = "cm"))
	float StopGap = 0.f;
};
