// 가드 어빌리티. 플레이어와 적이 같은 클래스를 쓴다. (PLAN 4.6 방어 규칙, Step 6-11)
// 버튼을 누르고 있는 동안 가드를 유지한다. 막았을 때의 결과는 매니저 규칙(규칙 6-4): 이 어빌리티는 State.Guarding을 붙이기만 한다.
//
// 몽타주(캐릭터 데이터의 GuardMontage) 구간 세 개 (6-10, 6-11 결정):
//   Start — 가드를 올림. 앞부분 Window.Parry(6-12에서 사용). 버튼을 일찍 떼도 Start는 끝까지 재생하고 End로 간다
//   Loop  — 유지. 몽타주에서 자기 자신으로 이어지게 반복 설정
//   End   — 내림. 뒷부분 Window.Cancel. 그 전에는 다시 가드를 올릴 수 없다(연타 패리 방지, 6-10 결정)
// 가드 중에는 이동하지 않고 몸만 돈다(캐릭터의 AddMovementInput, 6-11 결정)
// 가드 중 공격·회피는 가드 몽타주의 캔슬 구간에서만 (UCombatGameplayAbility, 6-11 결정)

#pragma once

#include "CoreMinimal.h"
#include "Combat/Abilities/CombatGameplayAbility.h"
#include "GA_Guard.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

UCLASS()
class FIRSTACTION_API UGA_Guard : public UCombatGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Guard();

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

	// 버튼을 뗌 → 가드를 내린다
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// 가드 몽타주의 구간 이름 (CLAUDE.md 5.1 이름 규약)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	FName StartSection = TEXT("Start");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	FName LoopSection = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	FName EndSection = TEXT("End");

	// 패리 성공 시 넘어가는 구간 (6-12 결정). 이 구간의 Window.Cancel부터 반격할 수 있다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	FName ParrySection = TEXT("Parry");

private:
	// Start 중이면 Start가 끝난 뒤 End로(패리 구간을 자르지 않음), Loop 중이면 바로 End로
	void LowerGuard();

	// 자기 Event.HitReact: 결과가 패리면 Parry 구간으로 넘어간다 (6-12)
	UFUNCTION()
	void OnHitReactEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAnimMontage> GuardMontage;
};
