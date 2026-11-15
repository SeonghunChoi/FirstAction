#include "Combat/CombatCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Combat/CombatAttributeSet.h"

ACombatCharacterBase::ACombatCharacterBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// 캐릭터의 기본 서브오브젝트로 만든 속성 세트는 ASC가 초기화될 때 찾아서 등록한다
	CombatAttributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));
}

UAbilitySystemComponent* ACombatCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ACombatCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 소유자(Owner)와 몸(Avatar)이 모두 이 캐릭터. 컨트롤러가 없는 훈련 더미도 여기서 초기화된다
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	GrantStartupAbilitiesAndEffects();
}

void ACombatCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 플레이어 컨트롤러나 AI 컨트롤러가 붙은 뒤 ASC가 가진 컨트롤러 정보를 갱신한다
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void ACombatCharacterBase::GrantStartupAbilitiesAndEffects()
{
	if (bStartupGranted || !HasAuthority())
	{
		return;
	}
	bStartupGranted = true;

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : StartupEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}
