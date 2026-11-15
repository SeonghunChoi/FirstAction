// 플레이어와 적이 함께 상속하는 전투 캐릭터 기반 클래스. (PLAN 4.1, 4.2)
// ASC와 속성 세트를 갖고, 시작 어빌리티와 시작 이펙트를 부여한다.
// 입력이나 AI는 여기에 직접 공격 함수를 부르지 않고, ASC에 어빌리티 활성화만 요청한다(설계 규칙 1).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "CombatCharacterBase.generated.h"

class UAbilitySystemComponent;
class UCombatAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

// 시작할 때 부여할 어빌리티 한 줄. 같은 어빌리티 클래스를 다른 입력 태그로 여러 번 부여할 수 있다.
USTRUCT(BlueprintType)
struct FCombatAbilityGrant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> Ability;

	// 이 어빌리티를 실행할 입력 태그. 입력 없이 이벤트로만 발동하는 어빌리티(피격 반응 등)는 비워 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

UCLASS(Abstract)
class FIRSTACTION_API ACombatCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACombatCharacterBase();

	// IAbilitySystemInterface: 다른 시스템이 이 액터의 ASC를 찾는 표준 통로
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }

	// 입력 태그로 어빌리티 실행을 요청한다. 플레이어 입력도 AI도 이 두 함수만 부른다(설계 규칙 1)
	//   눌림: 짝인 어빌리티가 실행 중이 아니면 활성화를 시도하고, 실행 중이면 입력 눌림을 전달한다
	//   뗌:   실행 중인 어빌리티에 입력 뗌을 전달한다 (가드처럼 누르고 있는 동안만 유지하는 어빌리티용)
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// 시작할 때 부여할 어빌리티와 그 입력 태그 (공격, 회피, 피격 반응 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Startup", meta = (TitleProperty = "InputTag"))
	TArray<FCombatAbilityGrant> StartupAbilities;

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
