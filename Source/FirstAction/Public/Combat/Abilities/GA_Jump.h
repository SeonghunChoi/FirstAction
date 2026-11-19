// 점프 어빌리티 (Step 8-1 중 추가). 템플릿은 점프 입력을 ACharacter::Jump에 바로 묶어 전투 상태를 보지 않았다.
// 이제 점프도 입력 태그(InputTag.Jump)로 들어와 다른 행동과 같은 시작 규칙을 따른다 (설계 규칙 1, 2026-11-19 결정)
//
// 시작 조건 (회피와 같다):
//   경직(State.Staggered: 경직, 넘어짐, 그로기)·사망 중에는 막는다
//   공격·회피·움찔·가드 중이면 그 몽타주의 캔슬 구간(Window.Cancel)에서만 시작한다 (UCombatGameplayAbility)
//   이미 공중(State.InAir)이면 막는다 (이단 점프 없음)
// 시작하면 재생 중인 행동 몽타주를 멈춘다. 그 어빌리티는 자기 몽타주 취소 콜백으로 끝난다
// 착지하면 끝난다. 버튼을 떼면 StopJumping (JumpMaxHoldTime이 0보다 크면 누른 시간만큼 높이가 달라진다)
//
// 공중 규칙(공중에서는 공격·회피·가드 불가, 2026-11-19 결정)은 이 어빌리티가 아니라 캐릭터가 이동 모드로 붙이는
// State.InAir 태그가 맡는다. 점프가 아니라 걸어서 떨어질 때도 같은 규칙이 적용된다

#pragma once

#include "CoreMinimal.h"
#include "Combat/Abilities/CombatGameplayAbility.h"
#include "GA_Jump.generated.h"

UCLASS()
class FIRSTACTION_API UGA_Jump : public UCombatGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Jump();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);

	// 점프를 요청했는데 땅을 떠나지 못했으면(천장, 이동 불가 등) 끝낸다. 착지 알림이 오지 않아 어빌리티가 남는 것을 막는다
	void CheckLeftGround();

	FTimerHandle LeftGroundCheckTimer;
};
