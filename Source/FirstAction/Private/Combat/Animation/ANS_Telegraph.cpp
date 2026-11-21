#include "Combat/Animation/ANS_Telegraph.h"

#include "Combat/CombatCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

void UANS_Telegraph::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (ACombatCharacterBase* Character = MeshComp ? Cast<ACombatCharacterBase>(MeshComp->GetOwner()) : nullptr)
	{
		Character->BeginTelegraphWindow(TimeScale);
	}
}

void UANS_Telegraph::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (ACombatCharacterBase* Character = MeshComp ? Cast<ACombatCharacterBase>(MeshComp->GetOwner()) : nullptr)
	{
		Character->EndTelegraphWindow();
	}
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UANS_Telegraph::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Telegraph x%.2f"), TimeScale);
}
