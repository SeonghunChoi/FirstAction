#include "Combat/AttackDataAsset.h"

#include "Animation/AnimMontage.h"

#if WITH_EDITOR
#include "Combat/Abilities/GA_MeleeAttack.h"
#include "Combat/Animation/ANS_HitWindow.h"
#include "Combat/CombatDataValidation.h"
#include "Combat/CombatTags.h"
#include "Misc/DataValidation.h"
#endif

const UAttackDataAsset* UAttackDataAsset::FindNextAttack(const FGameplayTag& InputTag) const
{
	const FAttackComboLink* Link = ComboLinks.FindByPredicate(
		[&InputTag](const FAttackComboLink& Candidate) { return Candidate.InputTag == InputTag; });
	return Link ? Link->NextAttack.Get() : nullptr;
}

#if WITH_EDITOR

namespace
{
	// 링크를 따라가다 Start로 돌아오면 그 경로를 남긴다 (깊이 우선, 이미 본 공격은 다시 보지 않는다)
	bool FindCycleBackTo(const UAttackDataAsset* Start, const UAttackDataAsset* Current,
		TSet<const UAttackDataAsset*>& Visited, TArray<FString>& Path)
	{
		for (const FAttackComboLink& Link : Current->ComboLinks)
		{
			const UAttackDataAsset* Next = Link.NextAttack;
			if (!Next)
			{
				continue;
			}
			Path.Add(FString::Printf(TEXT("--%s--> %s"), *Link.InputTag.ToString(), *Next->GetName()));
			if (Next == Start)
			{
				return true;
			}
			if (!Visited.Contains(Next))
			{
				Visited.Add(Next);
				if (FindCycleBackTo(Start, Next, Visited, Path))
				{
					return true;
				}
			}
			Path.Pop();
		}
		return false;
	}
}

EDataValidationResult UAttackDataAsset::IsDataValid(FDataValidationContext& Context) const
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

	// 몽타주
	if (!Montage)
	{
		Error(TEXT("몽타주(Montage)가 비어 있습니다. 공격을 재생할 수 없습니다"));
	}
	else
	{
		if (!CombatDataValidation::HasNotifyState(Montage, UANS_HitWindow::StaticClass()))
		{
			Warning(FString::Printf(TEXT("%s에 판정 구간(ANS_HitWindow)이 없습니다. 이 공격은 맞지 않습니다 (헛동작이면 무시)"), *Montage->GetName()));
		}

		if (!ComboLinks.IsEmpty() && !CombatDataValidation::HasTagWindow(Montage, CombatTags::Window_Combo))
		{
			Error(FString::Printf(TEXT("콤보 링크가 있는데 %s에 Window.Combo 구간이 없습니다. 다음 공격으로 이어지지 않습니다"), *Montage->GetName()));
		}

		if (MaxWarpDistance > 0.f)
		{
			TArray<FName> WarpNames;
			CombatDataValidation::GetWarpTargetNames(Montage, WarpNames);
			const FName Expected = GetDefault<UGA_MeleeAttack>()->GetWarpTargetName();
			if (WarpNames.IsEmpty())
			{
				Warning(FString::Printf(TEXT("워핑 최대 거리가 %.0fcm인데 %s에 Motion Warping 구간이 없습니다. 대상에게 붙지 않습니다 (제자리 공격이면 무시)"),
					MaxWarpDistance, *Montage->GetName()));
			}
			else if (!WarpNames.Contains(Expected))
			{
				Warning(FString::Printf(TEXT("%s의 Motion Warping 목표 이름이 %s가 아닙니다 (GA_MeleeAttack이 등록하는 이름)"),
					*Montage->GetName(), *Expected.ToString()));
			}
		}
	}

	// 콤보 링크
	TSet<FGameplayTag> SeenInputs;
	for (int32 Index = 0; Index < ComboLinks.Num(); ++Index)
	{
		const FAttackComboLink& Link = ComboLinks[Index];
		if (!Link.InputTag.IsValid())
		{
			Error(FString::Printf(TEXT("콤보 링크 %d의 입력 태그가 비어 있습니다"), Index));
		}
		else if (SeenInputs.Contains(Link.InputTag))
		{
			Error(FString::Printf(TEXT("콤보 링크 %d: %s 링크가 이미 있습니다. 뒤의 링크는 쓰이지 않습니다"), Index, *Link.InputTag.ToString()));
		}
		SeenInputs.Add(Link.InputTag);

		if (!Link.NextAttack)
		{
			Error(FString::Printf(TEXT("콤보 링크 %d(%s)의 다음 공격이 비어 있습니다"), Index, *Link.InputTag.ToString()));
		}
	}

	// 순환: 링크를 따라가 이 공격으로 돌아오면 끝없이 이어지는 콤보다 (8-2: 맞을 때마다 경직이면 상대를 가둔다)
	TSet<const UAttackDataAsset*> Visited;
	Visited.Add(this);
	TArray<FString> Path;
	if (FindCycleBackTo(this, this, Visited, Path))
	{
		Warning(FString::Printf(TEXT("콤보 링크가 자기 자신으로 돌아옵니다: %s %s"), *GetName(), *FString::Join(Path, TEXT(" "))));
	}

	return Result;
}

#endif
