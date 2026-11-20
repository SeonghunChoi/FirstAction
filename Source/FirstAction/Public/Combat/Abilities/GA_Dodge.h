// 회피 어빌리티. 플레이어와 적이 같은 클래스를 쓴다. (PLAN 4.4, Step 6-9)
// 회피 방향(캐릭터가 정함: 스틱, 없으면 정면)으로 몸을 즉시 돌리고, 캐릭터 데이터의 회피 몽타주(앞구르기 루트 모션)를 재생한다.
//
// 타이밍은 몽타주가 정한다 (규칙 3):
//   State.Invulnerable 구간 — 무적. 매니저가 결과를 "회피됨"으로 바꿔 피해·반응이 없고 때린 쪽도 헛친다 (6-9 결정)
//   Window.Cancel 구간      — 이 뒤로 공격이나 다시 회피로 끊을 수 있다
// 시작 조건 (6-9 결정):
//   경직(State.Staggered: 경직, 넘어짐, 그로기) 중에는 막는다
//   공격·회피·움찔 중이면 그 몽타주의 캔슬 구간(Window.Cancel)에서만 시작한다 (UCombatGameplayAbility)
//   캔슬 구간 밖에서 누른 입력은 캐릭터의 입력 버퍼에 남았다가 구간이 열리는 순간 다시 시도된다

#pragma once

#include "CoreMinimal.h"
#include "Combat/Abilities/CombatGameplayAbility.h"
#include "GA_Dodge.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class FIRSTACTION_API UGA_Dodge : public UCombatGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dodge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	// 루트 모션 없는 회피 몽타주의 코드 이동 (9-3): 몽타주의 Motion Warping 구간 동안 Direction으로 Distance만큼
	void ScheduleMove(const class UAnimMontage* Montage, const FVector& Direction, float Distance);
	FTimerHandle MoveTimer;
};
