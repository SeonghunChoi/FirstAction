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

private:
	// 시작 공격 데이터를 정한다. 지금은 부여 건의 데이터(SourceObject)만 본다.
	// AI용 이벤트 페이로드 경로는 Step 7에서 여기에 추가한다
	const UAttackDataAsset* ResolveStartAttack() const;

	ACombatCharacterBase* GetCombatCharacter() const;

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
