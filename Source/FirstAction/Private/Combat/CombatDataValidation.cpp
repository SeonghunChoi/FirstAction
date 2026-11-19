#include "Combat/CombatDataValidation.h"

#if WITH_EDITOR

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimSequenceBase.h"
#include "AnimNotifyState_MotionWarping.h"
#include "Combat/Animation/ANS_TagWindow.h"
#include "RootMotionModifier.h"

namespace CombatDataValidation
{
	bool HasNotifyState(const UAnimSequenceBase* Animation, TSubclassOf<UAnimNotifyState> NotifyStateClass)
	{
		if (!Animation || !NotifyStateClass)
		{
			return false;
		}
		for (const FAnimNotifyEvent& Event : Animation->Notifies)
		{
			if (Event.NotifyStateClass && Event.NotifyStateClass->IsA(NotifyStateClass))
			{
				return true;
			}
		}
		return false;
	}

	bool HasTagWindow(const UAnimSequenceBase* Animation, const FGameplayTag& Tag)
	{
		if (!Animation)
		{
			return false;
		}
		for (const FAnimNotifyEvent& Event : Animation->Notifies)
		{
			const UANS_TagWindow* Window = Cast<UANS_TagWindow>(Event.NotifyStateClass);
			if (Window && Window->Tag == Tag)
			{
				return true;
			}
		}
		return false;
	}

	void GetWarpTargetNames(const UAnimSequenceBase* Animation, TArray<FName>& OutNames)
	{
		OutNames.Reset();
		if (!Animation)
		{
			return;
		}
		for (const FAnimNotifyEvent& Event : Animation->Notifies)
		{
			const UAnimNotifyState_MotionWarping* Warping = Cast<UAnimNotifyState_MotionWarping>(Event.NotifyStateClass);
			if (!Warping)
			{
				continue;
			}
			const URootMotionModifier_Warp* Warp = Cast<URootMotionModifier_Warp>(Warping->RootMotionModifier);
			OutNames.Add(Warp ? Warp->WarpTargetName : NAME_None);
		}
	}

	FString Prefix(const UObject* Asset)
	{
		return GetNameSafe(Asset) + TEXT(": ");
	}
}

#endif
