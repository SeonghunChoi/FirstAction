#include "Combat/TrainingDummy.h"

#include "AbilitySystemComponent.h"
#include "Combat/CombatAttributeSet.h"
#include "Combat/CombatTags.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "FirstAction.h"

ATrainingDummy::ATrainingDummy()
{
	// 컨트롤러 없이 서 있기만 한다
	AutoPossessAI = EAutoPossessAI::Disabled;

	// 컨트롤러가 없는 캐릭터는 기본적으로 이동 계산을 건너뛴다 (루트 모션 몽타주 재생 중만 예외).
	// 그러면 타격 후 밀림(Root Motion Source, 6-5)과 중력이 적용되지 않으므로 컨트롤러 없이도 이동을 계산하게 한다
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void ATrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	// 속성 값이 바뀔 때마다 알려 주는 ASC 델리게이트. 값을 바꾸는 쪽(GE)과 무관하게 결과만 지켜본다
	// 경직치는 그로기 규칙이 채우므로(6-7) 체력만 본다
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ATrainingDummy::OnHealthChanged);
	}

	if (DefenseMode != ETrainingDummyDefense::None && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(DefenseTimer, this, &ATrainingDummy::TickDefense, DefenseInterval, true, 0.1f);
	}
}

void ATrainingDummy::TickDefense()
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	switch (DefenseMode)
	{
	case ETrainingDummyDefense::HoldGuard:
		// 누른 채 떼지 않는다. 가드가 깨졌으면(가드 중이 아니면) 다시 누른다
		if (!ASC->HasMatchingGameplayTag(CombatTags::State_Guarding))
		{
			AbilityInputTagPressed(CombatTags::InputTag_Guard);
		}
		break;

	case ETrainingDummyDefense::RepeatGuard:
		// 살짝 누르기: 가드를 올렸다(앞부분 Window.Parry) 바로 내린다
		AbilityInputTagPressed(CombatTags::InputTag_Guard);
		AbilityInputTagReleased(CombatTags::InputTag_Guard);
		break;

	case ETrainingDummyDefense::RepeatDodge:
		AbilityInputTagPressed(CombatTags::InputTag_Dodge);
		AbilityInputTagReleased(CombatTags::InputTag_Dodge);
		break;

	default:
		break;
	}
}

void ATrainingDummy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefillTimer);
		World->GetTimerManager().ClearTimer(DefenseTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ATrainingDummy::OnHealthChanged(const FOnAttributeChangeData& ChangeData)
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
