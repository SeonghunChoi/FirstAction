#include "Combat/Abilities/GA_MeleeAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Combat/AttackDataAsset.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 실행 중에는 State.Attacking을 붙인다 (어빌리티가 끝나면 자동으로 떨어진다)
	ActivationOwnedTags.AddTag(CombatTags::State_Attacking);

	// 이미 공격 중이면 다른 공격 스펙(약공격 중 강공격 버튼 등)이 따로 시작되지 않게 막는다
	// 공격 중 입력으로 다음 공격을 고르는 것은 Step 4 콤보에서 이 어빌리티 안에서 처리한다
	ActivationBlockedTags.AddTag(CombatTags::State_Attacking);
}

const UAttackDataAsset* UGA_MeleeAttack::ResolveStartAttack() const
{
	return Cast<UAttackDataAsset>(GetCurrentSourceObject());
}

void UGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	CurrentAttack = ResolveStartAttack();
	if (!CurrentAttack || !CurrentAttack->Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_MeleeAttack: 시작 공격 데이터 또는 몽타주가 없습니다. 부여 목록의 Ability Data를 확인하세요."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] GA_MeleeAttack 시작: %s (몽타주 %s)"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack), *GetNameSafe(CurrentAttack->Montage));

	// 몽타주를 재생하고 끝날 때까지 기다리는 어빌리티 태스크 (코루틴의 yield 지점에 해당)
	// 어빌리티가 먼저 끝나면 몽타주도 함께 멈춘다(bStopWhenAbilityEnds 기본값 true)
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentAttack->Montage);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_MeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsActive())
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] GA_MeleeAttack 종료: %s (%s)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack), bWasCancelled ? TEXT("취소") : TEXT("완료"));
	}

	CurrentAttack = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MeleeAttack::OnMontageFinished()
{
	// 블렌드 아웃과 완료가 둘 다 오지만, 두 번째 EndAbility는 이미 끝난 상태라 무시된다
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_MeleeAttack::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
