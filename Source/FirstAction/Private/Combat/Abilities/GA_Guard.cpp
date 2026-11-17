#include "Combat/Abilities/GA_Guard.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Combat/HitResolveSubsystem.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"

UGA_Guard::UGA_Guard()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 가드 내리기(End)의 캔슬 구간에서 다시 누르면 처음(Start)부터 다시 올린다
	bRetriggerInstancedAbility = true;

	// 매니저가 이 태그와 정면 각도를 보고 결과를 Guarded로 바꾼다 (규칙 6-4)
	ActivationOwnedTags.AddTag(CombatTags::State_Guarding);

	ActivationBlockedTags.AddTag(CombatTags::State_Staggered);
	ActivationBlockedTags.AddTag(CombatTags::State_Dead);

	// 공격·회피·움찔·가드 중이면 그 몽타주의 캔슬 구간에서만 시작한다 (6-9 규칙, 6-10 결정: 가드 연타 방지)
	CancelWindowRequiredTags.AddTag(CombatTags::State_Attacking);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Dodging);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Flinching);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Guarding);
}

void UGA_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;
	GuardMontage = Data ? Data->GetGuardMontage() : nullptr;
	if (!GuardMontage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_Guard: 캐릭터 데이터에 Guard Montage가 없습니다"), *GetNameSafe(Character));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 가드 올림"), *GetNameSafe(Character));

	// Start부터 재생한다. Start → Loop → Loop… 이어짐은 몽타주 구간 설정이 정한다
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, GuardMontage, 1.f, StartSection);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Guard::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Guard::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Guard::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Guard::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// 패리 성공은 공격자가 보낸 자기 Event.HitReact의 결과로 안다 (매니저 규칙, 6-12)
	UAbilityTask_WaitGameplayEvent* WaitHitReact = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CombatTags::Event_HitReact, nullptr, false, true);
	WaitHitReact->EventReceived.AddDynamic(this, &UGA_Guard::OnHitReactEvent);
	WaitHitReact->ReadyForActivation();

	// 버퍼에서 늦게 시작되어 버튼이 이미 떼어져 있으면(살짝 누른 경우) 올렸다가 바로 내린다
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	bool bHeld = false;
	if (Character && Spec)
	{
		for (const FGameplayTag& InputTag : Spec->GetDynamicSpecSourceTags())
		{
			bHeld |= Character->IsInputTagHeld(InputTag);
		}
	}
	if (!bHeld)
	{
		LowerGuard();
	}
}

void UGA_Guard::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	LowerGuard();
}

void UGA_Guard::LowerGuard()
{
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (!IsActive() || !AnimInstance || !GuardMontage)
	{
		return;
	}

	const FName Current = AnimInstance->Montage_GetCurrentSection(GuardMontage);
	if (Current == StartSection || Current == ParrySection)
	{
		// 올리는 중이거나 튕겨 낸 자세: 그 구간(패리 구간, 반격 구간)을 자르지 않도록 끝까지 재생한 뒤 End로
		AnimInstance->Montage_SetNextSection(Current, EndSection, GuardMontage);
	}
	else if (Current == LoopSection)
	{
		AnimInstance->Montage_JumpToSection(EndSection, GuardMontage);
	}
	// End 중이면 이미 내리는 중

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 가드 내림 (현재 구간 %s)"), *GetNameSafe(GetAvatarActorFromActorInfo()), *Current.ToString());
}

void UGA_Guard::OnHitReactEvent(FGameplayEventData Payload)
{
	const FHitResolution* Resolution = UHitResolveSubsystem::FindHitResolution(Payload.TargetData);
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (!IsActive() || !Resolution || Resolution->Outcome != EHitOutcome::Parried || !AnimInstance || !GuardMontage)
	{
		return;
	}

	// 튕겨 낸 자세로. 그 구간의 Window.Cancel부터 공격·회피로 반격할 수 있다 (6-12 결정)
	AnimInstance->Montage_JumpToSection(ParrySection, GuardMontage);

	// 버튼을 이미 뗐다면 Parry 구간 다음은 End
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	const ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
	bool bHeld = false;
	if (Character && Spec)
	{
		for (const FGameplayTag& InputTag : Spec->GetDynamicSpecSourceTags())
		{
			bHeld |= Character->IsInputTagHeld(InputTag);
		}
	}
	if (!bHeld)
	{
		AnimInstance->Montage_SetNextSection(ParrySection, EndSection, GuardMontage);
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 패리 성공: %s 구간으로"), *GetNameSafe(Character), *ParrySection.ToString());
}

void UGA_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsActive())
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 가드 종료 (%s)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), bWasCancelled ? TEXT("끊김") : TEXT("완료"));
	}

	MontageTask = nullptr;
	GuardMontage = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Guard::OnMontageFinished()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Guard::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
