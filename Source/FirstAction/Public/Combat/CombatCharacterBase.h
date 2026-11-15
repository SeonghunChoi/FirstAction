// 플레이어와 적이 함께 상속하는 전투 캐릭터 기반 클래스. (PLAN 4.1, 4.2)
// ASC와 속성 세트를 갖고, 시작 어빌리티와 시작 이펙트를 부여한다.
// 입력이나 AI는 여기에 직접 공격 함수를 부르지 않고, ASC에 어빌리티 활성화만 요청한다(설계 규칙 1).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "CombatCharacterBase.generated.h"

class UAbilitySystemComponent;
class UCombatAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

UCLASS(Abstract)
class FIRSTACTION_API ACombatCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACombatCharacterBase();

	// IAbilitySystemInterface: 다른 시스템이 이 액터의 ASC를 찾는 표준 통로
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// 시작할 때 부여할 어빌리티 (회피, 피격 반응 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Startup")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	// 시작할 때 자신에게 적용할 이펙트 (속성 초기값 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Startup")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

private:
	void GrantStartupAbilitiesAndEffects();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatAttributeSet> CombatAttributeSet;

	// 시작 어빌리티/이펙트를 두 번 부여하지 않도록
	bool bStartupGranted = false;
};
