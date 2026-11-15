// 몽타주의 한 구간 동안 캐릭터 ASC에 태그 하나를 붙였다 떼는 범용 노티파이 스테이트. (PLAN 4.4)
// Window.Combo, Window.Cancel, State.Invulnerable 등 "언제"가 필요한 의미를 모두 이 클래스 하나로 처리한다.
// 타이밍은 애니메이션이, 의미는 태그가 가진다(설계 규칙 3). C++에는 프레임 번호가 없다.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ANS_TagWindow.generated.h"

UCLASS(meta = (DisplayName = "Tag Window"))
class FIRSTACTION_API UANS_TagWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 노티파이 트랙에 태그 이름이 보이게 한다
	virtual FString GetNotifyName_Implementation() const override;

	// 구간 동안 붙일 태그. 몽타주에 놓을 때마다 따로 정하므로 EditAnywhere
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag Window", meta = (Categories = "Window,State"))
	FGameplayTag Tag;
};
