#include "Combat/Abilities/GA_MeleeAttack.h"
#include "Content/CombatContentSubsystem.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "GameplayEffect.h"
#include "Animation/AnimMontage.h"
#include "Combat/AttackDataAsset.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatTags.h"
#include "Combat/HitResolveSubsystem.h"
#include "FirstAction.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include "Animation/AnimSequenceBase.h"
#include "Combat/Animation/ANS_AttackReach.h"
#include "Combat/Animation/ANS_HitWindow.h"
#include "Combat/Animation/ANS_Telegraph.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/MeleeTraceComponent.h"
#include "AnimNotifyState_MotionWarping.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "TimerManager.h"

namespace
{
	// 콘솔에서 FirstAction.ShowWarpTarget 1 → 고른 대상(주황 선)과 워핑 목표(초록 구, 화살표)를 그린다
	TAutoConsoleVariable<bool> CVarShowWarpTarget(
		TEXT("FirstAction.ShowWarpTarget"),
		false,
		TEXT("공격 대상과 모션 워핑 목표를 화면에 그린다 (0: 끔, 1: 켬)"));
}

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 실행 중에는 State.Attacking을 붙인다 (어빌리티가 끝나면 자동으로 떨어진다)
	// 콤보는 같은 실행 안에서 이어지므로 타와 타 사이에도 이 태그가 빠지지 않는다
	ActivationOwnedTags.AddTag(CombatTags::State_Attacking);

	// 이미 공격 중이면 다른 공격 스펙(약공격 중 강공격 버튼 등)이 따로 시작되지 않게 막는다
	// 공격 중 입력은 입력 버퍼를 거쳐 이 어빌리티의 콤보 분기로 들어온다
	ActivationBlockedTags.AddTag(CombatTags::State_Attacking);

	// 경직(그로기, 넘어짐 포함) 중에는 공격을 시작하지 못한다 (6-7). 움찔(State.Flinching)은 막지 않아 공격으로 끊고 나갈 수 있다
	ActivationBlockedTags.AddTag(CombatTags::State_Staggered);

	// 공중에서는 시작하지 못한다. 누른 입력은 버퍼에 남았다가 착지하는 순간 다시 시도된다 (2026-11-19 결정)
	ActivationBlockedTags.AddTag(CombatTags::State_InAir);

	// 회피·가드 중에는 그 몽타주의 캔슬 구간(Window.Cancel)에서만 공격으로 이어 간다 (6-9, 6-11 결정)
	CancelWindowRequiredTags.AddTag(CombatTags::State_Dodging);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Guarding);

	// AI의 공격 요청 (7-1): Event.AttackRequest 게임플레이 이벤트로도 시작한다. 요청자는 이 어빌리티를 모른다(규칙 1)
	FAbilityTriggerData RequestTrigger;
	RequestTrigger.TriggerTag = CombatTags::Event_AttackRequest;
	RequestTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(RequestTrigger);
}

const UAttackDataAsset* UGA_MeleeAttack::ResolveStartAttack(const FGameplayEventData* TriggerEventData) const
{
	// 페이로드 우선 (2026-11-16 결정): AI는 공격 요청 이벤트에 공격 데이터를 실어 보낸다
	if (TriggerEventData)
	{
		if (const UAttackDataAsset* Requested = Cast<UAttackDataAsset>(TriggerEventData->OptionalObject))
		{
			return Requested;
		}
	}
	return Cast<UAttackDataAsset>(GetCurrentSourceObject());
}

ACombatCharacterBase* UGA_MeleeAttack::GetCombatCharacter() const
{
	return Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
}

void UGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	CurrentAttack = ResolveStartAttack(TriggerEventData);
	if (!CurrentAttack || !CurrentAttack->Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_MeleeAttack: 시작 공격 데이터 또는 몽타주가 없습니다. 부여 목록의 Ability Data를 확인하세요."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] GA_MeleeAttack 시작: %s"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack));

	bComboContinuation = false;
	PlayCurrentAttackMontage();

	// 콤보 구간이 열리는 순간을 기다린다. 콤보가 이어지면 다음 몽타주의 구간에서도 다시 불린다(OnlyTriggerOnce = false)
	UAbilityTask_WaitGameplayTagAdded* WaitComboWindow =
		UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, CombatTags::Window_Combo, nullptr, false);
	WaitComboWindow->Added.AddDynamic(this, &UGA_MeleeAttack::OnComboWindowOpened);
	WaitComboWindow->ReadyForActivation();

	// 판정이 대상을 맞히면 캐릭터가 Event.Hit을 보낸다. 콤보가 이어지는 동안 계속 받는다
	UAbilityTask_WaitGameplayEvent* WaitHit =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, CombatTags::Event_Hit, nullptr, false, true);
	WaitHit->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnHitEvent);
	WaitHit->ReadyForActivation();

	// 구간 안에서 누른 입력도 바로 처리하도록 버퍼 기록을 구독한다
	if (ACombatCharacterBase* Character = GetCombatCharacter())
	{
		InputBufferedHandle = Character->OnInputBuffered.AddUObject(this, &UGA_MeleeAttack::OnInputBuffered);

		// 워핑 구간이 시작될 때마다 대상을 다시 묻는다
		if (UMotionWarpingComponent* MotionWarping = Character->GetMotionWarping())
		{
			RefreshedWarpModifiers.Reset();
			MotionWarping->OnPreUpdate.AddDynamic(this, &UGA_MeleeAttack::OnMotionWarpingPreUpdate);
		}
	}
}

void UGA_MeleeAttack::PlayCurrentAttackMontage()
{
	// 콤보로 몽타주를 바꾸면 이전 몽타주는 "끊김"으로 끝난다.
	// 그 콜백이 어빌리티를 끝내지 않도록 먼저 떼어 내고 이전 태스크를 정리한다
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveAll(this);
		MontageTask->OnBlendOut.RemoveAll(this);
		MontageTask->OnInterrupted.RemoveAll(this);
		MontageTask->OnCancelled.RemoveAll(this);
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	if (!CurrentAttack || !CurrentAttack->Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_MeleeAttack: %s에 몽타주가 없습니다."),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack));
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	// 이전 몽타주의 목표는 지운다. 새 목표는 이 몽타주의 Motion Warping 구간이 시작될 때 등록한다(OnMotionWarpingPreUpdate)
	if (ACombatCharacterBase* Character = GetCombatCharacter())
	{
		if (UMotionWarpingComponent* MotionWarping = Character->GetMotionWarping())
		{
			MotionWarping->RemoveWarpTarget(WarpTargetName);
		}
	}

	// 몽타주를 재생하고 끝날 때까지 기다리는 어빌리티 태스크 (코루틴의 yield 지점에 해당)
	// 어빌리티가 먼저 끝나면 몽타주도 함께 멈춘다(bStopWhenAbilityEnds 기본값 true)
	// 콘텐츠 세트(9-1): 배포 세트에 짝이 있으면 그 몽타주를 재생한다. 공격 데이터는 한 벌
	UAnimMontage* Montage = UCombatContentSubsystem::ResolveMontage(this, CurrentAttack->Montage);
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// 적 공격 예비 동작 감속과 번쩍임 (9-4 이후). 접근 타이머가 감속을 반영하도록 먼저 켠다
	BeginTelegraph(Montage);

	// 제자리 동작(루트 모션 없음)이면 워핑 구간 동안 직접 옮긴다 (9-3)
	ClearApproaches();
	if (Montage && !Montage->HasRootMotion())
	{
		ScheduleApproaches(Montage);
	}
}

float UGA_MeleeAttack::FindStopGap(const URootMotionModifier* Modifier)
{
	return Modifier ? FindStopGapAt(Modifier->Animation.Get(), Modifier->StartTime) : 0.f;
}

float UGA_MeleeAttack::FindStopGapAt(const UAnimSequenceBase* Animation, float StartTime)
{
	// 워핑 구간이 시작되는 시점을 덮는 Attack Reach 노티파이를 같은 애니메이션(몽타주)에서 찾는다
	if (!Animation)
	{
		return 0.f;
	}

	for (const FAnimNotifyEvent& Event : Animation->Notifies)
	{
		const UANS_AttackReach* Reach = Cast<UANS_AttackReach>(Event.NotifyStateClass);
		if (Reach && Event.GetTriggerTime() <= StartTime + KINDA_SMALL_NUMBER && StartTime < Event.GetEndTriggerTime())
		{
			return Reach->StopGap;
		}
	}
	return 0.f;
}

bool UGA_MeleeAttack::ComputeWarpGoal(float StopGap, FVector& OutLocation, FVector& OutDirection, float& OutMoveDistance, AActor*& OutTarget, bool bVerbose) const
{
	ACombatCharacterBase* Character = GetCombatCharacter();
	if (!Character || !CurrentAttack)
	{
		return false;
	}

	const float MaxWarpDistance = CurrentAttack->MaxWarpDistance;
	OutTarget = MaxWarpDistance > 0.f ? Character->FindAttackTarget(MaxWarpDistance) : nullptr;
	if (!OutTarget)
	{
		return false;
	}

	const FVector From = Character->GetActorLocation();
	const FVector ToTarget = (OutTarget->GetActorLocation() - From) * FVector(1.f, 1.f, 0.f);
	OutDirection = ToTarget.GetSafeNormal();
	if (OutDirection.IsNearlyZero())
	{
		return false;
	}

	// 몸 간격이 StopGap이 되는 곳까지만, 그리고 최대 거리까지만 (6-2 결정 + 휘두름별 멈출 간격)
	// StopGap은 몽타주의 Attack Reach 노티파이 값, 없으면 0(몸이 닿는 거리). 이미 그보다 가까우면 이동 0, 방향만 돌린다
	const float ContactSpacing = Character->GetSimpleCollisionRadius() + OutTarget->GetSimpleCollisionRadius();
	OutMoveDistance = FMath::Clamp(ToTarget.Size() - ContactSpacing - StopGap, 0.f, MaxWarpDistance);
	OutLocation = From + OutDirection * OutMoveDistance;

	if (!bVerbose)
	{
		return true;
	}
	UE_LOG(LogFirstAction, Log, TEXT("[%s] 워핑: %s → %s (간격 %.0f, 멈출 간격 %.0f, 이동 %.0f / 최대 %.0f)"),
		*GetNameSafe(Character), *GetNameSafe(CurrentAttack), *GetNameSafe(OutTarget),
		ToTarget.Size() - ContactSpacing, StopGap, OutMoveDistance, MaxWarpDistance);

	if (CVarShowWarpTarget.GetValueOnGameThread())
	{
		UWorld* World = Character->GetWorld();
		DrawDebugLine(World, From, OutTarget->GetActorLocation(), FColor::Orange, false, 1.5f, 0, 1.5f);
		DrawDebugSphere(World, OutLocation, 15.f, 12, FColor::Green, false, 1.5f);
		DrawDebugDirectionalArrow(World, OutLocation, OutLocation + OutDirection * 60.f, 20.f, FColor::Green, false, 1.5f, 0, 2.f);
	}
	return true;
}

void UGA_MeleeAttack::ScheduleApproaches(const UAnimMontage* Montage)
{
	ACombatCharacterBase* Character = GetCombatCharacter();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	// 몽타주의 Motion Warping 구간(이 공격의 목표 이름)을 그대로 접근 구간으로 쓴다. 타이밍은 애니메이션에 (규칙 3)
	const float Rate = FMath::Max(Montage->RateScale, KINDA_SMALL_NUMBER);
	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		const UAnimNotifyState_MotionWarping* Warping = Cast<UAnimNotifyState_MotionWarping>(Event.NotifyStateClass);
		const URootMotionModifier_Warp* Warp = Warping ? Cast<URootMotionModifier_Warp>(Warping->RootMotionModifier) : nullptr;
		if (!Warp || Warp->WarpTargetName != WarpTargetName)
		{
			continue;
		}

		const float StartTime = Event.GetTriggerTime();
		const float StopGap = FindStopGapAt(Montage, StartTime);
		const float Duration = Event.GetDuration() / Rate;
		if (StartTime <= KINDA_SMALL_NUMBER)
		{
			StartApproach(StopGap, Duration);
			continue;
		}

		FTimerHandle& Handle = ApproachTimers.AddDefaulted_GetRef();
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, StopGap, Duration]()
		{
			if (IsActive())
			{
				StartApproach(StopGap, Duration);
			}
		}), MontageTimeToWorldDelay(StartTime, Rate), false);
	}
}

float UGA_MeleeAttack::MontageTimeToWorldDelay(float MontageTime, float RateScale) const
{
	// 예비 동작 감속 구간은 배율만큼 늘어난다. 히트스톱 등 그 밖의 시간 배율은 예측하지 않는다
	const float Rate = FMath::Max(RateScale, KINDA_SMALL_NUMBER);
	float Delay = MontageTime;
	for (const FTelegraphInterval& Interval : TelegraphIntervals)
	{
		const float Overlap = FMath::Clamp(MontageTime, Interval.Start, Interval.End) - Interval.Start;
		Delay += Overlap * (1.f / FMath::Max(Interval.Scale, 0.05f) - 1.f);
	}
	return Delay / Rate;
}

void UGA_MeleeAttack::BeginTelegraph(const UAnimMontage* Montage)
{
	EndTelegraph();

	ACombatCharacterBase* Character = GetCombatCharacter();
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;
	UMeleeTraceComponent* MeleeTrace = Character ? Character->GetMeleeTrace() : nullptr;
	const float Scale = Data ? Data->GetTelegraphTimeScale(bComboContinuation) : 1.f;
	if (!Montage || !MeleeTrace || !Data || !Data->UsesTelegraph())
	{
		return;
	}

	// 몽타주에 예비 동작 구간(ANS_Telegraph)이 있으면 그 구간이 감속과 번쩍임을 맡는다. 캐릭터는 이 비율만큼 따른다
	const float WindowStrength = bComboContinuation ? Data->GetTelegraphComboStrength() : 1.f;
	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (const UANS_Telegraph* Window = Cast<UANS_Telegraph>(Event.NotifyStateClass))
		{
			TelegraphIntervals.Add({ Event.GetTriggerTime(), Event.GetEndTriggerTime(), FMath::Lerp(1.f, Window->TimeScale, WindowStrength) });
		}
	}
	if (TelegraphIntervals.Num() > 0)
	{
		Character->SetTelegraphWindowStrength(WindowStrength);
		return;
	}
	if (Scale >= 1.f)
	{
		return;
	}

	// 구간이 없으면 예전 방식: 몽타주 시작 ~ 첫 판정 구간 전체 (타이밍은 애니메이션, 규칙 3). 판정 구간이 없으면 감속하지 않는다
	float FirstHitTime = TNumericLimits<float>::Max();
	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Cast<UANS_HitWindow>(Event.NotifyStateClass))
		{
			FirstHitTime = FMath::Min(FirstHitTime, Event.GetTriggerTime());
		}
	}
	if (FirstHitTime == TNumericLimits<float>::Max() || FirstHitTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	TelegraphScale = Scale;
	TelegraphIntervals.Add({ 0.f, FirstHitTime, Scale });
	Character->SetTelegraphTimeScale(Scale);
	TraceBeganHandle = MeleeTrace->OnTraceBegan.AddUObject(this, &UGA_MeleeAttack::EndTelegraph);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ExecuteGameplayCue(CombatTags::GameplayCue_Attack_Telegraph, ASC->MakeEffectContext());
	}
	UE_LOG(LogFirstAction, Verbose, TEXT("[%s] 예비 동작: %s 배율 %.2f, 판정까지 %.2f초(몽타주)"),
		*GetNameSafe(Character), *GetNameSafe(Montage), Scale, FirstHitTime);
}

void UGA_MeleeAttack::EndTelegraph()
{
	ACombatCharacterBase* Character = GetCombatCharacter();
	if (TraceBeganHandle.IsValid())
	{
		if (UMeleeTraceComponent* MeleeTrace = Character ? Character->GetMeleeTrace() : nullptr)
		{
			MeleeTrace->OnTraceBegan.Remove(TraceBeganHandle);
		}
		TraceBeganHandle.Reset();
	}
	if (Character)
	{
		if (TelegraphScale < 1.f)
		{
			Character->SetTelegraphTimeScale(1.f);
		}
		// 몽타주가 바뀌거나 끝나면 구간 감속도 끝낸다(다음 몽타주에서 다시 정한다)
		Character->EndTelegraphWindow();
		Character->SetTelegraphWindowStrength(0.f);
	}
	TelegraphScale = 1.f;
	TelegraphIntervals.Reset();
}

void UGA_MeleeAttack::StartApproach(float StopGap, float Duration)
{
	ACombatCharacterBase* Character = GetCombatCharacter();
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	static const FName ApproachSourceName(TEXT("AttackApproach"));
	Movement->RemoveRootMotionSource(ApproachSourceName);

	FVector Goal;
	FVector Direction;
	float MoveDistance = 0.f;
	AActor* Target = nullptr;
	if (!ComputeWarpGoal(StopGap, Goal, Direction, MoveDistance, Target))
	{
		return;
	}

	// 방향은 바로 돌린다 (워핑의 회전과 같은 역할)
	Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	if (MoveDistance < 1.f)
	{
		return;
	}

	Character->ApplyTimedMove(ApproachSourceName, Goal, Duration);

	// 구간 동안 대상이 움직여도(밀려나는 중 등) 목표를 다시 계산해 따라 붙는다
	ApproachStopGap = StopGap;
	// 이동은 캐릭터 시간으로 흐른다(예비 동작 감속 중이면 더 오래). 추적은 이동이 끝나면(Root Motion Source가 없으면) 먼저 멈춘다
	ApproachEndTime = Character->GetWorld()->GetTimeSeconds() + Duration / FMath::Max(Character->CustomTimeDilation, 0.05f);
	Character->GetWorld()->GetTimerManager().SetTimer(ApproachTrackTimer, this, &UGA_MeleeAttack::TrackApproach, 1.f / 60.f, true);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 접근: %s %.0fcm / %.2f초 (제자리 동작, Root Motion Source, 대상 추적)"),
		*GetNameSafe(Character), *GetNameSafe(CurrentAttack), MoveDistance, Duration);
}

void UGA_MeleeAttack::TrackApproach()
{
	static const FName ApproachSourceName(TEXT("AttackApproach"));
	ACombatCharacterBase* Character = GetCombatCharacter();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	FVector Goal;
	FVector Direction;
	float MoveDistance = 0.f;
	AActor* Target = nullptr;
	if (!World || World->GetTimeSeconds() >= ApproachEndTime
		|| !ComputeWarpGoal(ApproachStopGap, Goal, Direction, MoveDistance, Target, false)
		|| !Character->UpdateTimedMoveTarget(ApproachSourceName, Goal))
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(ApproachTrackTimer);
		}
		return;
	}
	Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
}

void UGA_MeleeAttack::ClearApproaches()
{
	if (ACombatCharacterBase* Character = GetCombatCharacter())
	{
		if (UWorld* World = Character->GetWorld())
		{
			for (FTimerHandle& Handle : ApproachTimers)
			{
				World->GetTimerManager().ClearTimer(Handle);
			}
			World->GetTimerManager().ClearTimer(ApproachTrackTimer);
		}
	}
	ApproachTimers.Reset();
}

void UGA_MeleeAttack::UpdateWarpTarget(float StopGap)
{
	ACombatCharacterBase* Character = GetCombatCharacter();
	UMotionWarpingComponent* MotionWarping = Character ? Character->GetMotionWarping() : nullptr;
	if (!MotionWarping || !CurrentAttack)
	{
		return;
	}

	// 이전 타의 목표가 남지 않도록 먼저 지운다. 목표가 없으면 Motion Warping 구간은 아무것도 고치지 않는다
	MotionWarping->RemoveWarpTarget(WarpTargetName);

	FVector WarpLocation;
	FVector Direction;
	float MoveDistance = 0.f;
	AActor* Target = nullptr;
	if (!ComputeWarpGoal(StopGap, WarpLocation, Direction, MoveDistance, Target))
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 워핑: %s 대상 없음 (원래 루트 모션)"), *GetNameSafe(Character), *GetNameSafe(CurrentAttack));
		return;
	}
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, Direction.Rotation());
}

void UGA_MeleeAttack::OnMotionWarpingPreUpdate(UMotionWarpingComponent* MotionWarpingComp)
{
	if (!IsActive() || !MotionWarpingComp)
	{
		return;
	}

	for (URootMotionModifier* Modifier : MotionWarpingComp->GetModifiers())
	{
		// 이 공격의 목표 이름을 쓰는 구간이 막 생겨 아직 활성화 전(Waiting)이면, 휘두름이 시작되는 순간이다
		const URootMotionModifier_Warp* WarpModifier = Cast<URootMotionModifier_Warp>(Modifier);
		if (WarpModifier && WarpModifier->WarpTargetName == WarpTargetName
			&& WarpModifier->GetState() == ERootMotionModifierState::Waiting
			&& !RefreshedWarpModifiers.Contains(Modifier))
		{
			RefreshedWarpModifiers.Add(Modifier);
			UpdateWarpTarget(FindStopGap(WarpModifier));
			return;
		}
	}
}

void UGA_MeleeAttack::TryAdvanceCombo()
{
	if (!IsActive() || !CurrentAttack || CurrentAttack->ComboLinks.IsEmpty())
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	ACombatCharacterBase* Character = GetCombatCharacter();
	if (!ASC || !Character || !ASC->HasMatchingGameplayTag(CombatTags::Window_Combo))
	{
		return;
	}

	// 이 공격의 ComboLinks에 있는 입력만 받는다 (회피 입력 등은 버퍼에 남는다)
	FGameplayTagContainer AcceptedInputTags;
	for (const FAttackComboLink& Link : CurrentAttack->ComboLinks)
	{
		if (Link.InputTag.IsValid() && Link.NextAttack)
		{
			AcceptedInputTags.AddTag(Link.InputTag);
		}
	}

	FGameplayTag PressedTag;
	if (!Character->ConsumeBufferedInput(AcceptedInputTags, PressedTag))
	{
		return;
	}

	const FAttackComboLink* ChosenLink = CurrentAttack->ComboLinks.FindByPredicate(
		[&PressedTag](const FAttackComboLink& Link) { return Link.InputTag == PressedTag; });
	if (!ChosenLink || !ChosenLink->NextAttack)
	{
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 콤보: %s --%s--> %s"),
		*GetNameSafe(Character), *GetNameSafe(CurrentAttack), *PressedTag.ToString(), *GetNameSafe(ChosenLink->NextAttack));

	CurrentAttack = ChosenLink->NextAttack;
	bComboContinuation = true;
	PlayCurrentAttackMontage();
}

void UGA_MeleeAttack::OnHitEvent(FGameplayEventData Payload)
{
	if (!IsActive() || !CurrentAttack)
	{
		return;
	}

	AActor* Target = const_cast<AActor*>(Payload.Target.Get());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC)
	{
		// ASC가 없는 대상(벽에 붙은 장식 Pawn 등)은 피해를 받지 않는다
		return;
	}

	if (!DamageEffect)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_MeleeAttack: DamageEffect가 지정되지 않았습니다. BP_GA_MeleeAttack 기본값을 확인하세요."),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		return;
	}

	UHitResolveSubsystem* Resolver = GetWorld() ? GetWorld()->GetSubsystem<UHitResolveSubsystem>() : nullptr;
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	if (!Resolver || !SpecHandle.IsValid())
	{
		return;
	}

	// --- 타격 해석 (Step 6-4). 어빌리티는 공격 데이터의 값과 판정 결과를 넘기기만 한다 (규칙 6) ---
	const FHitResult* Hit = Payload.TargetData.Get(0) ? Payload.TargetData.Get(0)->GetHitResult() : nullptr;

	FHitResolveInput Input;
	Input.Attacker = GetAvatarActorFromActorInfo();
	Input.Victim = Target;
	Input.Hit = Hit ? *Hit : FHitResult();
	Input.ImpactForce = CurrentAttack->ImpactForce;
	Input.PostHitSpacing = CurrentAttack->PostHitSpacing;
	Input.Damage = CurrentAttack->Damage;
	Input.HitStopDuration = CurrentAttack->HitStopDuration;
	Input.MoveDuration = CurrentAttack->PostHitMoveDuration;
	Input.AttackBone = CurrentAttack->AttackBone;

	const FHitResolution Resolution = Resolver->Resolve(Input);

	// 무적인 몸을 맞힘 (6-9): 그 휘두름의 맞힘은 소진되지만, 헛친 것처럼 피해·히트스톱·이동·반응·연출을 모두 건너뛴다
	// 저스트 회피당함 (6-13): 때린 쪽은 일반 회피처럼 헛친다. 회피한 쪽에는 결과(슬로모션 보상)와 전용 Cue를 보낸다
	if (Resolution.Outcome == EHitOutcome::JustDodged)
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 저스트 회피당함: %s → %s (헛친 것으로 처리)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack), *GetNameSafe(Target));

		FGameplayEventData DodgerPayload;
		DodgerPayload.EventTag = CombatTags::Event_HitReact;
		DodgerPayload.Instigator = GetAvatarActorFromActorInfo();
		DodgerPayload.Target = Target;
		DodgerPayload.TargetData.Add(new FGameplayAbilityTargetData_HitResolution(Resolution));
		if (Hit)
		{
			SpecHandle.Data->GetContext().AddHitResult(*Hit, true);
		}
		DodgerPayload.ContextHandle = SpecHandle.Data->GetContext();
		TargetASC->HandleGameplayEvent(DodgerPayload.EventTag, &DodgerPayload);
		TargetASC->ExecuteGameplayCue(CombatTags::GameplayCue_Hit_JustDodged, SpecHandle.Data->GetContext());
		return;
	}

	if (Resolution.Outcome == EHitOutcome::Evaded)
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 회피됨: %s → %s (헛친 것으로 처리)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack), *GetNameSafe(Target));
		return;
	}

	// 패리당함 (6-12): 방어자에게는 결과(긴 히트스톱, 가드의 Parry 구간)를, 자신에게는 되돌린 몫을 각자의 Event.HitReact로 보낸다
	// 자기 반응(경직)이 이 공격의 몽타주를 끊어 어빌리티가 끝날 수 있으므로 마지막에 처리하고 바로 빠진다
	if (Resolution.Outcome == EHitOutcome::Parried)
	{
		if (Hit)
		{
			SpecHandle.Data->GetContext().AddHitResult(*Hit, true);
		}
		HandleParried(Input, Resolution, Resolver, SpecHandle, SourceASC, TargetASC);
		return;
	}

	// 피해와 경직치는 매니저가 정한 값. GE_Damage는 속성에 이 값을 더하므로 음수로 넘긴다 (규칙 5)
	SpecHandle.Data->SetSetByCallerMagnitude(CombatTags::SetByCaller_Damage, -Resolution.Damage);
	SpecHandle.Data->SetSetByCallerMagnitude(CombatTags::SetByCaller_PoiseDamage, -Resolution.PoiseDamage);

	// 판정 결과(맞은 위치, 본)를 이펙트 컨텍스트에 함께 싣는다. 연출 Cue가 맞은 위치로 쓴다 (6-1)
	if (Hit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*Hit, true);
	}

	// 피해 경로는 결과와 상관없이 GE_Damage 하나다 (규칙 6-4: 방어 전용 피해 경로 없음). 가드면 체력 값이 0으로 와 있다
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	// 맞은 쪽 연출은 결과에 따라 고른다 (6-11): 일반 타격은 Hit.Taken, 가드는 Hit.Guarded
	// (6-1에서는 GE_Damage의 Cue 목록에 Hit.Taken을 두었으나, 가드에도 같은 GE가 쓰이므로 여기로 옮겼다)
	const FGameplayTag TakenCue = Resolution.Outcome == EHitOutcome::Guarded ? CombatTags::GameplayCue_Hit_Guarded : CombatTags::GameplayCue_Hit_Taken;
	TargetASC->ExecuteGameplayCue(TakenCue, SpecHandle.Data->GetContext());

	UE_LOG(LogFirstAction, Log, TEXT("[%s] %s: %s → %s (체력 %.1f, 경직치 %.1f)"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), Resolution.Outcome == EHitOutcome::Guarded ? TEXT("가드됨") : TEXT("피해"),
		*GetNameSafe(CurrentAttack), *GetNameSafe(Target), Resolution.Damage, Resolution.PoiseDamage);

	// --- 결과 적용. 각자 자기 몫만 (규칙 6) ---

	// 때린 쪽: 자기 히트스톱과 자기 이동 (6-5). AttackerDisplacement는 맞은 쪽 반대 방향 기준(음수면 전진)
	if (ACombatCharacterBase* Character = GetCombatCharacter())
	{
		Character->ApplyHitStop(Resolution.AttackerHitStop);
		Character->ApplyHitDisplacement(-Resolution.Direction, Resolution.AttackerDisplacement, Resolution.MoveDuration);

		// 맞힘이나 가드됨: 공격이 닿았다는 사실만 남긴다 (7-4). 누가 읽는지(적 AI의 콤보 판단)는 모른다
		Character->NotifyAttackConnected();
	}

	// 맞은 쪽: 해석 결과 전체를 전용 타깃 데이터로 실어 보낸다 (6-4 결정). 받은 쪽이 자기 몫(히트스톱, 6-5 밀림, 6-7 반응)을 적용한다
	FGameplayEventData ReactPayload;
	ReactPayload.EventTag = CombatTags::Event_HitReact;
	ReactPayload.Instigator = GetAvatarActorFromActorInfo();
	ReactPayload.Target = Target;
	ReactPayload.TargetData.Add(new FGameplayAbilityTargetData_HitResolution(Resolution));
	ReactPayload.ContextHandle = SpecHandle.Data->GetContext();
	TargetASC->HandleGameplayEvent(ReactPayload.EventTag, &ReactPayload);

	// 때린 쪽 연출: 자기 ASC에 Cue를 발생시킨다. 카메라가 있으면(로컬 플레이어면) 흔들리는지는 Cue 에셋이 판단한다(규칙 1)
	// 컨텍스트에 판정 결과가 있으므로 Cue 쪽에서 맞은 위치를 쓸 수 있다
	SourceASC->ExecuteGameplayCue(CombatTags::GameplayCue_Hit_Dealt, SpecHandle.Data->GetContext());
}

void UGA_MeleeAttack::HandleParried(const FHitResolveInput& Input, const FHitResolution& Resolution, const UHitResolveSubsystem* Resolver,
	const FGameplayEffectSpecHandle& SpecHandle, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC)
{
	AActor* Self = Input.Attacker;
	const FGameplayEffectContextHandle Context = SpecHandle.Data->GetContext();

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 패리당함: %s → %s"), *GetNameSafe(Self), *GetNameSafe(CurrentAttack), *GetNameSafe(Input.Victim));

	// 방어자: 피해 없음. 긴 히트스톱은 캐릭터가, Parry 구간 이동은 방어자의 GA_Guard가 이 이벤트로 적용한다 (규칙 6)
	FGameplayEventData DefenderPayload;
	DefenderPayload.EventTag = CombatTags::Event_HitReact;
	DefenderPayload.Instigator = Self;
	DefenderPayload.Target = Input.Victim;
	DefenderPayload.TargetData.Add(new FGameplayAbilityTargetData_HitResolution(Resolution));
	DefenderPayload.ContextHandle = Context;
	TargetASC->HandleGameplayEvent(DefenderPayload.EventTag, &DefenderPayload);

	// 연출: 전용 Cue를 양쪽에 (6-12 결정). 카메라 흔들림은 로컬 플레이어 쪽에서만 난다
	TargetASC->ExecuteGameplayCue(CombatTags::GameplayCue_Hit_Parried, Context);
	SourceASC->ExecuteGameplayCue(CombatTags::GameplayCue_Hit_Parried, Context);

	// 자신(공격자): 되돌린 몫. 경직치는 같은 피해 경로(GE_Damage)로 자신에게, 반응·부위 반응·히트스톱은 자기 Event.HitReact로
	const FHitResolution Recoil = Resolver->ResolveParryRecoil(Input, Resolution);

	FGameplayEffectSpecHandle RecoilSpec = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	if (RecoilSpec.IsValid())
	{
		RecoilSpec.Data->SetSetByCallerMagnitude(CombatTags::SetByCaller_Damage, 0.f);
		RecoilSpec.Data->SetSetByCallerMagnitude(CombatTags::SetByCaller_PoiseDamage, -Recoil.PoiseDamage);
		SourceASC->ApplyGameplayEffectSpecToSelf(*RecoilSpec.Data.Get());
	}

	FGameplayEventData RecoilPayload;
	RecoilPayload.EventTag = CombatTags::Event_HitReact;
	RecoilPayload.Instigator = Input.Victim;
	RecoilPayload.Target = Self;
	RecoilPayload.TargetData.Add(new FGameplayAbilityTargetData_HitResolution(Recoil));
	RecoilPayload.ContextHandle = Context;
	// 이 호출 안에서 반응 몽타주가 공격 몽타주를 끊어 이 어빌리티가 끝날 수 있다. 이 뒤로는 아무것도 하지 않는다
	SourceASC->HandleGameplayEvent(RecoilPayload.EventTag, &RecoilPayload);
}

void UGA_MeleeAttack::OnComboWindowOpened()
{
	TryAdvanceCombo();
}

void UGA_MeleeAttack::OnInputBuffered(const FGameplayTag& InputTag)
{
	TryAdvanceCombo();
}

void UGA_MeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsActive())
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] GA_MeleeAttack 종료: %s (%s)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack), bWasCancelled ? TEXT("취소") : TEXT("완료"));
	}

	if (InputBufferedHandle.IsValid())
	{
		if (ACombatCharacterBase* Character = GetCombatCharacter())
		{
			Character->OnInputBuffered.Remove(InputBufferedHandle);
		}
		InputBufferedHandle.Reset();
	}

	// 다음 활성화에 지난 공격의 워핑 목표가 남지 않도록
	if (ACombatCharacterBase* Character = GetCombatCharacter())
	{
		if (UMotionWarpingComponent* MotionWarping = Character->GetMotionWarping())
		{
			MotionWarping->RemoveWarpTarget(WarpTargetName);
			MotionWarping->OnPreUpdate.RemoveDynamic(this, &UGA_MeleeAttack::OnMotionWarpingPreUpdate);
		}
	}
	RefreshedWarpModifiers.Reset();
	ClearApproaches();
	EndTelegraph();

	CurrentAttack = nullptr;
	MontageTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MeleeAttack::OnMontageFinished()
{
	// 블렌드 아웃과 완료가 둘 다 오지만, 두 번째 EndAbility는 이미 끝난 상태라 무시된다
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_MeleeAttack::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
