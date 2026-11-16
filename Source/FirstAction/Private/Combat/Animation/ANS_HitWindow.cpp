#include "Combat/Animation/ANS_HitWindow.h"

#include "Combat/MeleeTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	// 캐릭터 클래스를 몰라도 되도록 컴포넌트로 찾는다. 몽타주 에디터 미리보기에서는 nullptr
	UMeleeTraceComponent* FindMeleeTrace(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UMeleeTraceComponent>() : nullptr;
	}
}

void UANS_HitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UMeleeTraceComponent* MeleeTrace = FindMeleeTrace(MeshComp))
	{
		MeleeTrace->BeginTrace();
	}
}

void UANS_HitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	// 콤보로 몽타주가 끊겨도 여기로 와서 판정이 꺼진다
	if (UMeleeTraceComponent* MeleeTrace = FindMeleeTrace(MeshComp))
	{
		MeleeTrace->EndTrace();
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UANS_HitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Hit Window");
}
