// 데이터 검증(IsDataValid, Step 8-3)이 함께 쓰는 몽타주 검사 함수. 에디터 전용
// 공격 데이터, 캐릭터 데이터, 패턴 데이터가 "몽타주에 이 노티파이가 있는가"를 같은 방법으로 본다
//
// 등급 (8-3 결정 B): 동작이 분명히 깨지는 것은 오류, 일부러 그랬을 수 있는 것은 경고
// 노티파이는 몽타주 자체의 노티파이 트랙만 본다. 이 프로젝트는 구간 노티파이를 시퀀스가 아니라 몽타주에 둔다

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

#if WITH_EDITOR

class UAnimSequenceBase;
class UAnimNotifyState;

namespace CombatDataValidation
{
	// 이 클래스(또는 자식)의 노티파이 스테이트가 하나라도 있는가
	FIRSTACTION_API bool HasNotifyState(const UAnimSequenceBase* Animation, TSubclassOf<UAnimNotifyState> NotifyStateClass);

	// 이 태그를 붙이는 ANS_TagWindow 구간이 하나라도 있는가
	FIRSTACTION_API bool HasTagWindow(const UAnimSequenceBase* Animation, const FGameplayTag& Tag);

	// Motion Warping 구간들의 워핑 목표 이름. 워핑 구간이 없으면 비어 있다
	FIRSTACTION_API void GetWarpTargetNames(const UAnimSequenceBase* Animation, TArray<FName>& OutNames);

	// 검증 메시지 앞에 붙일 에셋 이름 (예: "DA_Attack_Light_01: ")
	FIRSTACTION_API FString Prefix(const UObject* Asset);
}

#endif
