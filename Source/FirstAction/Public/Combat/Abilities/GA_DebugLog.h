// 활성화되면 로그만 남기고 바로 끝나는 테스트 어빌리티. (Step 2-3)
// 입력 → 입력 태그 → 어빌리티 경로를 확인하고, 새 입력을 붙일 때 연결만 먼저 시험하는 용도로 남겨 둔다.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_DebugLog.generated.h"

UCLASS()
class FIRSTACTION_API UGA_DebugLog : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_DebugLog();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
