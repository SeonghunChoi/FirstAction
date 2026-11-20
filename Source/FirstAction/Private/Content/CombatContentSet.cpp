#include "Content/CombatContentSet.h"

#if WITH_EDITOR

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Combat/Animation/ANS_AttackReach.h"
#include "Combat/Animation/ANS_HitWindow.h"
#include "Combat/CombatDataValidation.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Misc/DataValidation.h"

namespace
{
	// 기본에 있는 것이 배포에 없으면 오류, 둘 다 있는데 개수가 다르면 경고 (9-1 질문 3 C)
	template <typename ErrorFn, typename WarningFn>
	void CompareCount(const FString& What, int32 DefaultCount, int32 ReleaseCount, bool bMissingIsError,
		const ErrorFn& Error, const WarningFn& Warning)
	{
		if (DefaultCount <= 0 || DefaultCount == ReleaseCount)
		{
			return;
		}
		if (ReleaseCount == 0)
		{
			const FString Message = FString::Printf(TEXT("%s이(가) 배포 쪽에 없습니다 (기본 %d개)"), *What, DefaultCount);
			bMissingIsError ? Error(Message) : Warning(Message);
		}
		else
		{
			Warning(FString::Printf(TEXT("%s 개수가 다릅니다 (기본 %d개, 배포 %d개)"), *What, DefaultCount, ReleaseCount));
		}
	}
}

EDataValidationResult UCombatContentSet::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	const FString P = CombatDataValidation::Prefix(this);

	// 짝마다 "기본 → 배포: " 접두사를 붙인다
	FString Pair;
	auto Error = [&](const FString& Message)
	{
		Context.AddError(FText::FromString(P + Pair + Message));
		Result = EDataValidationResult::Invalid;
	};
	auto Warning = [&](const FString& Message)
	{
		Context.AddWarning(FText::FromString(P + Pair + Message));
	};
	auto SetPair = [&](const UObject* Default, const UObject* Release)
	{
		Pair = FString::Printf(TEXT("%s → %s: "), *GetNameSafe(Default), *GetNameSafe(Release));
	};

	// 몽타주: 타이밍은 몽타주가 정하므로 구간 구성을 비교한다 (규칙 3, PLAN 5.2 규칙 3)
	for (const TPair<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimMontage>>& It : Montages)
	{
		const UAnimMontage* Default = It.Key;
		const UAnimMontage* Release = It.Value;
		SetPair(Default, Release);
		if (!Default || !Release)
		{
			Error(TEXT("짝의 한쪽이 비어 있습니다"));
			continue;
		}

		TMap<FGameplayTag, int32> DefaultTags;
		TMap<FGameplayTag, int32> ReleaseTags;
		CombatDataValidation::CountTagWindows(Default, DefaultTags);
		CombatDataValidation::CountTagWindows(Release, ReleaseTags);
		for (const TPair<FGameplayTag, int32>& Tag : DefaultTags)
		{
			CompareCount(Tag.Key.ToString() + TEXT(" 구간"), Tag.Value, ReleaseTags.FindRef(Tag.Key), true, Error, Warning);
		}

		CompareCount(TEXT("판정 구간(ANS_HitWindow)"),
			CombatDataValidation::CountNotifyState(Default, UANS_HitWindow::StaticClass()),
			CombatDataValidation::CountNotifyState(Release, UANS_HitWindow::StaticClass()), true, Error, Warning);

		// 멈출 간격이 없으면 몸이 닿는 거리까지 붙는다. 동작은 하므로 경고
		CompareCount(TEXT("멈출 간격(Attack Reach)"),
			CombatDataValidation::CountNotifyState(Default, UANS_AttackReach::StaticClass()),
			CombatDataValidation::CountNotifyState(Release, UANS_AttackReach::StaticClass()), false, Error, Warning);

		TArray<FName> DefaultWarps;
		TArray<FName> ReleaseWarps;
		CombatDataValidation::GetWarpTargetNames(Default, DefaultWarps);
		CombatDataValidation::GetWarpTargetNames(Release, ReleaseWarps);
		TSet<FName> WarpNames;
		WarpNames.Append(DefaultWarps);
		for (const FName& WarpName : WarpNames)
		{
			CompareCount(FString::Printf(TEXT("Motion Warping 구간(%s)"), *WarpName.ToString()),
				DefaultWarps.FilterByPredicate([&](const FName& N) { return N == WarpName; }).Num(),
				ReleaseWarps.FilterByPredicate([&](const FName& N) { return N == WarpName; }).Num(), true, Error, Warning);
		}

		// 몽타주 구간 이름: 어빌리티가 이름으로 넘어간다 (가드 Start/Loop/End/Parry)
		for (const FCompositeSection& Section : Default->CompositeSections)
		{
			if (Release->GetSectionIndex(Section.SectionName) == INDEX_NONE)
			{
				Error(FString::Printf(TEXT("몽타주 구간 %s가 배포 쪽에 없습니다"), *Section.SectionName.ToString()));
			}
		}
	}

	// 스켈레탈 메시: 소켓 이름(HandGrip_R 등)이 같아야 무기가 붙는다
	for (const TPair<TObjectPtr<USkeletalMesh>, TObjectPtr<USkeletalMesh>>& It : SkeletalMeshes)
	{
		const USkeletalMesh* Default = It.Key;
		const USkeletalMesh* Release = It.Value;
		SetPair(Default, Release);
		if (!Default || !Release)
		{
			Error(TEXT("짝의 한쪽이 비어 있습니다"));
			continue;
		}
		for (const USkeletalMeshSocket* Socket : Default->GetActiveSocketList())
		{
			if (Socket && !Release->FindSocket(Socket->SocketName))
			{
				Error(FString::Printf(TEXT("소켓 %s가 배포 쪽에 없습니다"), *Socket->SocketName.ToString()));
			}
		}
		if (Default->GetSkeleton() != Release->GetSkeleton())
		{
			Warning(TEXT("스켈레톤이 다릅니다. 애니메이션은 마네킹 스켈레톤 기준이다 (에셋 규칙 3). 애님 BP도 함께 치환했는지 확인하세요"));
		}
	}

	// 무기 메시: 판정 소켓(blade_start, blade_end)이 같아야 맞는다
	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UStaticMesh>>& It : StaticMeshes)
	{
		const UStaticMesh* Default = It.Key;
		const UStaticMesh* Release = It.Value;
		SetPair(Default, Release);
		if (!Default || !Release)
		{
			Error(TEXT("짝의 한쪽이 비어 있습니다"));
			continue;
		}
		for (const UStaticMeshSocket* Socket : Default->Sockets)
		{
			if (Socket && !Release->FindSocket(Socket->SocketName))
			{
				Error(FString::Printf(TEXT("소켓 %s가 배포 쪽에 없습니다"), *Socket->SocketName.ToString()));
			}
		}
	}

	for (const TPair<TSubclassOf<UAnimInstance>, TSubclassOf<UAnimInstance>>& It : AnimClasses)
	{
		SetPair(It.Key.Get(), It.Value.Get());
		if (!It.Key || !It.Value)
		{
			Error(TEXT("짝의 한쪽이 비어 있습니다"));
		}
	}

	// 짝이 없는 기본 몽타주 (9-1 질문 2 B: 실행 중에는 기본 몽타주로 재생된다)
	Pair.Reset();
	if (UAssetManager::IsInitialized())
	{
		FARFilter Filter;
		Filter.ClassPaths.Add(UAnimMontage::StaticClass()->GetClassPathName());
		Filter.PackagePaths.Add(TEXT("/Game/Portfolio"));
		Filter.bRecursivePaths = true;
		TArray<FAssetData> DefaultMontages;
		UAssetManager::Get().GetAssetRegistry().GetAssets(Filter, DefaultMontages);

		TSet<FSoftObjectPath> Covered;
		for (const TPair<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimMontage>>& It : Montages)
		{
			Covered.Add(FSoftObjectPath(It.Key.Get()));
		}

		TArray<FString> Missing;
		for (const FAssetData& Asset : DefaultMontages)
		{
			if (!Covered.Contains(Asset.GetSoftObjectPath()))
			{
				Missing.Add(Asset.AssetName.ToString());
			}
		}
		if (!Missing.IsEmpty())
		{
			Missing.Sort();
			Warning(FString::Printf(TEXT("배포 짝이 없는 기본 몽타주 %d/%d개 (기본 몽타주로 재생됨): %s"),
				Missing.Num(), DefaultMontages.Num(), *FString::Join(Missing, TEXT(", "))));
		}
	}

	return Result;
}

#endif
