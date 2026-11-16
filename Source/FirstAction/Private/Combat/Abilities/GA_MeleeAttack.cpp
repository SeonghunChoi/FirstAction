#include "Combat/Abilities/GA_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "GameplayEffect.h"
#include "Animation/AnimMontage.h"
#include "Combat/AttackDataAsset.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 실행 중에는 State.Attacking을 붙인다 (어빌리티가 끝나면 자동으로 떨어진다)
	// 콤보는 같은 실행 안에서 이어지므로 타와 타 사이에도 이 태그가 빠지지 않는다
	ActivationOwnedTags.AddTag(CombatTags::State_Attacking);

	// 이미 공격 중이면 다른 공격 스펙(약공격 중 강공격 버튼 등)이 따로 시작되지 않게 막는다
	// 공격 중 입력은 입력 버퍼를 거쳐 이 어빌리티의 콤보 분기로 들어온다
	ActivationBlockedTags.AddTag(CombatTags::State_Attacking);
}

const UAttackDataAsset* UGA_MeleeAttack::ResolveStartAttack() const
{
	return Cast<UAttackDataAsset>(GetCurrentSourceObject());
}

ACombatCharacterBase* UGA_MeleeAttack::GetCombatCharacter() const
{
	return Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
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

	UE_LOG(LogFirstAction, Log, TEXT("[%s] GA_MeleeAttack 시작: %s"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack));

	PlayCurrentAttackMontage();

	// 콤보 구간이 열리는 순간을 기다린다. 콤보가 이어지면 다음 몽타주의 구간에서도 다시 불린다(OnlyTriggerOnce = false)
	UAbilityTask_WaitGameplayTagAdded* WaitComboWindow =
		UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, CombatTags::Window_Combo, nullptr, false);
	WaitComboWindow->Added.AddDynamic(this, &UGA_MeleeAttack::OnComboWindowOpened);
	WaitComboWindow->ReadyForActivation();

	// 판정이 대상을 맞히면 캐릭터가 Event.Hit을 보낸다. 콤보가 이어지는 동안 계속 받는다
	UAbilityTask_WaitGameplayEvent* WaitHit =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CombatTags::Event_Hit, nullptr, false, true);
	WaitHit->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnHitEvent);
	WaitHit->ReadyForActivation();

	// 구간 안에서 누른 입력도 바로 처리하도록 버퍼 기록을 구독한다
	if (ACombatCharacterBase* Character = GetCombatCharacter())
	{
		InputBufferedHandle = Character->OnInputBuffered.AddUObject(this, &UGA_MeleeAttack::OnInputBuffered);
	}
}

void UGA_MeleeAttack::PlayCurrentAttackMontage()
{
	// 콤보로 몽타주를 바꾸면 이전 몽타주는 "끊김"으로 끝난다.
	// 그 콜백이 어빌리티를 끝내지 않도록 먼저 떼어 내고 이전 태스크를 정리한다
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveAll(this);
		MontageTask->OnBlendOut.RemoveAll(this);
		MontageTask->OnInterrupted.RemoveAll(this);
		MontageTask->OnCancelled.RemoveAll(this);
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	if (!CurrentAttack || !CurrentAttack->Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_MeleeAttack: %s에 몽타주가 없습니다."),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack));
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	// 몽타주를 재생하고 끝날 때까지 기다리는 어빌리티 태스크 (코루틴의 yield 지점에 해당)
	// 어빌리티가 먼저 끝나면 몽타주도 함께 멈춘다(bStopWhenAbilityEnds 기본값 true)
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentAttack->Montage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_MeleeAttack::TryAdvanceCombo()
{
	if (!IsActive() || !CurrentAttack || CurrentAttack->ComboLinks.IsEmpty())
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	ACombatCharacterBase* Character = GetCombatCharacter();
	if (!ASC || !Character || !ASC->HasMatchingGameplayTag(CombatTags::Window_Combo))
	{
		return;
	}

	// 이 공격의 ComboLinks에 있는 입력만 받는다 (회피 입력 등은 버퍼에 남는다)
	FGameplayTagContainer AcceptedInputTags;
	for (const FAttackComboLink& Link : CurrentAttack->ComboLinks)
	{
		if (Link.InputTag.IsValid() && Link.NextAttack)
		{
			AcceptedInputTags.AddTag(Link.InputTag);
		}
	}

	FGameplayTag PressedTag;
	if (!Character->ConsumeBufferedInput(AcceptedInputTags, PressedTag))
	{
		return;
	}

	const FAttackComboLink* ChosenLink = CurrentAttack->ComboLinks.FindByPredicate(
		[&PressedTag](const FAttackComboLink& Link) { return Link.InputTag == PressedTag; });
	if (!ChosenLink || !ChosenLink->NextAttack)
	{
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 콤보: %s --%s--> %s"),
		*GetNameSafe(Character), *GetNameSafe(CurrentAttack), *PressedTag.ToString(), *GetNameSafe(ChosenLink->NextAttack));

	CurrentAttack = ChosenLink->NextAttack;
	PlayCurrentAttackMontage();
}

void UGA_MeleeAttack::OnHitEvent(FGameplayEventData Payload)
{
	if (!IsActive() || !CurrentAttack)
	{
		return;
	}

	AActor* Target = const_cast<AActor*>(Payload.Target.Get());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC)
	{
		// ASC가 없는 대상(벽에 붙은 장식 Pawn 등)은 피해를 받지 않는다
		return;
	}

	if (!DamageEffect)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_MeleeAttack: DamageEffect가 지정되지 않았습니다. BP_GA_MeleeAttack 기본값을 확인하세요."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 피해량은 공격 데이터에서. GE_Damage는 Health에 이 값을 더하므로 음수로 넘긴다
	SpecHandle.Data->SetSetByCallerMagnitude(CombatTags::SetByCaller_Damage, -CurrentAttack->Damage);

	// 판정 결과(맞은 위치, 본)를 이펙트 컨텍스트에 함께 싣는다. 피격 이펙트, 반응에서 꺼내 쓸 수 있다
	if (const FHitResult* Hit = Payload.TargetData.Get(0) ? Payload.TargetData.Get(0)->GetHitResult() : nullptr)
	{
		SpecHandle.Data->GetContext().AddHitResult(*Hit, true);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 피해: %s → %s (%.1f)"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack), *GetNameSafe(Target), CurrentAttack->Damage);
}

void UGA_MeleeAttack::OnComboWindowOpened()
{
	TryAdvanceCombo();
}

void UGA_MeleeAttack::OnInputBuffered(const FGameplayTag& InputTag)
{
	TryAdvanceCombo();
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

	if (InputBufferedHandle.IsValid())
	{
		if (ACombatCharacterBase* Character = GetCombatCharacter())
		{
			Character->OnInputBuffered.Remove(InputBufferedHandle);
		}
		InputBufferedHandle.Reset();
	}

	CurrentAttack = nullptr;
	MontageTask = nullptr;
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
