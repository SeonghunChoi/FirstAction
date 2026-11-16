// 모든 근접 공격을 실행하는 범용 어빌리티. (PLAN 4.1~4.4, 설계 규칙 2, 4)
// 특정 공격을 모른다. 시작 공격 데이터(UAttackDataAsset)를 받아 그 몽타주를 재생하고,
// Window.Combo 구간에서 입력 버퍼를 꺼내 ComboLinks에 따라 같은 실행 안에서 다음 공격으로 넘어간다(4-4 결정).
// 플레이어 약공격·강공격과 적의 모든 공격이 이 클래스 하나를 쓴다.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_MeleeAttack.generated.h"

class UAttackDataAsset;
class UAbilityTask_PlayMontageAndWait;
class ACombatCharacterBase;
class UGameplayEffect;
class UMotionWarpingComponent;
struct FGameplayEventData;

UCLASS()
class FIRSTACTION_API UGA_MeleeAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MeleeAttack();

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

	// 맞은 대상에게 적용할 피해 이펙트 (Step 5-3)
	// C++에서 에셋 경로를 하드코딩하지 않도록 블루프린트 자식(BP_GA_MeleeAttack)의 기본값에서 지정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// 공격 몽타주의 Motion Warping 구간이 쓰는 목표 이름 (Step 6-2, CLAUDE.md 5.1 이름 규약)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Warping")
	FName WarpTargetName = TEXT("AttackTarget");

private:
	// 판정이 대상을 맞힘 (Event.Hit) → 피해 이펙트 적용, 히트스톱, 맞은 쪽에 Event.HitReact, 때린 쪽 Cue (Step 6-1)
	// 타격 해석 매니저(UHitResolveSubsystem)가 결과를 계산하고, 이 어빌리티는 때린 쪽 몫만 적용한다(설계 규칙 6, Step 6-4)
	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	// 시작 공격 데이터를 정한다. 지금은 부여 건의 데이터(SourceObject)만 본다.
	// AI용 이벤트 페이로드 경로는 Step 7에서 여기에 추가한다
	const UAttackDataAsset* ResolveStartAttack() const;

	ACombatCharacterBase* GetCombatCharacter() const;

	// 캐릭터에 공격 대상을 묻고 워핑 목표를 등록한다 (Step 6-2)
	// 목표: 대상 쪽 방향, 몸끼리 닿는 위치(양쪽 캡슐 반지름 합)까지, 단 MaxWarpDistance를 넘지 않음
	// 대상이 없으면 목표를 지워 원래 루트 모션대로 휘두른다
	void UpdateWarpTarget();

	// Motion Warping 구간이 시작될 때마다 대상을 다시 묻는다 (6-6에서 발견한 문제의 수정)
	// 엔진이 새 구간의 수정자(modifier)를 만든 직후, 활성화하기 직전에 불린다. 한 몽타주에 여러 번 휘두르는 공격(Heavy_Combo)도 휘두름마다 새 목표를 쓴다
	UFUNCTION()
	void OnMotionWarpingPreUpdate(UMotionWarpingComponent* MotionWarpingComp);

	// 이미 목표를 갱신해 준 구간의 수정자. 같은 구간에서 매 프레임 다시 묻지 않도록
	// (헤더에서 수정자 클래스 정의를 몰라도 되도록 UObject로 보관)
	TSet<TWeakObjectPtr<UObject>> RefreshedWarpModifiers;

	// CurrentAttack의 몽타주를 재생한다. 이전 몽타주 태스크가 있으면 콜백을 떼고 정리한 뒤 교체한다
	void PlayCurrentAttackMontage();

	// Window.Combo가 열려 있고 버퍼에 ComboLinks의 입력이 있으면 다음 공격으로 넘어간다
	void TryAdvanceCombo();

	// Window.Combo가 붙는 순간 (구간 전에 미리 눌러 둔 입력 처리)
	UFUNCTION()
	void OnComboWindowOpened();

	// 입력이 버퍼에 기록되는 순간 (구간 안에서 누른 입력 처리)
	void OnInputBuffered(const FGameplayTag& InputTag);

	// 몽타주가 끝까지 재생되었거나 블렌드 아웃을 시작함 → 정상 종료
	UFUNCTION()
	void OnMontageFinished();

	// 다른 몽타주에 끊겼거나 어빌리티가 취소됨 → 취소 종료
	UFUNCTION()
	void OnMontageCancelled();

	// 지금 실행 중인 공격. 콤보가 이어지면 다음 공격 데이터로 바뀐다
	UPROPERTY(VisibleInstanceOnly, Category = "Attack")
	TObjectPtr<const UAttackDataAsset> CurrentAttack;

	// 지금 몽타주를 기다리는 태스크. 콤보로 몽타주를 바꿀 때 이전 것을 정리하기 위해 들고 있는다
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	FDelegateHandle InputBufferedHandle;
};
