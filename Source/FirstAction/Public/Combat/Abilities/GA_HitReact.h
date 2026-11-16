// 피격 반응 어빌리티. 플레이어와 적이 같은 클래스를 쓴다. (PLAN 4.6, Step 6-7)
// Event.HitReact(타격 해석 결과)와 Event.Groggy(경직치 0)로 발동한다(Ability Trigger). 입력으로는 발동하지 않는다.
//
// 맡는 것: 반응 몽타주와 상태 태그(State.Staggered, State.Groggy). 그로기가 끝날 때 경직치 회복
// 맡지 않는 것: 히트스톱과 밀림. 반응이 막혀도(가드, 그로기, 누움) 몸은 밀리므로 캐릭터가 적용한다(6-7 결정)
//
// 반응 규칙 (6-7 결정):
//   - 단계는 매니저가 상대 충격량으로 정한 값(FHitResolution::ReactionStage)을 그대로 쓴다. 질량이 버티는 정도를 정한다
//   - 공격 중 움찔: 몽타주 없이 연출만(공격을 끊지 않음). 경직 이상: 반응 몽타주가 공격 몽타주를 끊는다
//   - 반응 중 다시 맞음: 같거나 높은 단계면 처음부터 다시, 낮으면 지금 반응 유지
//   - 누운 구간(State.Downed)과 그로기 중에는 반응 없음(피해와 밀림만)
//     단 누운 구간에 경직치가 0이 되면 그로기는 들어간다(그로기 이벤트는 한 번뿐이라 막으면 경직치가 회복되지 않음)

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Combat/HitReactionTypes.h"
#include "GA_HitReact.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;

UCLASS()
class FIRSTACTION_API UGA_HitReact : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_HitReact();

protected:
	// 이벤트가 올 때마다 발동 전에 불린다(실행 중이면 실행 중인 인스턴스에서). 반응할지 여기서 정한다
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayEventData* Payload) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// 그로기가 끝날 때 적용해 경직치를 최대치로 채우는 이펙트 (규칙 5: 속성은 GE로만)
	// 블루프린트 자식(BP_GA_HitReact)의 기본값에서 지정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Groggy")
	TSubclassOf<UGameplayEffect> PoiseRestoreEffect;

private:
	// 이벤트에서 반응 단계를 꺼낸다: Event.Groggy → 그로기, Event.HitReact → 해석 결과의 단계
	static EHitReactionStage GetStageFromEvent(const FGameplayEventData* Payload);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageCancelled();

	// 지금 재생 중인 반응 단계. 다시 맞았을 때 같거나 높은 단계인지 비교한다
	UPROPERTY(VisibleInstanceOnly, Category = "Hit React")
	EHitReactionStage CurrentStage = EHitReactionStage::None;

	// 이번 반응이 붙인 상태 태그. 끝날 때 붙인 만큼 뗀다
	FGameplayTagContainer AddedStateTags;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
};
