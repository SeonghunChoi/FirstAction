#include "Combat/TrainingDummy.h"

#include "AbilitySystemComponent.h"
#include "Combat/CombatAttributeSet.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "FirstAction.h"

ATrainingDummy::ATrainingDummy()
{
	// 컨트롤러 없이 서 있기만 한다
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ATrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	// 속성 값이 바뀔 때마다 알려 주는 ASC 델리게이트. 값을 바꾸는 쪽(GE)과 무관하게 결과만 지켜본다
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ATrainingDummy::OnVitalAttributeChanged);
		PoiseChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetPoiseAttribute())
			.AddUObject(this, &ATrainingDummy::OnVitalAttributeChanged);
	}
}

void ATrainingDummy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetPoiseAttribute()).Remove(PoiseChangedHandle);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefillTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ATrainingDummy::OnVitalAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	// 0으로 떨어진 순간에만 예약한다. 이미 예약돼 있으면 그대로 둔다(맞는 동안 계속 미뤄지지 않게)
	const bool bDroppedToZero = ChangeData.OldValue > 0.f && ChangeData.NewValue <= 0.f;
	UWorld* World = GetWorld();
	if (!bDroppedToZero || !World || World->GetTimerManager().IsTimerActive(RefillTimer))
	{
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] %s 0 → %.1f초 뒤 리필"),
		*GetName(), *ChangeData.Attribute.GetName(), RefillDelay);

	if (RefillDelay <= 0.f)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ATrainingDummy::Refill);
	}
	else
	{
		World->GetTimerManager().SetTimer(RefillTimer, this, &ATrainingDummy::Refill, RefillDelay, false);
	}
}

void ATrainingDummy::Refill()
{
	// 값을 직접 넣지 않고 시작 이펙트(GE_InitAttributes)를 다시 적용한다 (설계 규칙 5)
	ApplyStartupEffects();
	UE_LOG(LogFirstAction, Log, TEXT("[%s] 리필 완료"), *GetName());
}
