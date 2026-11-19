#include "Combat/Abilities/GA_Jump.h"

#include "AbilitySystemComponent.h"
#include "Combat/CombatTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "FirstAction.h"

namespace
{
	// 점프를 요청한 뒤 이 시간 안에 공중으로 뜨지 않으면 실패로 보고 끝낸다
	constexpr float LeftGroundCheckDelay = 0.2f;
}

UGA_Jump::UGA_Jump()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationBlockedTags.AddTag(CombatTags::State_Staggered);
	ActivationBlockedTags.AddTag(CombatTags::State_Dead);
	ActivationBlockedTags.AddTag(CombatTags::State_InAir);

	// 공격·회피·움찔·가드 중이면 그 몽타주의 캔슬 구간에서만 시작한다 (회피와 같은 규칙)
	CancelWindowRequiredTags.AddTag(CombatTags::State_Attacking);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Dodging);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Flinching);
	CancelWindowRequiredTags.AddTag(CombatTags::State_Guarding);
}

void UGA_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->CanJump())
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 점프 못함 (엔진 CanJump 거부)"), *GetNameSafe(Character));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 캔슬 구간에서 점프로 끊으면 그 행동의 몽타주를 멈춘다. 루트 모션이 점프를 끌고 가지 않게 한다
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->GetCurrentMontage())
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 점프로 몽타주 끊음: %s"), *Character->GetName(), *GetNameSafe(ASC->GetCurrentMontage()));
		ASC->CurrentMontageStop();
	}

	Character->LandedDelegate.AddDynamic(this, &UGA_Jump::OnLanded);
	Character->Jump();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(LeftGroundCheckTimer, this, &UGA_Jump::CheckLeftGround, LeftGroundCheckDelay, false);
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 점프 시작"), *Character->GetName());
}

void UGA_Jump::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->StopJumping();
	}
}

void UGA_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LeftGroundCheckTimer);
	}

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->LandedDelegate.RemoveDynamic(this, &UGA_Jump::OnLanded);
		Character->StopJumping();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Jump::OnLanded(const FHitResult& Hit)
{
	UE_LOG(LogFirstAction, Log, TEXT("[%s] 점프 끝 (착지)"), *GetNameSafe(GetAvatarActorFromActorInfo()));
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Jump::CheckLeftGround()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (Movement && Movement->IsFalling())
	{
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 점프 끝 (땅을 떠나지 못함)"), *GetNameSafe(Character));
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
