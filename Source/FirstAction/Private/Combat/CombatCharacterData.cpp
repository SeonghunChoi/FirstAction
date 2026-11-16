#include "Combat/CombatCharacterData.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

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
