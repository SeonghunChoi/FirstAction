#include "Combat/Animation/ANS_AttackReach.h"

FString UANS_AttackReach::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Reach %.0fcm"), StopGap);
}
