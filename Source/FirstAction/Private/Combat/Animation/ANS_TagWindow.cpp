#include "Combat/Animation/ANS_TagWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	// 몽타주 에디터 미리보기처럼 ASC가 없는 곳에서는 nullptr
	UAbilitySystemComponent* FindAbilitySystem(const USkeletalMeshComponent* MeshComp)
	{
		return MeshComp ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()) : nullptr;
	}
}

void UANS_TagWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!Tag.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = FindAbilitySystem(MeshComp))
	{
		// Loose 태그: GameplayEffect 없이 직접 붙이는 태그. 붙인 횟수만큼 떼야 사라진다(개수로 관리)
		ASC->AddLooseGameplayTag(Tag);
	}
}

void UANS_TagWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	// 몽타주가 중간에 끊겨도 구간이 끝난 것으로 처리되어 여기로 와야 한다 (4-3 콤보에서 끊김 상황으로 확인)
	if (Tag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = FindAbilitySystem(MeshComp))
		{
			ASC->RemoveLooseGameplayTag(Tag);
		}
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UANS_TagWindow::GetNotifyName_Implementation() const
{
	return Tag.IsValid() ? Tag.ToString() : TEXT("Tag Window (태그 없음)");
}
