#include "Combat/LockOnComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatTags.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "FirstAction.h"

namespace
{
	// 깃 버전에는 락온 표시 UI가 없어, 대상 머리 위에 디버그 표시를 그린다 (FirstAction.ShowLockOn 0으로 끔)
	TAutoConsoleVariable<bool> CVarShowLockOn(
		TEXT("FirstAction.ShowLockOn"),
		true,
		TEXT("락온 대상 머리 위에 표시를 그린다 (0: 끔, 1: 켬)"));
}

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

FVector ULockOnComponent::GetViewDirection() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	const FVector Direction = Controller ? Controller->GetControlRotation().Vector() : GetOwner()->GetActorForwardVector();
	return Direction.GetSafeNormal2D();
}

bool ULockOnComponent::IsValidTarget(const AActor* Candidate, float MaxDistance) const
{
	if (!Candidate || Candidate == GetOwner() || Candidate->IsActorBeingDestroyed() || !Candidate->IsA<ACombatCharacterBase>())
	{
		return false;
	}
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate);
	if (ASC && ASC->HasMatchingGameplayTag(CombatTags::State_Dead))
	{
		return false;
	}
	return FVector::Dist2D(Candidate->GetActorLocation(), GetOwner()->GetActorLocation()) <= MaxDistance;
}

AActor* ULockOnComponent::FindBestTarget(const AActor* Exclude) const
{
	const FVector View = GetViewDirection();
	const FVector From = GetOwner()->GetActorLocation();

	AActor* Best = nullptr;
	float BestCos = -1.f;
	for (TActorIterator<ACombatCharacterBase> It(GetWorld()); It; ++It)
	{
		ACombatCharacterBase* Candidate = *It;
		if (Candidate == Exclude || !IsValidTarget(Candidate, AcquireDistance))
		{
			continue;
		}
		// 카메라 정면에서 각도가 가장 작은 대상 (6-14 결정). 카메라 뒤쪽은 잡지 않는다
		const float Cos = FVector::DotProduct(View, (Candidate->GetActorLocation() - From).GetSafeNormal2D());
		if (Cos > 0.f && Cos > BestCos)
		{
			BestCos = Cos;
			Best = Candidate;
		}
	}
	return Best;
}

AActor* ULockOnComponent::FindSwitchTarget(float Side) const
{
	const AActor* Current = LockTarget.Get();
	if (!Current)
	{
		return nullptr;
	}

	const FVector From = GetOwner()->GetActorLocation();
	const FVector ToCurrent = (Current->GetActorLocation() - From).GetSafeNormal2D();

	AActor* Best = nullptr;
	float BestAngle = TNumericLimits<float>::Max();
	for (TActorIterator<ACombatCharacterBase> It(GetWorld()); It; ++It)
	{
		ACombatCharacterBase* Candidate = *It;
		if (Candidate == Current || !IsValidTarget(Candidate, AcquireDistance))
		{
			continue;
		}
		// 지금 대상에서 본 좌우 각도(도). 위에서 보아 시계 방향(오른쪽)이 +
		const FVector ToCandidate = (Candidate->GetActorLocation() - From).GetSafeNormal2D();
		const float SignedAngle = FMath::RadiansToDegrees(FMath::Atan2(FVector::CrossProduct(ToCurrent, ToCandidate).Z, FVector::DotProduct(ToCurrent, ToCandidate)));
		if (SignedAngle * Side > 0.f && FMath::Abs(SignedAngle) < BestAngle)
		{
			BestAngle = FMath::Abs(SignedAngle);
			Best = Candidate;
		}
	}
	return Best;
}

void ULockOnComponent::SetLockTarget(AActor* NewTarget)
{
	LockTarget = NewTarget;
	SetComponentTickEnabled(NewTarget != nullptr);
	UE_LOG(LogFirstAction, Log, TEXT("[%s] 락온 %s"), *GetNameSafe(GetOwner()),
		NewTarget ? *FString::Printf(TEXT("대상: %s"), *GetNameSafe(NewTarget)) : TEXT("해제"));
}

void ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		SetLockTarget(nullptr);
		return;
	}

	AActor* Target = FindBestTarget(nullptr);
	if (!Target)
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] 락온: %.0fcm 안 카메라 앞에 대상 없음"), *GetNameSafe(GetOwner()), AcquireDistance);
		return;
	}
	bSwitchArmed = true;
	SetLockTarget(Target);
}

void ULockOnComponent::HandleSwitchInput(const FVector2D& StickAxis)
{
	if (!IsLockedOn())
	{
		return;
	}

	// 문턱값의 절반 아래로 돌아오면 다시 튕길 수 있다 (놓는 순간은 ResetSwitchInput)
	if (FMath::Abs(StickAxis.X) < SwitchStickThreshold * 0.5f)
	{
		bSwitchArmed = true;
		return;
	}
	if (!bSwitchArmed || FMath::Abs(StickAxis.X) < SwitchStickThreshold)
	{
		return;
	}

	bSwitchArmed = false;
	if (AActor* Next = FindSwitchTarget(FMath::Sign(StickAxis.X)))
	{
		SetLockTarget(Next);
	}
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Target = LockTarget.Get();

	// 대상이 죽었거나 사라짐 → 다음 대상으로, 없으면 해제 (6-14 결정)
	const UAbilitySystemComponent* TargetASC = Target ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target) : nullptr;
	if (!Target || Target->IsActorBeingDestroyed() || (TargetASC && TargetASC->HasMatchingGameplayTag(CombatTags::State_Dead)))
	{
		SetLockTarget(FindBestTarget(Target));
		return;
	}

	// 벗어남 → 해제
	if (!IsValidTarget(Target, LoseDistance))
	{
		SetLockTarget(nullptr);
		return;
	}

	// 카메라 좌우만 대상 쪽으로 (위아래는 그대로)
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (Controller)
	{
		const FRotator Current = Controller->GetControlRotation();
		const float TargetYaw = (Target->GetActorLocation() - GetOwner()->GetActorLocation()).Rotation().Yaw;
		const FRotator Desired(Current.Pitch, TargetYaw, Current.Roll);
		// 카메라는 히트스톱·슬로모션과 무관하게 실제 시간으로 돈다
		const float RealDelta = DeltaTime / FMath::Max(GetOwner()->CustomTimeDilation, KINDA_SMALL_NUMBER);
		Controller->SetControlRotation(FMath::RInterpConstantTo(Current, Desired, RealDelta, CameraYawSpeed));
	}

	if (CVarShowLockOn.GetValueOnGameThread())
	{
		const FVector Top = Target->GetActorLocation() + FVector(0.f, 0.f, Target->GetSimpleCollisionHalfHeight() + 30.f);
		DrawDebugSphere(GetWorld(), Top, 12.f, 8, FColor::Red, false, 0.f, 0, 1.5f);
	}
}
