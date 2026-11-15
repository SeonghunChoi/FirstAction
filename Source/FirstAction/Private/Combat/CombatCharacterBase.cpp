#include "Combat/CombatCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "Combat/CombatAttributeSet.h"
#include "FirstAction.h"

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

void ACombatCharacterBase::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	bool bFound = false;
	TArray<FGameplayAbilitySpecHandle, TInlineAllocator<4>> HandlesToActivate;

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		bFound = true;
		if (Spec.IsActive())
		{
			// 이미 실행 중이면 입력이 들어왔다는 사실만 알린다 (콤보 입력은 Step 4에서 사용)
			AbilitySystemComponent->AbilitySpecInputPressed(Spec);
		}
		else
		{
			HandlesToActivate.Add(Spec.Handle);
		}
	}

	// 목록을 도는 중에 활성화하면 목록이 바뀔 수 있으므로, 핸들을 모은 뒤 따로 활성화한다
	for (const FGameplayAbilitySpecHandle& Handle : HandlesToActivate)
	{
		AbilitySystemComponent->TryActivateAbility(Handle);
	}

	if (!bFound)
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 입력 태그 %s에 연결된 어빌리티가 없습니다"), *GetName(), *InputTag.ToString());
	}
}

void ACombatCharacterBase::AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.IsActive() && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySystemComponent->AbilitySpecInputReleased(Spec);
		}
	}
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

	for (const FCombatAbilityGrant& Grant : StartupAbilities)
	{
		if (!Grant.Ability)
		{
			continue;
		}

		// 부여 건에 데이터가 있으면 그것을, 없으면 캐릭터를 SourceObject로 둔다
		UObject* SourceObject = Grant.AbilityData ? static_cast<UObject*>(Grant.AbilityData) : static_cast<UObject*>(this);
		FGameplayAbilitySpec Spec(Grant.Ability, 1, INDEX_NONE, SourceObject);
		if (Grant.InputTag.IsValid())
		{
			// 입력 태그는 클래스가 아니라 이 부여 건(스펙)에 붙인다. 같은 클래스를 다른 태그로 또 부여할 수 있다
			Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
		}
		AbilitySystemComponent->GiveAbility(Spec);
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
