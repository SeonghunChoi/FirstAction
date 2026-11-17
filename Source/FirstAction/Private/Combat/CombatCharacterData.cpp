#include "Combat/CombatCharacterData.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

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
