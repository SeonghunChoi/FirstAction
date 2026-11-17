#include "Combat/PartialHitReactionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatTags.h"
#include "Components/SkeletalMeshComponent.h"
#include "FirstAction.h"

namespace
{
	// 캐릭터 데이터가 없을 때의 회복 시간. 캐릭터 데이터 기본값과 같다
	constexpr float FallbackRecoverTime = 0.4f;
}

UPartialHitReactionComponent::UPartialHitReactionComponent()
{
	// 흔들리는 부위가 있을 때만 틱한다
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// FirstVR에서 쓰던 값에서 시작한다. 6-15에서 조정
	PhysicalAnimationData.bIsLocalSimulation = false;
	PhysicalAnimationData.OrientationStrength = 1000.f;
	PhysicalAnimationData.AngularVelocityStrength = 100.f;
	PhysicalAnimationData.PositionStrength = 1000.f;
	PhysicalAnimationData.VelocityStrength = 100.f;

	BlockedTags.AddTag(CombatTags::State_Downed);
	BlockedTags.AddTag(CombatTags::State_Guarding);
}

void UPartialHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	Mesh = Character ? Character->GetMesh() : nullptr;
	PhysicalAnimation = GetOwner()->FindComponentByClass<UPhysicalAnimationComponent>();
	if (!Mesh || !PhysicalAnimation)
	{
		UE_LOG(LogFirstAction, Error, TEXT("[%s] 부위 반응에는 캐릭터 메시와 PhysicalAnimationComponent가 필요합니다"), *GetNameSafe(GetOwner()));
		return;
	}

	PhysicalAnimation->SetSkeletalMeshComponent(Mesh);

	// 상체(spine_01 아래)가 물리로 돌 때 메시 컴포넌트가 바디를 따라가면, 월드 기준 목표 자세도 같이 움직여 돌아오지 않는다 (FirstVR)
	Mesh->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;

	// 메시가 물리 시뮬레이션을 하려면 충돌이 물리까지 켜져 있어야 한다.
	// 프로필(CharacterMesh)은 바꾸지 않는다: 오브젝트 타입이 Pawn으로 남아야 판정 스윕(5-1)이 메시를 맞히고,
	// 이 프로필은 Pawn 채널을 무시하므로 흔들리는 바디가 자기 캡슐이나 다른 캐릭터와 부딪히지 않는다
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void UPartialHitReactionComponent::ApplyPartialHit(FName HitBone, const FVector& Velocity)
{
	if (!Mesh || !PhysicalAnimation || HitBone.IsNone() || Velocity.IsNearlyZero())
	{
		return;
	}

	const ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetOwner());
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;

	if (const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (ASC->HasAnyMatchingGameplayTags(BlockedTags))
		{
			UE_LOG(LogFirstAction, Log, TEXT("[%s] 부위 반응 막힘 (%s)"), *GetNameSafe(GetOwner()), *HitBone.ToString());
			return;
		}
	}

	FName RootBone = Data ? Data->FindPartialReactionRootBone(Mesh, HitBone) : HitBone;
	if (RootBone.IsNone())
	{
		return;
	}

	// 이미 흔들리는 더 큰 부위 안이면 그 부위를 처음부터 다시 흔든다 (예: 상체가 흔들리는 중에 머리를 맞음)
	for (const TPair<FName, float>& Reaction : ActiveReactions)
	{
		if (Reaction.Key == RootBone || Mesh->BoneIsChildOf(RootBone, Reaction.Key))
		{
			RootBone = Reaction.Key;
			break;
		}
	}
	// 새 부위 안에 들어가는 작은 반응은 새 반응이 이어받는다
	for (auto It = ActiveReactions.CreateIterator(); It; ++It)
	{
		if (Mesh->BoneIsChildOf(It.Key(), RootBone))
		{
			It.RemoveCurrent();
		}
	}

	PhysicalAnimation->ApplyPhysicalAnimationSettingsBelow(RootBone, PhysicalAnimationData);
	Mesh->SetAllBodiesBelowSimulatePhysics(RootBone, true);
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(RootBone, 1.f);

	// 맞은 바디에 속도 변화를 준다(bVelChange: 바디 질량과 무관). 부위의 나머지는 관절로 끌려간다
	Mesh->AddImpulse(Velocity, HitBone, true);

	ActiveReactions.Add(RootBone, 0.f);
	SetComponentTickEnabled(true);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 부위 반응: 맞은 본 %s → 부위 %s, 속도 변화 %.0f"),
		*GetNameSafe(GetOwner()), *HitBone.ToString(), *RootBone.ToString(), Velocity.Size());
}

void UPartialHitReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 회복 시간은 몸마다 (6-8 결정). 히트스톱 중에는 DeltaTime이 줄어 회복도 늦춰진다
	const ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(GetOwner());
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;
	const float RecoverTime = FMath::Max(Data ? Data->GetPartialReactionRecoverTime() : FallbackRecoverTime, KINDA_SMALL_NUMBER);

	TArray<FName> Finished;
	for (TPair<FName, float>& Reaction : ActiveReactions)
	{
		Reaction.Value += DeltaTime;
		const float Weight = 1.f - FMath::Clamp(Reaction.Value / RecoverTime, 0.f, 1.f);
		Mesh->SetAllBodiesBelowPhysicsBlendWeight(Reaction.Key, Weight);
		if (Weight <= 0.f)
		{
			Finished.Add(Reaction.Key);
		}
	}

	for (const FName& RootBone : Finished)
	{
		StopReaction(RootBone);
	}

	if (ActiveReactions.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

void UPartialHitReactionComponent::StopReaction(FName RootBone)
{
	Mesh->SetAllBodiesBelowSimulatePhysics(RootBone, false);
	ActiveReactions.Remove(RootBone);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 부위 반응 회복: %s"), *GetNameSafe(GetOwner()), *RootBone.ToString());
}
