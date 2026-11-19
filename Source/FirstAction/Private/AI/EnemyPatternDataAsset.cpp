#include "AI/EnemyPatternDataAsset.h"

#if WITH_EDITOR

#include "Combat/AttackDataAsset.h"
#include "Combat/CombatDataValidation.h"
#include "Misc/DataValidation.h"

EDataValidationResult UEnemyPatternDataAsset::IsDataValid(FDataValidationContext& Context) const
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
	auto CheckRange = [&](const TCHAR* MinName, float Min, const TCHAR* MaxName, float Max)
	{
		if (Min > Max)
		{
			Error(FString::Printf(TEXT("%s(%.2f)가 %s(%.2f)보다 큽니다"), MinName, Min, MaxName, Max));
		}
	};

	// 인지와 교전 값
	CheckRange(TEXT("SightRadius"), SightRadius, TEXT("LoseSightRadius"), LoseSightRadius);
	CheckRange(TEXT("StrafeDistance"), StrafeDistance, TEXT("MaxStrafeDistance"), MaxStrafeDistance);
	CheckRange(TEXT("StrafeDurationMin"), StrafeDurationMin, TEXT("StrafeDurationMax"), StrafeDurationMax);
	CheckRange(TEXT("ComboInputDelayMin"), ComboInputDelayMin, TEXT("ComboInputDelayMax"), ComboInputDelayMax);

	// 패턴
	if (Patterns.IsEmpty())
	{
		Warning(TEXT("공격 패턴이 없습니다. 이 적은 공격하지 않습니다"));
	}

	TSet<FName> SeenNames;
	for (int32 Index = 0; Index < Patterns.Num(); ++Index)
	{
		const FEnemyAttackPattern& Pattern = Patterns[Index];
		const FString Label = Pattern.Name.IsNone() ? FString::Printf(TEXT("패턴 %d"), Index) : FString::Printf(TEXT("패턴 %s"), *Pattern.Name.ToString());

		if (Pattern.Name.IsNone())
		{
			Warning(FString::Printf(TEXT("%s의 이름이 비어 있습니다 (로그에서 구분하기 어렵습니다)"), *Label));
		}
		else if (SeenNames.Contains(Pattern.Name))
		{
			Warning(FString::Printf(TEXT("%s: 같은 이름의 패턴이 이미 있습니다"), *Label));
		}
		SeenNames.Add(Pattern.Name);

		if (Pattern.Weight <= 0.f)
		{
			Warning(FString::Printf(TEXT("%s의 가중치가 0 이하라 고를 수 없습니다"), *Label));
		}
		if (Pattern.MinGap > Pattern.MaxGap)
		{
			Error(FString::Printf(TEXT("%s의 MinGap(%.0f)이 MaxGap(%.0f)보다 큽니다. 고를 수 있는 거리가 없습니다"), *Label, Pattern.MinGap, Pattern.MaxGap));
		}

		if (!Pattern.StartAttack)
		{
			Error(FString::Printf(TEXT("%s의 시작 공격(StartAttack)이 비어 있습니다"), *Label));
			continue;
		}

		// 연결: 시작 공격부터 입력 순서대로 ComboLinks를 따라간다 (적은 입력만 보내고 다음 공격은 링크가 정한다, 규칙 1·4)
		const UAttackDataAsset* Current = Pattern.StartAttack;
		FString Route = Current->GetName();
		for (int32 Step = 0; Step < Pattern.ComboInputs.Num(); ++Step)
		{
			const FGameplayTag& Input = Pattern.ComboInputs[Step];
			const UAttackDataAsset* Next = Current->FindNextAttack(Input);
			if (!Next)
			{
				Error(FString::Printf(TEXT("%s의 입력 %d/%d(%s): %s에 이 입력의 콤보 링크가 없습니다. 경로 %s에서 끊깁니다"),
					*Label, Step + 1, Pattern.ComboInputs.Num(), *Input.ToString(), *Current->GetName(), *Route));
				break;
			}
			Current = Next;
			Route += FString::Printf(TEXT(" --%s--> %s"), *Input.ToString(), *Current->GetName());
		}
	}

	return Result;
}

#endif
