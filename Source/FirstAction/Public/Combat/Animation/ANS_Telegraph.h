// 공격 예비 동작 구간 (9-4 이후). 이 구간 동안 공격하는 캐릭터의 시간 배율을 TimeScale로 낮추고, 시작할 때 번쩍임 Cue를 낸다.
// 칼을 치켜든 정점 부근에 짧고 강하게(예: 0.05초, 0.12배) 두면 "들고 멈칫했다가 내리침"으로 보인다. 타이밍은 애니메이션(규칙 3)
// 쓸지는 몸이 정한다: 캐릭터 데이터의 예비 동작 배율이 1이면(플레이어) 무시한다. 공유 몽타주에 두어도 플레이어는 느려지지 않는다.
// 콤보로 이어진 타는 캐릭터 데이터의 TelegraphComboStrength만큼만 적용한다. 몽타주에 이 구간이 없으면 GA_MeleeAttack이
// 몽타주 시작~첫 판정 구간 전체를 캐릭터 데이터 배율로 늦춘다(예전 방식)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_Telegraph.generated.h"

UCLASS(meta = (DisplayName = "Telegraph"))
class FIRSTACTION_API UANS_Telegraph : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 구간 동안 시간 배율 (낮을수록 오래 머문다)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float TimeScale = 0.15f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
