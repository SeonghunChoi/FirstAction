#include "Combat/Abilities/GA_Death.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"

UGA_Death::UGA_Death()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData DeathTrigger;
	DeathTrigger.TriggerTag = CombatTags::Event_Death;
	DeathTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(DeathTrigger);

	// 두 번 죽지 않는다
	ActivationBlockedTags.AddTag(CombatTags::State_Dead);
}

void UGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
	if (!ASC || !Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 사망 표시. 어빌리티가 끝나도 남도록 Loose 태그로 붙인다
	ASC->AddLooseGameplayTag(CombatTags::State_Dead);

	// 2. 진행 중인 공격·반응·가드 등을 모두 끝내고, 몸을 멈춘다
	ASC->CancelAbilities(nullptr, nullptr, this);
	Character->HandleDeathStarted();

	// 3. 사망 몽타주 → 래그돌 (7-1 결정)
	const UCombatCharacterData* Data = Character->GetCharacterData();
	UAnimMontage* Montage = Data ? Data->GetDeathMontage() : nullptr;
	if (!Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_Death: 캐릭터 데이터에 Death Montage가 없어 바로 래그돌로"), *GetNameSafe(Character));
		FinishWithRagdoll();
		return;
	}

	// 끝나도 몽타주를 멈추지 않는다(래그돌로 넘어가는 순간까지 마지막 자세 유지)
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, 1.f, NAME_None, false);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Death::OnMontageDone);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Death::OnMontageDone);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Death::OnMontageDone);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Death::OnMontageDone);
	MontageTask->ReadyForActivation();
}

void UGA_Death::OnMontageDone()
{
	FinishWithRagdoll();
}

void UGA_Death::FinishWithRagdoll()
{
	if (!IsActive())
	{
		return;
	}
	if (ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Character->StartRagdoll();
	}
	MontageTask = nullptr;
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
