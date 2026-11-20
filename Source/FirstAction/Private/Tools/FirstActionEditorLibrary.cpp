#include "Tools/FirstActionEditorLibrary.h"

#if WITH_EDITOR

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "FirstAction.h"
#include "Animation/SkeletalMeshActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "MeshUtilitiesCommon.h"
#include "MeshUtilitiesEngine.h"
#include "PhysicsAssetUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PreviewScene.h"
#include "UObject/Package.h"

bool UFirstActionEditorLibrary::SetMontageSlot(UAnimMontage* Montage, FName SlotName)
{
	if (!Montage || Montage->SlotAnimTracks.IsEmpty() || SlotName.IsNone())
	{
		return false;
	}
	Montage->Modify();
	Montage->SlotAnimTracks[0].SlotName = SlotName;
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

bool UFirstActionEditorLibrary::SetMontageSegments(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations, const TArray<float>& PlayRates)
{
	return SetMontageSegmentRanges(Montage, Animations, {}, {}, PlayRates);
}

bool UFirstActionEditorLibrary::SetMontageSegmentRanges(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations,
	const TArray<float>& StartTimes, const TArray<float>& EndTimes, const TArray<float>& PlayRates)
{
	if (!Montage || Montage->SlotAnimTracks.IsEmpty() || Animations.IsEmpty())
	{
		return false;
	}
	for (const UAnimSequenceBase* Animation : Animations)
	{
		if (!Animation || Animation->GetSkeleton() != Montage->GetSkeleton())
		{
			UE_LOG(LogFirstAction, Warning, TEXT("SetMontageSegments: %s의 스켈레톤이 몽타주 %s와 다릅니다"),
				*GetNameSafe(Animation), *Montage->GetName());
			return false;
		}
	}

	Montage->Modify();
	FAnimTrack& Track = Montage->SlotAnimTracks[0].AnimTrack;
	Track.AnimSegments.Reset();

	float Position = 0.f;
	for (int32 Index = 0; Index < Animations.Num(); ++Index)
	{
		UAnimSequenceBase* Animation = Animations[Index];
		FAnimSegment Segment;
		Segment.SetAnimReference(Animation, true);
		const float Length = Animation->GetPlayLength();
		Segment.AnimStartTime = StartTimes.IsValidIndex(Index) ? FMath::Clamp(StartTimes[Index], 0.f, Length) : 0.f;
		Segment.AnimEndTime = EndTimes.IsValidIndex(Index) && EndTimes[Index] > Segment.AnimStartTime
			? FMath::Min(EndTimes[Index], Length) : Length;
		Segment.AnimPlayRate = PlayRates.IsValidIndex(Index) && PlayRates[Index] > 0.f ? PlayRates[Index] : 1.f;
		Segment.LoopingCount = 1;
		Segment.StartPos = Position;
		Position += Segment.GetLength();
		Track.AnimSegments.Add(Segment);
	}

	Montage->SetCompositeLength(Position);
	static_cast<UAnimCompositeBase*>(Montage)->UpdateCommonTargetFrameRate();
	Montage->UpdateLinkableElements();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

bool UFirstActionEditorLibrary::SetMontageSections(UAnimMontage* Montage, const TArray<FName>& Names, const TArray<float>& StartTimes, const TArray<FName>& NextNames)
{
	if (!Montage || Names.IsEmpty() || Names.Num() != StartTimes.Num())
	{
		return false;
	}

	Montage->Modify();
	Montage->CompositeSections.Reset();
	for (int32 Index = 0; Index < Names.Num(); ++Index)
	{
		FCompositeSection Section;
		Section.SectionName = Names[Index];
		Section.Link(Montage, FMath::Clamp(StartTimes[Index], 0.f, Montage->GetPlayLength()));
		Section.NextSectionName = NextNames.IsValidIndex(Index) ? NextNames[Index] : NAME_None;
		Montage->CompositeSections.Add(Section);
	}
	// 첫 구간은 0초에서 시작해야 한다 (엔진 몽타주 팩토리와 같은 규칙)
	Montage->CompositeSections[0].SetTime(0.f);

	Montage->UpdateLinkableElements();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

FString UFirstActionEditorLibrary::DescribeMontage(const UAnimMontage* Montage)
{
	if (!Montage)
	{
		return TEXT("(없음)");
	}
	FString Out = FString::Printf(TEXT("%s 길이 %.3f 스켈레톤 %s\n"), *Montage->GetName(), Montage->GetPlayLength(), *GetNameSafe(Montage->GetSkeleton()));
	for (const FSlotAnimationTrack& Slot : Montage->SlotAnimTracks)
	{
		Out += FString::Printf(TEXT("  슬롯 %s\n"), *Slot.SlotName.ToString());
		for (const FAnimSegment& Segment : Slot.AnimTrack.AnimSegments)
		{
			Out += FString::Printf(TEXT("    %s 시작 %.3f 구간 %.3f~%.3f 배속 %.2f\n"), *GetNameSafe(Segment.GetAnimReference()),
				Segment.StartPos, Segment.AnimStartTime, Segment.AnimEndTime, Segment.AnimPlayRate);
		}
	}
	for (const FCompositeSection& Section : Montage->CompositeSections)
	{
		Out += FString::Printf(TEXT("  구간 %s %.3f → %s\n"), *Section.SectionName.ToString(), Section.GetTime(), *Section.NextSectionName.ToString());
	}
	return Out;
}

UPhysicsAsset* UFirstActionEditorLibrary::CreateRagdollPhysicsAsset(USkeletalMesh* Mesh, const TArray<FName>& BodyBones, const FString& AssetPath)
{
	if (!Mesh || BodyBones.IsEmpty() || !AssetPath.StartsWith(TEXT("/Game/")))
	{
		UE_LOG(LogFirstAction, Warning, TEXT("CreateRagdollPhysicsAsset: 메시, 뼈 목록, /Game/ 경로가 필요하다 (%s)"), *AssetPath);
		return nullptr;
	}

	// 있으면 비우고 다시 채운다 (이 물리 에셋을 가리키는 BP 참조가 끊기지 않게)
	const FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
	UPhysicsAsset* PhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, *(AssetPath + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!PhysicsAsset)
	{
		UPackage* Package = CreatePackage(*AssetPath);
		PhysicsAsset = NewObject<UPhysicsAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(PhysicsAsset);
	}
	PhysicsAsset->Modify();
	PhysicsAsset->SkeletalBodySetups.Empty();
	PhysicsAsset->ConstraintSetup.Empty();
	PhysicsAsset->CollisionDisableTable.Empty();
	PhysicsAsset->UpdateBodySetupIndexMap();
	PhysicsAsset->UpdateBoundsBodiesArray();
	PhysicsAsset->PreviewSkeletalMesh = Mesh;

	const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
	const int32 NumBones = RefSkeleton.GetRawBoneNum();

	// 뼈마다 가장 강하게 붙은 정점 (뼈 공간)
	TArray<FBoneVertInfo> Infos;
	FMeshUtilitiesEngine::CalcBoneVertInfos(Mesh, Infos, true, 0);

	// 기준 자세의 컴포넌트 공간 변환 (부모가 항상 앞 번호)
	const TArray<FTransform>& LocalPose = RefSkeleton.GetRawRefBonePose();
	TArray<FTransform> ComponentPose;
	ComponentPose.SetNum(NumBones);
	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		const int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);
		ComponentPose[BoneIndex] = ParentIndex == INDEX_NONE ? LocalPose[BoneIndex] : LocalPose[BoneIndex] * ComponentPose[ParentIndex];
	}

	TArray<int32> BodyBoneIndices;
	for (const FName BoneName : BodyBones)
	{
		const int32 BoneIndex = RefSkeleton.FindRawBoneIndex(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("CreateRagdollPhysicsAsset: %s에 뼈 %s가 없다"), *Mesh->GetName(), *BoneName.ToString());
			continue;
		}
		BodyBoneIndices.AddUnique(BoneIndex);
	}
	BodyBoneIndices.Sort();   // 부모 뼈가 먼저

	// 목록 밖 뼈의 정점을 가장 가까운 목록 조상에 합친다: 비틀림 뼈, 손·발·머리 아래(손가락, 발가락, 얼굴).
	// 무기 뼈와 옷·머리카락 같은 나머지는 뺀다
	TArray<FBoneVertInfo> Merged;
	Merged.SetNum(NumBones);
	for (const int32 BoneIndex : BodyBoneIndices)
	{
		Merged[BoneIndex] = Infos[BoneIndex];
	}
	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		if (BodyBoneIndices.Contains(BoneIndex))
		{
			continue;
		}
		int32 Ancestor = RefSkeleton.GetRawParentIndex(BoneIndex);
		while (Ancestor != INDEX_NONE && !BodyBoneIndices.Contains(Ancestor))
		{
			Ancestor = RefSkeleton.GetRawParentIndex(Ancestor);
		}
		if (Ancestor == INDEX_NONE)
		{
			continue;
		}
		const FString BoneName = RefSkeleton.GetBoneName(BoneIndex).ToString();
		const FString AncestorName = RefSkeleton.GetBoneName(Ancestor).ToString();
		const bool bTwist = BoneName.Contains(TEXT("twist"));
		const bool bExtremity = AncestorName.StartsWith(TEXT("hand_")) || AncestorName.StartsWith(TEXT("foot_")) || AncestorName == TEXT("head");
		const bool bWeapon = BoneName.Contains(TEXT("weapon")) || BoneName.Contains(TEXT("sword")) || BoneName.Contains(TEXT("blade"));
		if ((!bTwist && !bExtremity) || bWeapon)
		{
			continue;
		}
		const FTransform ToAncestor = ComponentPose[BoneIndex].GetRelativeTransform(ComponentPose[Ancestor]);
		const FBoneVertInfo& From = Infos[BoneIndex];
		for (int32 V = 0; V < From.Positions.Num(); ++V)
		{
			Merged[Ancestor].Positions.Add(FVector3f(ToAncestor.TransformPosition(FVector(From.Positions[V]))));
			Merged[Ancestor].Normals.Add(FVector3f(ToAncestor.TransformVectorNoScale(FVector(From.Normals[V]))));
		}
	}

	// 바디: 정점에 맞춘 캡슐 (엔진 자동 생성과 같은 설정)
	FPhysAssetCreateParams Params;
	Params.GeomType = EFG_Sphyl;
	Params.VertWeight = EVW_DominantWeight;
	Params.AngularConstraintMode = ACM_Limited;
	for (const int32 BoneIndex : BodyBoneIndices)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		if (Merged[BoneIndex].Positions.Num() < 4)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("CreateRagdollPhysicsAsset: %s 정점 %d개라 바디를 만들지 않음"), *BoneName.ToString(), Merged[BoneIndex].Positions.Num());
			continue;
		}
		const int32 BodyIndex = FPhysicsAssetUtils::CreateNewBody(PhysicsAsset, BoneName, Params);
		if (!FPhysicsAssetUtils::CreateCollisionFromBone(PhysicsAsset->SkeletalBodySetups[BodyIndex], Mesh, BoneIndex, Params, Merged[BoneIndex]))
		{
			UE_LOG(LogFirstAction, Warning, TEXT("CreateRagdollPhysicsAsset: %s 충돌 모양을 만들지 못함"), *BoneName.ToString());
			FPhysicsAssetUtils::DestroyBody(PhysicsAsset, BodyIndex);
		}
	}

	// 관절: 바디마다 가장 가까운 조상 바디와, 자식 뼈 원점에 (엔진 자동 생성과 같은 방식)
	for (const int32 BoneIndex : BodyBoneIndices)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		const int32 BodyIndex = PhysicsAsset->FindBodyIndex(BoneName);
		if (BodyIndex == INDEX_NONE)
		{
			continue;
		}
		int32 ParentBone = RefSkeleton.GetRawParentIndex(BoneIndex);
		int32 ParentBody = INDEX_NONE;
		FName ParentName;
		while (ParentBone != INDEX_NONE)
		{
			ParentName = RefSkeleton.GetBoneName(ParentBone);
			ParentBody = PhysicsAsset->FindBodyIndex(ParentName);
			if (ParentBody != INDEX_NONE)
			{
				break;
			}
			ParentBone = RefSkeleton.GetRawParentIndex(ParentBone);
		}
		if (ParentBody == INDEX_NONE)
		{
			continue;
		}
		const int32 ConstraintIndex = FPhysicsAssetUtils::CreateNewConstraint(PhysicsAsset, BoneName);
		UPhysicsConstraintTemplate* Constraint = PhysicsAsset->ConstraintSetup[ConstraintIndex];
		Constraint->DefaultInstance.SetAngularSwing1Motion(ACM_Limited);
		Constraint->DefaultInstance.SetAngularSwing2Motion(ACM_Limited);
		Constraint->DefaultInstance.SetAngularTwistMotion(ACM_Limited);
		Constraint->DefaultInstance.ConstraintBone1 = BoneName;
		Constraint->DefaultInstance.ConstraintBone2 = ParentName;
		Constraint->DefaultInstance.SnapTransformsToDefault(EConstraintTransformComponentFlags::All, PhysicsAsset);
		Constraint->SetDefaultProfile(Constraint->DefaultInstance);
		PhysicsAsset->DisableCollision(BodyIndex, ParentBody);
	}
	PhysicsAsset->UpdateBodySetupIndexMap();
	PhysicsAsset->UpdateBoundsBodiesArray();

	// 기준 자세에서 겹치는 바디끼리 충돌을 끈다 (엔진 자동 생성과 같은 방식)
	{
		FPreviewScene TempScene;
		ASkeletalMeshActor* Actor = TempScene.GetWorld()->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FTransform::Identity);
		USkeletalMeshComponent* Component = Actor->GetSkeletalMeshComponent();
		Component->SetSkeletalMesh(Mesh);
		Component->SetPhysicsAsset(PhysicsAsset);
		Actor->RegisterAllComponents();
		const TArray<FBodyInstance*> Bodies = Component->Bodies;
		for (int32 A = 0; A < Bodies.Num(); ++A)
		{
			if (!Bodies[A] || !Bodies[A]->BodySetup.IsValid())
			{
				continue;
			}
			const FTransform BodyTM = Bodies[A]->GetUnrealWorldTransform();
			for (int32 B = A + 1; B < Bodies.Num(); ++B)
			{
				if (Bodies[B] && Bodies[B]->BodySetup.IsValid() && Bodies[A]->OverlapTestForBody(BodyTM.GetLocation(), BodyTM.GetRotation(), Bodies[B]))
				{
					PhysicsAsset->DisableCollision(A, B);
				}
			}
		}
	}

	PhysicsAsset->PostEditChange();
	PhysicsAsset->MarkPackageDirty();
	UE_LOG(LogFirstAction, Log, TEXT("CreateRagdollPhysicsAsset: %s 바디 %d개, 관절 %d개"), *AssetPath, PhysicsAsset->SkeletalBodySetups.Num(), PhysicsAsset->ConstraintSetup.Num());
	return PhysicsAsset;
}

#endif
