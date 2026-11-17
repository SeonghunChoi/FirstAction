#include "Combat/Abilities/GA_Dodge.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"

UGA_Dodge::UGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 회피 중 다시 회피(연속 구르기): 캔슬 구간에서 다시 누르면 지금 회피를 끝내고 처음부터 (6-9 결정)
	bRetriggerInstancedAbility = true;

	ActivationOwnedTags.AddTag(CombatTags::State_Dodging);

	// 경직(넘어짐, 그로기 포함) 중에는 회피할 수 없다
	ActivationBlockedTags.AddTag(CombatTags::State_Staggered);
	ActivationBlockedTags.AddTag(CombatTags::State_Dead);

	// 공격·회피·움찔 중이면 그 몽타주의 캔슬 구간에서만 시작한다
	CancelWindowRequiredTags.AddTag(CombatTags::State_Attacking);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Dodging);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Flinching);
}

void UGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	UAnimMontage* Montage = Data ? Data->GetDodgeMontage() : nullptr;
	if (!Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_Dodge: 캐릭터 데이터에 Dodge Montage가 없습니다"), *GetNameSafe(Character));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 회피 방향으로 몸을 즉시 돌린다. 몽타주는 앞구르기 하나라 몸이 향한 쪽으로 구른다 (6-9 결정)
	const FVector Direction = Character->GetDodgeDirection().GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		Character->SetActorRotation(Direction.Rotation());
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 회피 시작: %s, 방향 %s"),
		*GetNameSafe(Character), *GetNameSafe(Montage), *Direction.ToCompactString());

	// 같은 슬롯의 공격·움찔 몽타주는 여기서 끊기고, 그 어빌리티는 자기 몽타주 취소 콜백으로 끝난다
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Dodge::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Dodge::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsActive())
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 회피 종료 (%s)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), bWasCancelled ? TEXT("끊김") : TEXT("완료"));
	}

	MontageTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Dodge::OnMontageFinished()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Dodge::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
