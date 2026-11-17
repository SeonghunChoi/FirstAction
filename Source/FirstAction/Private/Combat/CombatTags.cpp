#include "Combat/CombatTags.h"

namespace CombatTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_LightAttack, "InputTag.LightAttack", "약공격 입력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_HeavyAttack, "InputTag.HeavyAttack", "강공격 입력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dodge, "InputTag.Dodge", "회피 입력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Guard, "InputTag.Guard", "가드 입력 (누르고 있는 동안)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "공격 어빌리티 실행 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging, "State.Dodging", "회피 어빌리티 실행 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Guarding, "State.Guarding", "가드 중. 타격 해석에서 가드 규칙 적용");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "무적. 판정은 되지만 피해와 반응이 없음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Flinching, "State.Flinching", "움찔 반응 몽타주 재생 중. 행동은 막지 않고, 회피는 캔슬 구간에서만 끊는다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Staggered, "State.Staggered", "경직 중. 행동 불가, AI와 공격 토큰이 공격을 멈추는 기준");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Groggy, "State.Groggy", "경직치 0으로 그로기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Downed, "State.Downed", "넘어져 누운 구간. 넘어짐 몽타주의 ANS_TagWindow가 붙인다. 이 동안 피격 반응 없음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Window_Combo, "Window.Combo", "다음 콤보 입력을 소비하는 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Window_Cancel, "Window.Cancel", "회피 등으로 끊을 수 있는 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Window_Parry, "Window.Parry", "패리 성립 구간. 타격 해석에서 패리 규칙 적용");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Window_JustDodge, "Window.JustDodge", "저스트 회피 성립 구간. 타격 해석에서 저스트 회피 규칙 적용");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Hit, "Event.Hit", "판정 트레이스가 대상을 맞힘 → 공격 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_HitReact, "Event.HitReact", "피해를 받음 → 피격 반응 어빌리티 발동");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Groggy, "Event.Groggy", "경직치가 0이 됨");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage", "GE_Damage 피해량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_PoiseDamage, "SetByCaller.PoiseDamage", "GE_Damage 경직치 감소량");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Dealt, "GameplayCue.Hit.Dealt", "내가 대상을 맞힘 (때린 쪽 연출: 카메라 흔들림)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Taken, "GameplayCue.Hit.Taken", "내가 맞음 (맞은 쪽 연출: 불꽃, 타격음, 카메라 흔들림)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit_Guarded, "GameplayCue.Hit.Guarded", "가드로 막음: 막은 쪽 연출(금속 불꽃, 막는 소리). 공격 어빌리티가 결과에 따라 맞은 쪽에 발생 (6-11)");
}
