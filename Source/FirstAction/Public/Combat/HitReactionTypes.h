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

// 타격의 결과 종류. 매니저가 맞는 쪽 상태 태그를 보고 정한다 (규칙 6-4, Step 6-9)
// 가드·패리·저스트 회피(6-11~6-13)도 여기에 값을 더해 같은 자리에서 결과를 바꾼다
UENUM(BlueprintType)
enum class EHitOutcome : uint8
{
	// 일반 타격: 피해, 경직치, 밀림, 반응 모두 적용
	Hit,
	// 무적(State.Invulnerable) 중이라 아무 결과도 없음. 때린 쪽도 헛친 것처럼 히트스톱·이동·연출이 없다
	Evaded,
	// 가드(State.Guarding, 정면 각도 안): 체력 피해 0, 경직치·밀림·히트스톱은 일반 타격과 같음, 반응 없음 (6-11)
	Guarded,
	// 패리(Window.Parry, 정면 각도 안): 방어자는 피해·경직치·밀림·반응 없음, 히트스톱은 길게.
	// 충격이 공격자에게 되돌아가 공격자가 AttackBone 부위 반응과 최소 경직으로 튕긴다 (6-12, 규칙 6-5)
	Parried
};
