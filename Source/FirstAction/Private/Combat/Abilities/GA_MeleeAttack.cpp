#include "Combat/Abilities/GA_MeleeAttack.h"

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

	// 경직(그로기, 넘어짐 포함) 중에는 공격을 시작하지 못한다 (6-7). 움찔은 태그가 없어 공격으로 끊고 나갈 수 있다
	ActivationBlockedTags.AddTag(CombatTags::State_Staggered);
}

const UAttackDataAsset* UGA_MeleeAttack::ResolveStartAttack() const
{
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
	CurrentAttack = ResolveStartAttack();
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
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentAttack->Montage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_MeleeAttack::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

float UGA_MeleeAttack::FindStopGap(const URootMotionModifier* Modifier)
{
	// 워핑 구간이 시작되는 시점을 덮는 Attack Reach 노티파이를 같은 애니메이션(몽타주)에서 찾는다
	const UAnimSequenceBase* Animation = Modifier ? Modifier->Animation.Get() : nullptr;
	if (!Animation)
	{
		return 0.f;
	}

	for (const FAnimNotifyEvent& Event : Animation->Notifies)
	{
		const UANS_AttackReach* Reach = Cast<UANS_AttackReach>(Event.NotifyStateClass);
		if (Reach && Event.GetTriggerTime() <= Modifier->StartTime + KINDA_SMALL_NUMBER
			&& Modifier->StartTime < Event.GetEndTriggerTime())
		{
			return Reach->StopGap;
		}
	}
	return 0.f;
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

	const float MaxWarpDistance = CurrentAttack->MaxWarpDistance;
	AActor* Target = MaxWarpDistance > 0.f ? Character->FindAttackTarget(MaxWarpDistance) : nullptr;
	if (!Target)
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 워핑: %s 대상 없음 (원래 루트 모션)"), *GetNameSafe(Character), *GetNameSafe(CurrentAttack));
		return;
	}

	const FVector From = Character->GetActorLocation();
	const FVector ToTarget = (Target->GetActorLocation() - From) * FVector(1.f, 1.f, 0.f);
	const FVector Direction = ToTarget.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	// 몸 간격이 StopGap이 되는 곳까지만, 그리고 최대 거리까지만 (6-2 결정 + 휘두름별 멈출 간격)
	// StopGap은 몽타주의 Attack Reach 노티파이 값, 없으면 0(몸이 닿는 거리). 이미 그보다 가까우면 이동 0, 방향만 돌린다
	const float ContactSpacing = Character->GetSimpleCollisionRadius() + Target->GetSimpleCollisionRadius();
	const float MoveDistance = FMath::Clamp(ToTarget.Size() - ContactSpacing - StopGap, 0.f, MaxWarpDistance);
	const FVector WarpLocation = From + Direction * MoveDistance;

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, Direction.Rotation());

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 워핑: %s → %s (간격 %.0f, 멈출 간격 %.0f, 이동 %.0f / 최대 %.0f)"),
		*GetNameSafe(Character), *GetNameSafe(CurrentAttack), *GetNameSafe(Target),
		ToTarget.Size() - ContactSpacing, StopGap, MoveDistance, MaxWarpDistance);

	if (CVarShowWarpTarget.GetValueOnGameThread())
	{
		UWorld* World = Character->GetWorld();
		DrawDebugLine(World, From, Target->GetActorLocation(), FColor::Orange, false, 1.5f, 0, 1.5f);
		DrawDebugSphere(World, WarpLocation, 15.f, 12, FColor::Green, false, 1.5f);
		DrawDebugDirectionalArrow(World, WarpLocation, WarpLocation + Direction * 60.f, 20.f, FColor::Green, false, 1.5f, 0, 2.f);
	}
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

	const FHitResolution Resolution = Resolver->Resolve(Input);

	// 피해와 경직치는 매니저가 정한 값. GE_Damage는 속성에 이 값을 더하므로 음수로 넘긴다 (규칙 5)
	SpecHandle.Data->SetSetByCallerMagnitude(CombatTags::SetByCaller_Damage, -Resolution.Damage);
	SpecHandle.Data->SetSetByCallerMagnitude(CombatTags::SetByCaller_PoiseDamage, -Resolution.PoiseDamage);

	// 판정 결과(맞은 위치, 본)를 이펙트 컨텍스트에 함께 싣는다. 연출 Cue가 맞은 위치로 쓴다 (6-1)
	if (Hit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*Hit, true);
	}

	// GE_Damage의 GameplayCues 목록에 있는 GameplayCue.Hit.Taken도 이때 맞은 쪽에서 발생한다 (Step 6-1)
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 피해: %s → %s (체력 %.1f, 경직치 %.1f)"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(CurrentAttack), *GetNameSafe(Target),
		Resolution.Damage, Resolution.PoiseDamage);

	// --- 결과 적용. 각자 자기 몫만 (규칙 6) ---

	// 때린 쪽: 자기 히트스톱과 자기 이동 (6-5). AttackerDisplacement는 맞은 쪽 반대 방향 기준(음수면 전진)
	if (ACombatCharacterBase* Character = GetCombatCharacter())
	{
		Character->ApplyHitStop(Resolution.AttackerHitStop);
		Character->ApplyHitDisplacement(-Resolution.Direction, Resolution.AttackerDisplacement, Resolution.MoveDuration);
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
