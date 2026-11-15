#include "Combat/Abilities/GA_DebugLog.h"

#include "Engine/Engine.h"
#include "FirstAction.h"

UGA_DebugLog::UGA_DebugLog()
{
	// 캐릭터마다 인스턴스 하나. 상태를 갖는 어빌리티(콤보, 몽타주 대기)는 모두 이 방식을 쓴다
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_DebugLog::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 비용과 쿨다운을 확정한다. 실패하면 실행하지 않고 끝낸다
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 어떤 입력 태그로 부여된 스펙인지 함께 찍는다. 같은 클래스를 두 입력에 묶었는지 확인하는 데 쓴다
	FString InputTags = TEXT("(없음)");
	if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		InputTags = Spec->GetDynamicSpecSourceTags().ToStringSimple();
	}

	const FString Message = FString::Printf(TEXT("[%s] %s 활성화 (입력 태그: %s)"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetClass()->GetName(), *InputTags);

	UE_LOG(LogFirstAction, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, Message);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
