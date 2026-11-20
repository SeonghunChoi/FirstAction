#include "Tools/FirstActionEditorLibrary.h"

#if WITH_EDITOR

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "FirstAction.h"

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

#endif
