#include "Combat/Abilities/GA_Dodge.h"
#include "Content/CombatContentSubsystem.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatTags.h"
#include "FirstAction.h"
#include "AnimNotifyState_MotionWarping.h"
#include "TimerManager.h"

UGA_Dodge::UGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 회피 중 다시 회피(연속 구르기): 캔슬 구간에서 다시 누르면 지금 회피를 끝내고 처음부터 (6-9 결정)
	bRetriggerInstancedAbility = true;

	ActivationOwnedTags.AddTag(CombatTags::State_Dodging);

	// 경직(넘어짐, 그로기 포함) 중에는 회피할 수 없다
	ActivationBlockedTags.AddTag(CombatTags::State_Staggered);
	ActivationBlockedTags.AddTag(CombatTags::State_Dead);

	// 공중에서는 회피하지 못한다. 누른 입력은 착지하는 순간 다시 시도된다 (2026-11-19 결정)
	ActivationBlockedTags.AddTag(CombatTags::State_InAir);

	// 공격·회피·움찔·가드 중이면 그 몽타주의 캔슬 구간에서만 시작한다 (6-9, 6-11 결정)
	CancelWindowRequiredTags.AddTag(CombatTags::State_Guarding);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Attacking);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Dodging);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Flinching);
}

void UGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;
	const FVector Direction = Character ? Character->GetDodgeDirection().GetSafeNormal2D() : FVector::ZeroVector;

	UAnimMontage* Chosen = Data ? Data->GetDodgeMontage() : nullptr;
	float LocalYaw = 0.f;
	if (Data && Data->HasDirectionalDodge() && !Direction.IsNearlyZero())
	{
		// 방향별 회피 (9-3): 몸은 그대로, 입력이 몸 정면 기준 어느 쪽인지로 몽타주를 고른다
		const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
		LocalYaw = FMath::RadiansToDegrees(FMath::Atan2(FVector::CrossProduct(Forward, Direction).Z, FVector::DotProduct(Forward, Direction)));
		Chosen = Data->FindDodgeMontage(LocalYaw);
	}
	else if (!Direction.IsNearlyZero())
	{
		// 회피 방향으로 몸을 즉시 돌린다. 몽타주는 앞구르기 하나라 몸이 향한 쪽으로 구른다 (6-9 결정)
		Character->SetActorRotation(Direction.Rotation());
	}

	UAnimMontage* Montage = UCombatContentSubsystem::ResolveMontage(this, Chosen);
	if (!Montage)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] GA_Dodge: 캐릭터 데이터에 Dodge Montage가 없습니다"), *GetNameSafe(Character));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 회피 시작: %s, 방향 %s (몸 기준 %.0f도)"),
		*GetNameSafe(Character), *GetNameSafe(Montage), *Direction.ToCompactString(), LocalYaw);

	// 같은 슬롯의 공격·움찔 몽타주는 여기서 끊기고, 그 어빌리티는 자기 몽타주 취소 콜백으로 끝난다
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Dodge::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Dodge::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Dodge::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// 루트 모션이 없는 회피 몽타주는 코드로 옮긴다 (9-3). 방향은 입력, 없으면 몸 정면
	if (Data && Data->GetDodgeDistance() > 0.f && !Montage->HasRootMotion())
	{
		ScheduleMove(Montage, Direction.IsNearlyZero() ? Character->GetActorForwardVector().GetSafeNormal2D() : Direction, Data->GetDodgeDistance());
	}
}

void UGA_Dodge::ScheduleMove(const UAnimMontage* Montage, const FVector& Direction, float Distance)
{
	ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetAvatarActorFromActorInfo());
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	// 이동 구간 = 몽타주의 Motion Warping 구간 (공격 접근과 같은 약속). 없으면 몽타주 앞 절반
	const float Rate = FMath::Max(Montage->RateScale, KINDA_SMALL_NUMBER);
	float StartTime = 0.f;
	float Duration = Montage->GetPlayLength() * 0.5f;
	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Cast<UAnimNotifyState_MotionWarping>(Event.NotifyStateClass))
		{
			StartTime = Event.GetTriggerTime();
			Duration = Event.GetDuration();
			break;
		}
	}

	static const FName DodgeMoveSourceName(TEXT("DodgeMove"));
	TWeakObjectPtr<ACombatCharacterBase> WeakCharacter = Character;
	auto Start = [WeakCharacter, Direction, Distance, Duration = Duration / Rate]()
	{
		if (ACombatCharacterBase* C = WeakCharacter.Get())
		{
			C->ApplyTimedMove(DodgeMoveSourceName, C->GetActorLocation() + Direction * Distance, Duration);
			UE_LOG(LogFirstAction, Log, TEXT("[%s] 회피 이동 %.0fcm / %.2f초 (Root Motion Source)"), *C->GetName(), Distance, Duration);
		}
	};

	World->GetTimerManager().ClearTimer(MoveTimer);
	if (StartTime <= KINDA_SMALL_NUMBER)
	{
		Start();
	}
	else
	{
		World->GetTimerManager().SetTimer(MoveTimer, FTimerDelegate::CreateWeakLambda(this, Start), StartTime / Rate, false);
	}
}

void UGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsActive())
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 회피 종료 (%s)"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), bWasCancelled ? TEXT("끊김") : TEXT("완료"));
	}

	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UWorld* World = Avatar->GetWorld())
		{
			World->GetTimerManager().ClearTimer(MoveTimer);
		}
	}
	MontageTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Dodge::OnMontageFinished()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Dodge::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
