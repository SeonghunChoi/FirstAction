// 배포판 에셋을 Python 원격 실행으로 만들 때 쓰는 에디터 전용 함수 (9-3, CLAUDE.md 2.4)
// Python에 노출되지 않는 몽타주 구조(슬롯, 세그먼트, 구간)를 다룬다. 노티파이와 커브는 엔진 AnimationLibrary로 충분하다
//
//   unreal.FirstActionEditorLibrary.set_montage_slot(montage, "UpperBody")
//   unreal.FirstActionEditorLibrary.set_montage_segments(montage, [anim_a, anim_b], [1.0, 1.0])
//   unreal.FirstActionEditorLibrary.set_montage_sections(montage, ["Default"], [0.0], [""])
//   print(unreal.FirstActionEditorLibrary.describe_montage(montage))

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FirstActionEditorLibrary.generated.h"

class UAnimMontage;
class UAnimSequenceBase;

UCLASS()
class FIRSTACTION_API UFirstActionEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	// 첫 슬롯 트랙의 슬롯 이름 (애님 BP의 Slot 노드 이름과 같아야 재생된다)
	UFUNCTION(BlueprintCallable, Category = "FirstAction|Editor")
	static bool SetMontageSlot(UAnimMontage* Montage, FName SlotName);

	// 첫 슬롯 트랙을 주어진 애니메이션들로 이어 붙여 다시 만든다. PlayRates가 비거나 짧으면 1.0
	// 몽타주 길이와 구간 연결도 갱신한다. 애니메이션의 스켈레톤이 몽타주와 다르면 실패
	UFUNCTION(BlueprintCallable, Category = "FirstAction|Editor")
	static bool SetMontageSegments(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Animations, const TArray<float>& PlayRates);

	// 몽타주 구간을 다시 만든다. NextNames[i]가 비어 있으면 다음 구간으로 넘어가지 않는다
	UFUNCTION(BlueprintCallable, Category = "FirstAction|Editor")
	static bool SetMontageSections(UAnimMontage* Montage, const TArray<FName>& Names, const TArray<float>& StartTimes, const TArray<FName>& NextNames);

	// 슬롯, 세그먼트, 구간, 길이를 한 줄씩 적은 문자열
	UFUNCTION(BlueprintCallable, Category = "FirstAction|Editor")
	static FString DescribeMontage(const UAnimMontage* Montage);
#endif
};
