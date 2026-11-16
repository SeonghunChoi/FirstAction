// 피격 반응 단계. 타격 해석 매니저, 캐릭터 데이터, GA_HitReact가 함께 쓴다. (PLAN 4.6, Step 6-7)
// 값이 클수록 큰 반응이다. 반응 중에 다시 맞으면 같거나 높은 단계만 반응을 처음부터 다시 시작한다(6-7 결정).

#pragma once

#include "CoreMinimal.h"
#include "HitReactionTypes.generated.h"

UENUM(BlueprintType)
enum class EHitReactionStage : uint8
{
	// 반응 없음 (충격 0)
	None,
	// 움찔: 공격 중이면 몽타주 없이 연출만, 아니면 짧은 반응 몽타주. 행동을 막지 않는다
	Flinch,
	// 경직: 공격을 끊고 State.Staggered 동안 행동 불가
	Stagger,
	// 넘어짐: 경직 + 누운 구간(State.Downed, 몽타주의 ANS_TagWindow)에는 반응 없음
	Knockdown,
	// 그로기: 경직치 0 (Event.Groggy). 몽타주가 끝나면 경직치를 채운다. 그 동안은 피해만
	Groggy
};
