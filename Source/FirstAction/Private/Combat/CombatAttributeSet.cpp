#include "Combat/CombatAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"

bool UCombatAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetPoiseAttribute())
	{
		PoiseBeforeExecute = GetPoise();
	}
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		HealthBeforeExecute = GetHealth();
	}
	return Super::PreGameplayEffectExecute(Data);
}

void UCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 체력이 0보다 큰 값에서 0이 된 순간 한 번 Event.Death (7-1). 사망 처리는 GA_Death가 한다
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (ASC && HealthBeforeExecute > 0.f && GetHealth() <= 0.f)
		{
			FGameplayEventData Payload;
			Payload.EventTag = CombatTags::Event_Death;
			Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
			Payload.Target = ASC->GetAvatarActor();
			Payload.ContextHandle = Data.EffectSpec.GetContext();
			UE_LOG(LogFirstAction, Log, TEXT("[%s] 체력 0 → Event.Death (공격자 %s)"),
				*GetNameSafe(ASC->GetAvatarActor()), *GetNameSafe(Payload.Instigator.Get()));
			ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
		}
		return;
	}

	if (Data.EvaluatedData.Attribute != GetPoiseAttribute())
	{
		return;
	}

	// 이번 이펙트로 경직치가 0보다 큰 값에서 0으로 떨어진 순간에만 한 번 알린다
	// (이미 0인 그로기 상태에서 더 맞아도 다시 보내지 않는다)
	const float NewPoise = GetPoise();
	if (PoiseBeforeExecute <= 0.f || NewPoise > 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = CombatTags::Event_Groggy;
	Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
	Payload.Target = ASC->GetAvatarActor();
	Payload.ContextHandle = Data.EffectSpec.GetContext();

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 경직치 0 → Event.Groggy (공격자 %s)"),
		*GetNameSafe(ASC->GetAvatarActor()), *GetNameSafe(Payload.Instigator.Get()));

	// 반응(그로기 몽타주, State.Groggy 태그, 경직치 회복)은 Step 6-7 GA_HitReact가 이 이벤트로 처리한다
	ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
}

void UCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UCombatAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetPoiseAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPoise());
	}
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxPoiseAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}
