#include "Combat/Abilities/CombatGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"

bool UCombatGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || CancelWindowRequiredTags.IsEmpty())
	{
		return true;
	}

	// 다른 행동 중인데 그 행동의 캔슬 구간이 아니면 시작하지 않는다 (6-9 결정)
	if (ASC->HasAnyMatchingGameplayTags(CancelWindowRequiredTags) && !ASC->HasMatchingGameplayTag(CombatTags::Window_Cancel))
	{
		return false;
	}
	return true;
}
