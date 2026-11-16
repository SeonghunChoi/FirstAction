#include "Combat/Abilities/GA_HitReact.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatTags.h"
#include "Combat/HitResolveSubsystem.h"
#include "FirstAction.h"

namespace
{
	FString StageName(EHitReactionStage Stage)
	{
		return UEnum::GetDisplayValueAsText(Stage).ToString();
	}
}

UGA_HitReact::UGA_HitReact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 반응 중에 다시 발동되면 지금 실행을 끝내고 처음부터 다시 시작한다.
	// 낮은 단계는 ShouldAbilityRespondToEvent에서 걸러지므로, 여기까지 오는 것은 같거나 높은 단계뿐이다
	bRetriggerInstancedAbility = true;

	// 게임플레이 이벤트로 발동한다. 이벤트를 보낸 쪽(공격 어빌리티, 속성 세트)은 이 어빌리티를 모른다
	FAbilityTriggerData HitTrigger;
	HitTrigger.TriggerTag = CombatTags::Event_HitReact;
	HitTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(HitTrigger);

	FAbilityTriggerData GroggyTrigger;
	GroggyTrigger.TriggerTag = CombatTags::Event_Groggy;
	GroggyTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(GroggyTrigger);

	// 이 상태에서는 반응하지 않는다: 그로기 중(피해만), 사망
	// 누운 구간(State.Downed)은 그로기만 통과시켜야 해서 ShouldAbilityRespondToEvent에서 따로 거른다
	ActivationBlockedTags.AddTag(CombatTags::State_Groggy);
	ActivationBlockedTags.AddTag(CombatTags::State_Dead);
}

EHitReactionStage UGA_HitReact::GetStageFromEvent(const FGameplayEventData* Payload)
{
	if (!Payload)
	{
		return EHitReactionStage::None;
	}
	if (Payload->EventTag == CombatTags::Event_Groggy)
	{
		return EHitReactionStage::Groggy;
	}
	if (const FHitResolution* Resolution = UHitResolveSubsystem::FindHitResolution(Payload->TargetData))
	{
		return Resolution->ReactionStage;
	}
	return EHitReactionStage::None;
}

bool UGA_HitReact::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	if (!Super::ShouldAbilityRespondToEvent(ActorInfo, Payload))
	{
		return false;
	}

	const EHitReactionStage Stage = GetStageFromEvent(Payload);
	if (Stage == EHitReactionStage::None)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FString OwnerName = GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

	// 막는 상태는 ActivationBlockedTags가 다시 거르지만, 로그로 이유를 보이기 위해 여기서 먼저 본다
	if (ASC && ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags))
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 반응 없음: %s 맞음, 상태 %s (피해와 밀림만)"),
			*OwnerName, *StageName(Stage), *OwnedTags.Filter(ActivationBlockedTags).ToStringSimple());
		return false;
	}

	// 누운 동안은 반응하지 않는다. 단 그로기는 통과시킨다:
	// Event.Groggy는 경직치가 0이 되는 순간 한 번만 오므로, 여기서 막으면 경직치가 0에 머문 채 회복되지 않는다
	if (Stage != EHitReactionStage::Groggy && ASC && ASC->HasMatchingGameplayTag(CombatTags::State_Downed))
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 반응 없음: %s 맞음, 누운 구간 (피해와 밀림만)"), *OwnerName, *StageName(Stage));
		return false;
	}

	// 공격 중 움찔은 공격을 끊지 않는다. 몽타주 없이 히트스톱, 밀림, Cue로만 보인다 (6-7 결정)
	if (Stage == EHitReactionStage::Flinch && ASC && ASC->HasMatchingGameplayTag(CombatTags::State_Attacking))
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 공격 중 움찔: 공격을 이어 간다 (몽타주 없음)"), *OwnerName);
		return false;
	}

	// 반응 중에 낮은 단계로 맞으면 지금 반응을 유지한다 (같거나 높으면 재발동으로 처음부터)
	if (IsActive() && Stage < CurrentStage)
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] %s 중 %s 맞음: 지금 반응 유지"),
			*OwnerName, *StageName(CurrentStage), *StageName(Stage));
		return false;
	}

	return true;
}

void UGA_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentStage = GetStageFromEvent(TriggerEventData);

	ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;

	// 맞은 본으로 머리/몸통 몽타주를 고른다 (그로기 이벤트에는 타격 정보가 없다)
	const FHitResolution* Resolution = TriggerEventData ? UHitResolveSubsystem::FindHitResolution(TriggerEventData->TargetData) : nullptr;
	const FName HitBone = Resolution ? Resolution->Hit.BoneName : NAME_None;
	UAnimMontage* Montage = Data ? Data->FindReactionMontage(CurrentStage, Character->GetMesh(), HitBone) : nullptr;

	// 상태 태그: 경직 이상은 행동 불가(State.Staggered), 그로기는 State.Groggy도
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (CurrentStage >= EHitReactionStage::Stagger)
		{
			AddedStateTags.AddTag(CombatTags::State_Staggered);
		}
		if (CurrentStage == EHitReactionStage::Groggy)
		{
			AddedStateTags.AddTag(CombatTags::State_Groggy);
		}
		ASC->AddLooseGameplayTags(AddedStateTags);
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 피격 반응 %s: %s (맞은 본 %s)"),
		*GetNameSafe(Character), *StageName(CurrentStage), *GetNameSafe(Montage), *HitBone.ToString());

	if (!Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] 캐릭터 데이터에 %s 반응 몽타주가 없어 바로 끝냅니다"),
			*GetNameSafe(Character), *StageName(CurrentStage));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 같은 슬롯의 공격 몽타주는 여기서 끊기고, 공격 어빌리티는 자기 몽타주 취소 콜백으로 끝난다
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_HitReact::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_HitReact::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_HitReact::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_HitReact::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_HitReact::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsActive())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			ASC->RemoveLooseGameplayTags(AddedStateTags);
		}

		// 그로기는 몽타주가 끝나면 끝난다. 끊겨도 경직치 0에 머물지 않도록 어느 쪽이든 채운다 (6-7 결정)
		if (CurrentStage == EHitReactionStage::Groggy && PoiseRestoreEffect)
		{
			const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(PoiseRestoreEffect);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
			UE_LOG(LogFirstAction, Log, TEXT("[%s] 그로기 끝: 경직치 회복"), *GetNameSafe(GetAvatarActorFromActorInfo()));
		}
		else if (CurrentStage == EHitReactionStage::Groggy)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("[%s] 그로기 끝: PoiseRestoreEffect가 없어 경직치가 0에 머뭅니다"),
				*GetNameSafe(GetAvatarActorFromActorInfo()));
		}

		UE_LOG(LogFirstAction, Log, TEXT("[%s] 피격 반응 %s 종료 (%s)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *StageName(CurrentStage), bWasCancelled ? TEXT("끊김") : TEXT("완료"));
	}

	AddedStateTags.Reset();
	CurrentStage = EHitReactionStage::None;
	MontageTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_HitReact::OnMontageFinished()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_HitReact::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
