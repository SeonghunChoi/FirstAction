#include "Combat/CombatCharacterData.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

UAnimMontage* UCombatCharacterData::FindDodgeMontage(float LocalYawDegrees) const
{
	UAnimMontage* Chosen = nullptr;
	const float AbsYaw = FMath::Abs(LocalYawDegrees);
	if (AbsYaw >= 135.f)
	{
		Chosen = DodgeBackMontage;
	}
	else if (AbsYaw > 45.f)
	{
		Chosen = LocalYawDegrees > 0.f ? DodgeRightMontage : DodgeLeftMontage;
	}
	return Chosen ? Chosen : DodgeMontage.Get();
}

#if WITH_EDITOR
#include "Combat/CombatDataValidation.h"
#include "Combat/CombatTags.h"
#include "Misc/DataValidation.h"
#endif

UCombatCharacterData::UCombatCharacterData()
{
	// 마네킹 기준 부위 대표 본. 새로 만든 데이터 에셋과 이 칸을 저장한 적 없는 기존 에셋은 이 값을 쓴다
	PartialReactionRootBones = {
		TEXT("upperarm_l"), TEXT("upperarm_r"),
		TEXT("thigh_l"), TEXT("thigh_r"),
		TEXT("neck_01"),
		TEXT("spine_01")
	};
}

FName UCombatCharacterData::FindPartialReactionRootBone(const USkeletalMeshComponent* Mesh, FName HitBone) const
{
	if (!Mesh || HitBone.IsNone())
	{
		return NAME_None;
	}

	// 맞은 본 자신부터 부모 쪽으로 올라가며 처음 만나는 대표 본 = 가장 가까운 부위
	// (손 → 아래팔 → upperarm_l에서 멈춤. spine_01보다 upperarm이 먼저 걸리므로 팔은 팔만 흔들린다)
	for (FName Bone = HitBone; !Bone.IsNone(); Bone = Mesh->GetParentBone(Bone))
	{
		if (PartialReactionRootBones.Contains(Bone))
		{
			return Bone;
		}
	}
	return PartialReactionFallbackBone;
}

UAnimMontage* UCombatCharacterData::FindReactionMontage(EHitReactionStage Stage, const USkeletalMeshComponent* Mesh, FName HitBone) const
{
	// BoneIsChildOf는 자기 자신을 자식으로 치지 않으므로 같은 본도 따로 본다
	const bool bHeadHit = Mesh && !HitBone.IsNone() && !HeadRootBone.IsNone()
		&& (HitBone == HeadRootBone || Mesh->BoneIsChildOf(HitBone, HeadRootBone));

	switch (Stage)
	{
	case EHitReactionStage::Flinch:
		return (bHeadHit && FlinchHeadMontage) ? FlinchHeadMontage.Get() : FlinchMontage.Get();
	case EHitReactionStage::Stagger:
		return (bHeadHit && StaggerHeadMontage) ? StaggerHeadMontage.Get() : StaggerMontage.Get();
	case EHitReactionStage::Knockdown:
		return KnockdownMontage.Get();
	case EHitReactionStage::Groggy:
		return GroggyMontage.Get();
	default:
		return nullptr;
	}
}

#if WITH_EDITOR
EDataValidationResult UCombatCharacterData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	const FString P = CombatDataValidation::Prefix(this);

	auto Error = [&](const FString& Message)
	{
		Context.AddError(FText::FromString(P + Message));
		Result = EDataValidationResult::Invalid;
	};
	auto Warning = [&](const FString& Message)
	{
		Context.AddWarning(FText::FromString(P + Message));
	};

	if (Mass <= 0.f)
	{
		Error(FString::Printf(TEXT("질량(Mass)이 %.1fkg입니다. 상대 충격량을 계산할 수 없습니다"), Mass));
	}
	if (AttackTokens < 1)
	{
		Error(FString::Printf(TEXT("공격 토큰(AttackTokens)이 %d개라 이 캐릭터를 아무도 공격할 수 없습니다"), AttackTokens));
	}

	// 비어 있으면 그 반응·행동이 일어나지 않는다. 반응하지 않는 몸일 수 있으므로 경고 (8-3 결정 B)
	// 머리 몽타주는 비면 몸통 몽타주를 쓰므로 보지 않는다
	const TPair<const TCHAR*, const UAnimMontage*> Slots[] = {
		{ TEXT("FlinchMontage(움찔)"), FlinchMontage.Get() },
		{ TEXT("StaggerMontage(경직)"), StaggerMontage.Get() },
		{ TEXT("KnockdownMontage(넘어짐)"), KnockdownMontage.Get() },
		{ TEXT("GroggyMontage(그로기)"), GroggyMontage.Get() },
		{ TEXT("DodgeMontage(회피)"), DodgeMontage.Get() },
		{ TEXT("GuardMontage(가드)"), GuardMontage.Get() },
		{ TEXT("DeathMontage(사망)"), DeathMontage.Get() },
	};
	for (const TPair<const TCHAR*, const UAnimMontage*>& Slot : Slots)
	{
		if (!Slot.Value)
		{
			Warning(FString::Printf(TEXT("%s가 비어 있습니다. 이 동작을 하지 않습니다"), Slot.Key));
		}
	}

	if (KnockdownMontage && !CombatDataValidation::HasTagWindow(KnockdownMontage, CombatTags::State_Downed))
	{
		Warning(FString::Printf(TEXT("넘어짐 몽타주 %s에 State.Downed 구간이 없습니다. 누운 동안에도 다시 맞으면 반응합니다"), *KnockdownMontage->GetName()));
	}
	if (DodgeMontage && !CombatDataValidation::HasTagWindow(DodgeMontage, CombatTags::State_Invulnerable))
	{
		Warning(FString::Printf(TEXT("회피 몽타주 %s에 State.Invulnerable 구간이 없습니다. 회피 중에도 맞습니다"), *DodgeMontage->GetName()));
	}
	if (PartialReactionRootBones.IsEmpty())
	{
		Warning(TEXT("부위 대표 본(PartialReactionRootBones)이 없습니다. 모든 부위 반응이 대체 본에서 시작합니다"));
	}

	return Result;
}
#endif
