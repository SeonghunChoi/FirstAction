// 전투에서 쓰는 네이티브 게임플레이 태그를 한 곳에 모은다. (CLAUDE.md 설계 규칙 10)
// 새 태그는 여기와 CombatTags.cpp에 함께 추가하고, 문자열 태그를 코드에 직접 쓰지 않는다.

#pragma once

#include "NativeGameplayTags.h"

namespace CombatTags
{
	// 입력: 플레이어 입력과 AI 요청이 어빌리티를 찾는 키 (Step 2)
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_LightAttack);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_HeavyAttack);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Dodge);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Guard);

	// 상태: 지금 무엇을 하고 있는가. 어빌리티가 실행 중에 붙인다
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dodging);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guarding);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Staggered);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Downed);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	// 구간: 애니메이션 노티파이(ANS_TagWindow)가 열고 닫는다 (Step 4, 6)
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_Combo);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_Cancel);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_Parry);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_JustDodge);

	// 이벤트: 게임플레이 이벤트로 어빌리티에 전달된다 (Step 5, 6)
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);

	// SetByCaller: GE_Damage에 실행 시점 값을 넘기는 키 (Step 5)
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PoiseDamage);

	// GameplayCue: 연출(카메라 흔들림, 불꽃, 타격음) 전용 통로. 게임 코드는 태그만 발생시키고 내용은 GCN 에셋이 가진다 (Step 6-1)
	// GameplayCue 태그는 반드시 "GameplayCue."으로 시작해야 한다
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Dealt);
	FIRSTACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Taken);
}
