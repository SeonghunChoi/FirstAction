#include "Combat/CombatCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Combat/CombatTags.h"
#include "Engine/DataAsset.h"
#include "Engine/CollisionProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayEffect.h"
#include "Combat/CombatAttributeSet.h"
#include "Combat/MeleeTraceComponent.h"
#include "FirstAction.h"

ACombatCharacterBase::ACombatCharacterBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// 캐릭터의 기본 서브오브젝트로 만든 속성 세트는 ASC가 초기화될 때 찾아서 등록한다
	CombatAttributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));

	// 무기 메시: 마네킹 기본 소켓 HandGrip_R에 붙인다 (CLAUDE.md 5.1). 블루프린트에서 부모 소켓을 바꿀 수 있다
	// 판정은 소켓 스윕으로 하므로 무기 몸체는 아무것과도 충돌하지 않는다
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
	WeaponMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	WeaponMesh->SetGenerateOverlapEvents(false);

	MeleeTrace = CreateDefaultSubobject<UMeleeTraceComponent>(TEXT("MeleeTrace"));
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
	bool bActivatedNew = false;
	for (const FGameplayAbilitySpecHandle& Handle : HandlesToActivate)
	{
		if (AbilitySystemComponent->TryActivateAbility(Handle))
		{
			bActivatedNew = true;
		}
	}

	// 새 어빌리티를 시작한 입력은 이미 쓰였으므로 보관하지 않는다.
	// 막혔거나(공격 중 강공격), 실행 중인 어빌리티에 전달만 된 입력은 나중에 꺼내 쓸 수 있게 보관한다
	if (!bActivatedNew)
	{
		BufferInput(InputTag);
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

void ACombatCharacterBase::HandleMeleeTraceHit(const FHitResult& Hit)
{
	FGameplayEventData Payload;
	Payload.EventTag = CombatTags::Event_Hit;
	Payload.Instigator = this;
	Payload.Target = Hit.GetActor();

	// 맞은 위치, 맞은 본 등 판정 결과 전체를 타깃 데이터로 싣는다 (6-4 타격 해석, 6-8 부위 반응에서 사용)
	Payload.TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hit));

	AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
}

void ACombatCharacterBase::BufferInput(const FGameplayTag& InputTag)
{
	if (InputBufferDuration <= 0.f || !GetWorld())
	{
		return;
	}

	BufferedInput.InputTag = InputTag;
	BufferedInput.PressedTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 입력 버퍼 기록: %s"), *GetName(), *InputTag.ToString());

	OnInputBuffered.Broadcast(InputTag);
}

bool ACombatCharacterBase::ConsumeBufferedInput(const FGameplayTagContainer& AcceptedInputTags, FGameplayTag& OutInputTag)
{
	if (!BufferedInput.InputTag.IsValid() || !GetWorld())
	{
		return false;
	}

	// 보관 시간이 지난 입력은 버린다
	const double Age = GetWorld()->GetTimeSeconds() - BufferedInput.PressedTime;
	if (Age > InputBufferDuration)
	{
		BufferedInput = FCombatBufferedInput();
		return false;
	}

	// 이 소비자가 받지 않는 입력은 남겨 둔다
	if (!AcceptedInputTags.HasTagExact(BufferedInput.InputTag))
	{
		return false;
	}

	OutInputTag = BufferedInput.InputTag;
	BufferedInput = FCombatBufferedInput();

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 입력 버퍼 소비: %s (%.2f초 전 입력)"), *GetName(), *OutInputTag.ToString(), Age);
	return true;
}

void ACombatCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	MeleeTrace->SetWeaponMesh(WeaponMesh);
	MeleeTrace->OnHit.AddUObject(this, &ACombatCharacterBase::HandleMeleeTraceHit);

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

	ApplyStartupEffects();
}

void ACombatCharacterBase::ApplyStartupEffects()
{
	if (!HasAuthority())
	{
		return;
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
