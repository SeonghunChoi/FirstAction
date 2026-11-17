// 플레이어와 적의 행동 어빌리티(공격, 회피, 이후 가드)가 함께 쓰는 기반 클래스. (Step 6-9)
// 규칙 하나를 더한다: "다른 행동 중이면 그 행동의 캔슬 구간(Window.Cancel)에서만 시작할 수 있다" (6-9 결정)
//
// GAS의 ActivationBlockedTags는 "이 태그가 있으면 항상 막음"만 표현한다.
// CancelWindowRequiredTags는 "이 태그가 있으면 Window.Cancel도 있어야 시작"을 표현한다.
// 타이밍은 몽타주의 ANS_TagWindow(Window.Cancel)가 정하고(규칙 3), 이 클래스는 태그만 본다.
// 시작한 행동의 몽타주가 같은 슬롯의 이전 몽타주를 끊고, 이전 어빌리티는 자기 몽타주 취소 콜백으로 끝난다.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CombatGameplayAbility.generated.h"

UCLASS(Abstract)
class FIRSTACTION_API UCombatGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	// 이 태그 중 하나라도 있으면 Window.Cancel이 함께 있을 때만 시작한다
	// (예: 회피는 공격·회피·움찔 중이면 그 몽타주의 캔슬 구간에서만)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer CancelWindowRequiredTags;
};
