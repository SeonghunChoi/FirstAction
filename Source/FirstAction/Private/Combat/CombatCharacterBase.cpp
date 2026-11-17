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
#include "TimerManager.h"
#include "EngineUtils.h"
#include "MotionWarpingComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Combat/PartialHitReactionComponent.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/HitResolveSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

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

	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	PhysicalAnimation = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));
	PartialHitReaction = CreateDefaultSubobject<UPartialHitReactionComponent>(TEXT("PartialHitReaction"));
}

FVector ACombatCharacterBase::GetAttackTargetDirection() const
{
	// 마지막 이동 틱에서 소비한 이동 입력. 스틱을 놓으면 0이 된다
	const FVector InputDirection = GetLastMovementInputVector().GetSafeNormal2D();
	return InputDirection.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : InputDirection;
}

AActor* ACombatCharacterBase::FindAttackTarget(float MaxDistance) const
{
	UWorld* World = GetWorld();
	if (MaxDistance <= 0.f || AttackTargetConeAngle <= 0.f || !World)
	{
		return nullptr;
	}

	const FVector Origin = GetActorLocation();
	const FVector Direction = GetAttackTargetDirection();
	const float CosHalfCone = FMath::Cos(FMath::DegreesToRadians(AttackTargetConeAngle * 0.5f));
	const float MyRadius = GetSimpleCollisionRadius();

	ACombatCharacterBase* BestTarget = nullptr;
	float BestCos = -2.f;
	float BestGap = TNumericLimits<float>::Max();

	// 전투 캐릭터 수는 많지 않으므로 전부 훑는다
	for (TActorIterator<ACombatCharacterBase> It(World); It; ++It)
	{
		ACombatCharacterBase* Candidate = *It;
		if (Candidate == this || Candidate->IsActorBeingDestroyed()
			|| Candidate->AbilitySystemComponent->HasMatchingGameplayTag(CombatTags::State_Dead))
		{
			continue;
		}
		// 같은 편 제외는 적끼리 공존하는 Step 7에서 추가한다

		// 거리는 몸끼리의 간격(중심 거리 − 양쪽 캡슐 반지름)으로 잰다. 워핑 목표 계산과 같은 기준
		const FVector ToCandidate = Candidate->GetActorLocation() - Origin;
		const float Gap = ToCandidate.Size2D() - MyRadius - Candidate->GetSimpleCollisionRadius();
		if (Gap > MaxDistance)
		{
			continue;
		}

		const FVector ToCandidateDir = ToCandidate.GetSafeNormal2D();
		const float Cos = ToCandidateDir.IsNearlyZero() ? 1.f : FVector::DotProduct(Direction, ToCandidateDir);
		if (Cos < CosHalfCone)
		{
			continue;
		}

		// 각도 우선(코사인이 클수록 각도가 작다). 각도가 같으면 가까운 쪽
		const bool bBetterAngle = Cos > BestCos + KINDA_SMALL_NUMBER;
		const bool bSameAngleCloser = FMath::IsNearlyEqual(Cos, BestCos, KINDA_SMALL_NUMBER) && Gap < BestGap;
		if (bBetterAngle || bSameAngleCloser)
		{
			BestTarget = Candidate;
			BestCos = Cos;
			BestGap = Gap;
		}
	}

	return BestTarget;
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

void ACombatCharacterBase::HandleHitReactEvent(const FGameplayEventData* Payload)
{
	// 타격 해석 결과(6-4)에서 자기 몫만 꺼내 적용한다: 히트스톱과 밀림(6-5), 부위 물리 반응(6-8). 반응 몽타주는 6-7 GA_HitReact
	// 셋 다 몸에 생기는 물리 결과라 반응 어빌리티가 막혀도 적용한다(6-7, 6-8 결정). 부위 반응은 컴포넌트가 누운 구간·가드만 거른다
	const FHitResolution* Resolution = Payload ? UHitResolveSubsystem::FindHitResolution(Payload->TargetData) : nullptr;
	if (Resolution)
	{
		ApplyHitStop(Resolution->VictimHitStop);
		ApplyHitDisplacement(Resolution->Direction, Resolution->VictimDisplacement, Resolution->MoveDuration);
		if (PartialHitReaction && !Resolution->PartialReactionVelocity.IsNearlyZero())
		{
			PartialHitReaction->ApplyPartialHit(Resolution->Hit.BoneName, Resolution->PartialReactionVelocity);
		}
	}
}

void ACombatCharacterBase::ApplyHitStop(float Duration)
{
	if (Duration <= 0.f || HitStopTimeDilation >= 1.f || !GetWorld())
	{
		return;
	}

	// 타이머는 월드 시간으로 돈다. 이 캐릭터의 CustomTimeDilation에 영향을 받지 않으므로 정확히 Duration 뒤에 풀린다
	FTimerManager& TimerManager = GetWorldTimerManager();
	if (TimerManager.IsTimerActive(HitStopTimer) && TimerManager.GetTimerRemaining(HitStopTimer) >= Duration)
	{
		return;
	}

	CustomTimeDilation = HitStopTimeDilation;
	TimerManager.SetTimer(HitStopTimer, this, &ACombatCharacterBase::EndHitStop, Duration, false);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 히트스톱 %.3f초 (배율 %.2f)"), *GetName(), Duration, HitStopTimeDilation);
}

void ACombatCharacterBase::EndHitStop()
{
	CustomTimeDilation = 1.f;
}

namespace
{
	const FName HitDisplacementSourceName(TEXT("HitDisplacement"));

	// 이동 시간에 더해 주는 여유. 히트스톱으로 늘어난 시간을 감안한 값이고, 이보다 오래 남은 이동은 버린다
	constexpr double HitDisplacementExpireMargin = 1.0;
}

void ACombatCharacterBase::ApplyHitDisplacement(const FVector& Direction, float Distance, float Duration)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FVector Offset = Direction.GetSafeNormal2D() * Distance;
	if (!Movement || !GetWorld() || Duration <= 0.f || Offset.SizeSquared() < 1.f)
	{
		return;
	}

	// 이전 타격의 남은 이동은 새 타격으로 대체한다
	PendingHitDisplacement = FVector::ZeroVector;
	PendingHitDisplacementTime = 0.f;
	Movement->RemoveRootMotionSource(HitDisplacementSourceName);

	if (IsPlayingRootMotion())
	{
		// 루트 모션 몽타주 재생 중: 이동 컴포넌트가 루트 모션을 처리할 때마다 남은 몫을 더한다
		PendingHitDisplacement = Offset;
		PendingHitDisplacementTime = Duration;
		PendingHitDisplacementExpireTime = GetWorld()->GetTimeSeconds() + Duration + HitDisplacementExpireMargin;

		UE_LOG(LogFirstAction, Log, TEXT("[%s] 타격 후 이동 %.0fcm / %.2f초 (애니메이션 루트 모션에 더함)"), *GetName(), Distance, Duration);
		return;
	}

	// 루트 모션 몽타주가 없음: 일정 속도로 Duration 동안 옮기는 Root Motion Source
	TSharedPtr<FRootMotionSource_ConstantForce> Source = MakeShared<FRootMotionSource_ConstantForce>();
	Source->InstanceName = HitDisplacementSourceName;
	Source->AccumulateMode = ERootMotionAccumulateMode::Override;
	Source->Priority = 5;
	Source->Force = Offset / Duration;  // 속도(cm/s)
	Source->Duration = Duration;
	// 수평만 덮어쓰고 중력(Z)은 그대로 둔다
	Source->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	// 끝나면 멈춘다. 기본값(마지막 속도 유지)이면 마찰로 더 미끄러져 간격이 어긋난다
	Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Source->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	Movement->ApplyRootMotionSource(Source);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 타격 후 이동 %.0fcm / %.2f초 (Root Motion Source)"), *GetName(), Distance, Duration);
}

FTransform ACombatCharacterBase::ProcessHitDisplacementRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	if (PendingHitDisplacementTime <= 0.f || DeltaSeconds <= 0.f)
	{
		return InRootMotion;
	}

	// 몽타주가 도중에 끝나 오래 남은 이동은 버린다 (다음 공격 몽타주로 새지 않도록)
	if (GetWorld() && GetWorld()->GetTimeSeconds() > PendingHitDisplacementExpireTime)
	{
		PendingHitDisplacement = FVector::ZeroVector;
		PendingHitDisplacementTime = 0.f;
		return InRootMotion;
	}

	// DeltaSeconds는 이 캐릭터의 시간(CustomTimeDilation 적용)이다. 히트스톱 동안은 거의 진행되지 않는다
	const float Alpha = FMath::Min(DeltaSeconds / PendingHitDisplacementTime, 1.f);
	const FVector Step = PendingHitDisplacement * Alpha;
	PendingHitDisplacement -= Step;
	PendingHitDisplacementTime -= DeltaSeconds;
	if (PendingHitDisplacementTime <= 0.f)
	{
		PendingHitDisplacement = FVector::ZeroVector;
		PendingHitDisplacementTime = 0.f;
	}

	FTransform OutRootMotion = InRootMotion;
	OutRootMotion.AddToTranslation(Step);
	return OutRootMotion;
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

	// 질량의 정본은 캐릭터 데이터. 엔진 물리(충격, 물체 밀기)가 쓰는 무브먼트 Mass를 같은 값으로 맞춘다 (6-3 결정)
	if (CharacterData)
	{
		GetCharacterMovement()->Mass = CharacterData->Mass;
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 캐릭터 데이터 %s: 질량 %.0fkg"), *GetName(), *GetNameSafe(CharacterData), CharacterData->Mass);
	}
	else
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] CharacterData가 없습니다. 블루프린트 기본값에서 지정하세요 (타격 해석에 질량이 필요)"), *GetName());
	}

	// 타격 후 이동을 애니메이션 루트 모션에 더하는 연결 지점 (6-5)
	GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.BindUObject(this, &ACombatCharacterBase::ProcessHitDisplacementRootMotion);

	MeleeTrace->SetWeaponMesh(WeaponMesh);
	MeleeTrace->OnHit.AddUObject(this, &ACombatCharacterBase::HandleMeleeTraceHit);

	// 맞은 쪽 히트스톱 통로. 어빌리티 없이 이벤트 태그 하나만 듣는다 (정확히 이 태그일 때만 불린다)
	AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(CombatTags::Event_HitReact)
		.AddUObject(this, &ACombatCharacterBase::HandleHitReactEvent);

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
