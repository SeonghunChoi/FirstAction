// 플레이어와 적이 함께 쓰는 전투 속성. (PLAN 4.2)
// 값은 GameplayEffect로만 바꾼다(설계 규칙 5). 그래서 값을 직접 바꾸는 Setter는 만들지 않는다.

#pragma once

#include "AttributeSet.h"
#include "CombatAttributeSet.generated.h"

// 속성마다 붙이는 접근자 묶음
//   GetHealthAttribute() : 속성 식별자 (GameplayEffect, 델리게이트 등록에 쓴다)
//   GetHealth()          : 현재 값
//   InitHealth(값)       : 기본값과 현재 값을 함께 초기화 (GE를 거치지 않으므로 생성 시점에만)
#define COMBAT_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class FIRSTACTION_API UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Health);
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxHealth);
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Poise);
	COMBAT_ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxPoise);

	// Instant 이펙트처럼 기본값(Base)이 바뀌기 직전
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// Duration/Infinite 이펙트의 모디파이어처럼 현재 값(Current)이 바뀌기 직전
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// Instant 이펙트(GE_Damage 등)가 속성을 바꾸기 직전/직후 (Step 5-4)
	// 값을 다시 계산하지 않는다(감소 계산은 매니저 몫). 경계를 넘었는지만 보고 이벤트를 보낸다
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	// Health는 0 ~ MaxHealth, Poise는 0 ~ MaxPoise, 최대값은 0 이상으로 제한
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// 실행 중 상태값: 에디터에서는 보기만, 블루프린트에서는 읽기만 (설계 규칙 9)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Health", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData Health;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Health", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxHealth;

	// 경직치. 맞을 때마다 줄고 0이 되면 그로기 (Step 5-4, 6-7)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Poise", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData Poise;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Poise", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxPoise;

	// 이펙트 실행 직전의 경직치. 범위 제한(0 미만 → 0) 때문에 실행 후 값과 변화량만으로는 이전 값을 알 수 없어서 기록한다
	float PoiseBeforeExecute = 0.f;

	// 이펙트 적용 전 체력. 0보다 큰 값에서 0이 되는 순간에만 Event.Death를 보낸다 (7-1)
	float HealthBeforeExecute = 0.f;
};
