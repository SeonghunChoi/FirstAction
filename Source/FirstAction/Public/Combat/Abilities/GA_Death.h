// 사망 어빌리티. 플레이어와 적이 같은 클래스를 쓴다. (Step 7-1)
// Event.Death(체력이 0이 되는 순간, 속성 세트가 보냄)로 발동한다. 6-7 그로기(Event.Groggy → GA_HitReact)와 같은 구조.
//
// 순서 (7-1 결정):
//   1. State.Dead를 붙인다(되돌리지 않음). 판정·공격 대상·락온·반응이 이 태그로 사망을 안다
//   2. 다른 어빌리티를 모두 취소하고, 캐릭터가 이동·충돌을 끄고 사망을 알린다(HandleDeathStarted)
//   3. 캐릭터 데이터의 사망 몽타주를 재생하고, 끝나면 래그돌로
// 제거(적만, 일정 시간 뒤)는 캐릭터가 사망 알림에서 정한다. 훈련 더미에는 부여하지 않는다(자동 리필 유지)

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Death.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class FIRSTACTION_API UGA_Death : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Death();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	// 사망 몽타주가 끝났거나 끊김 → 래그돌로 넘어가고 끝난다
	UFUNCTION()
	void OnMontageDone();

	void FinishWithRagdoll();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
};
