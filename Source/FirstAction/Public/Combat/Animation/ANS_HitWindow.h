// 몽타주의 한 구간 동안 소유 액터의 무기 판정(UMeleeTraceComponent)을 켜는 노티파이 스테이트. (PLAN 4.4, 4.5)
// 칼날이 실제로 휘둘러지는 구간에 놓는다. 구간 하나가 "한 번의 휘두름"이다.
// 연속 베기 애니메이션은 휘두를 때마다 구간을 따로 놓는다(구간끼리 겹치지 않게).

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_HitWindow.generated.h"

UCLASS(meta = (DisplayName = "Hit Window"))
class FIRSTACTION_API UANS_HitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
