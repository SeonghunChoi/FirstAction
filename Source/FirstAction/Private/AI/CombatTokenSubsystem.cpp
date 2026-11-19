#include "AI/CombatTokenSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatSettings.h"
#include "Combat/CombatTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "FirstAction.h"

namespace
{
	TAutoConsoleVariable<bool> CVarShowAttackTokens(
		TEXT("FirstAction.ShowAttackTokens"),
		false,
		TEXT("1이면 공격 토큰을 그린다: 대상 위 남은 토큰, 보유자 → 대상 빨간 선과 쥔 수, 기다리는 적의 순번(노랑)"));

	// 이 시간 안에 다시 요청한 적만 "지금 기다리는 중"으로 본다. 기다리는 적은 매 틱 요청한다
	constexpr double RequestLifetime = 0.25;

	const UAbilitySystemComponent* GetASC(const AActor* Actor)
	{
		const IAbilitySystemInterface* AbilityActor = Cast<IAbilitySystemInterface>(Actor);
		return AbilityActor ? AbilityActor->GetAbilitySystemComponent() : nullptr;
	}

	// 토큰을 회수할 몸 상태 (7-2 결정). 경직 묶음은 7-3과 같다: 움찔은 빼고 Staggered / Downed / Groggy
	const TCHAR* GetReclaimReason(const AActor* Holder)
	{
		const UAbilitySystemComponent* ASC = GetASC(Holder);
		if (!ASC)
		{
			return nullptr;
		}
		if (ASC->HasMatchingGameplayTag(CombatTags::State_Dead))
		{
			return TEXT("사망");
		}
		if (ASC->HasMatchingGameplayTag(CombatTags::State_Staggered)
			|| ASC->HasMatchingGameplayTag(CombatTags::State_Downed)
			|| ASC->HasMatchingGameplayTag(CombatTags::State_Groggy))
		{
			return TEXT("경직");
		}
		return nullptr;
	}
}

bool UCombatTokenSubsystem::RequestTurn(AActor* Requester, AActor* Target, int32 MinCost)
{
	const UWorld* World = GetWorld();
	if (!Requester || !Target || !World)
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();
	FTurnRequest& Request = Requests.FindOrAdd(Requester);
	Request.Target = Target;
	Request.Time = Now;

	if (Holds.Contains(Requester) || GetFreeTokens(Target) < FMath::Max(1, MinCost))
	{
		return false;
	}

	// 같은 대상을 기다리는 적 중 우선순위 1등만 (질문 4 C)
	const float MyCost = GetPriorityCost(Requester, Target);
	for (const TPair<TWeakObjectPtr<AActor>, FTurnRequest>& Other : Requests)
	{
		const AActor* OtherActor = Other.Key.Get();
		if (!OtherActor || OtherActor == Requester || Other.Value.Target.Get() != Target
			|| !IsRequestFresh(Other.Value, Now) || Holds.Contains(Other.Key))
		{
			continue;
		}
		if (GetPriorityCost(OtherActor, Target) < MyCost)
		{
			return false;
		}
	}
	return true;
}

void UCombatTokenSubsystem::CancelTurnRequest(AActor* Requester)
{
	Requests.Remove(Requester);
}

bool UCombatTokenSubsystem::AcquireTokens(AActor* Requester, AActor* Target, int32 Cost)
{
	Cost = FMath::Max(1, Cost);
	if (!Requester || !Target || Holds.Contains(Requester) || GetFreeTokens(Target) < Cost)
	{
		return false;
	}

	FTokenHold& Hold = Holds.Add(Requester);
	Hold.Target = Target;
	Hold.Count = Cost;
	Requests.Remove(Requester);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 토큰 %d개 얻음 → %s (남은 토큰 %d/%d)"),
		*Requester->GetName(), Cost, *Target->GetName(), GetFreeTokens(Target), GetMaxTokens(Target));
	return true;
}

void UCombatTokenSubsystem::ReleaseTokens(AActor* Requester, const TCHAR* Reason)
{
	FTokenHold Hold;
	if (!Holds.RemoveAndCopyValue(Requester, Hold))
	{
		return;
	}

	AActor* Target = Hold.Target.Get();
	UE_LOG(LogFirstAction, Log, TEXT("[%s] 토큰 %d개 반납 (%s) → %s (남은 토큰 %d/%d)"),
		*GetNameSafe(Requester), Hold.Count, Reason, *GetNameSafe(Target), GetFreeTokens(Target), GetMaxTokens(Target));
}

int32 UCombatTokenSubsystem::GetMaxTokens(const AActor* Target) const
{
	// 대상 하나의 토큰 수는 대상의 캐릭터 데이터 (질문 2 A). 데이터가 없으면 1
	const ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(Target);
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;
	return Data ? FMath::Max(1, Data->AttackTokens) : 1;
}

int32 UCombatTokenSubsystem::GetFreeTokens(const AActor* Target) const
{
	if (!Target)
	{
		return 0;
	}
	int32 Used = 0;
	for (const TPair<TWeakObjectPtr<AActor>, FTokenHold>& Hold : Holds)
	{
		if (Hold.Value.Target.Get() == Target)
		{
			Used += Hold.Value.Count;
		}
	}
	return FMath::Max(0, GetMaxTokens(Target) - Used);
}

int32 UCombatTokenSubsystem::GetHeldTokens(AActor* Requester) const
{
	const FTokenHold* Hold = Holds.Find(Requester);
	return Hold ? Hold->Count : 0;
}

float UCombatTokenSubsystem::GetPriorityCost(const AActor* Requester, const AActor* Target) const
{
	const FVector ToRequester = Requester->GetActorLocation() - Target->GetActorLocation();
	const float Gap = FMath::Max(0.f, static_cast<float>(ToRequester.Size2D())
		- Requester->GetSimpleCollisionRadius() - Target->GetSimpleCollisionRadius());

	// 대상의 시선: 플레이어는 카메라(컨트롤 회전), AI는 Focus 방향. 컨트롤러가 없으면 몸 방향
	const APawn* TargetPawn = Cast<APawn>(Target);
	const FVector ViewDirection = (TargetPawn ? TargetPawn->GetViewRotation() : Target->GetActorRotation()).Vector().GetSafeNormal2D();
	const FVector Direction = ToRequester.GetSafeNormal2D();
	const float AngleDegrees = (ViewDirection.IsNearlyZero() || Direction.IsNearlyZero())
		? 0.f
		: FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(ViewDirection, Direction)), -1.f, 1.f)));

	return Gap + AngleDegrees * GetDefault<UCombatSettings>()->AttackTokenAnglePenalty;
}

bool UCombatTokenSubsystem::IsRequestFresh(const FTurnRequest& Request, double Now) const
{
	return Now - Request.Time <= RequestLifetime;
}

void UCombatTokenSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	ReclaimTokens();

	if (CVarShowAttackTokens.GetValueOnGameThread())
	{
		DrawDebug(World->GetTimeSeconds());
	}
}

void UCombatTokenSubsystem::ReclaimTokens()
{
	TArray<TPair<TWeakObjectPtr<AActor>, const TCHAR*>> ToRelease;
	for (const TPair<TWeakObjectPtr<AActor>, FTokenHold>& Hold : Holds)
	{
		const AActor* Holder = Hold.Key.Get();
		if (!Holder)
		{
			ToRelease.Emplace(Hold.Key, TEXT("보유자 없어짐"));
		}
		else if (!Hold.Value.Target.IsValid())
		{
			ToRelease.Emplace(Hold.Key, TEXT("대상 없어짐"));
		}
		else if (const TCHAR* Reason = GetReclaimReason(Holder))
		{
			ToRelease.Emplace(Hold.Key, Reason);
		}
	}
	for (const TPair<TWeakObjectPtr<AActor>, const TCHAR*>& Entry : ToRelease)
	{
		if (AActor* Holder = Entry.Key.Get())
		{
			ReleaseTokens(Holder, *FString::Printf(TEXT("관리자 회수: %s"), Entry.Value));
		}
		else
		{
			Holds.Remove(Entry.Key);
		}
	}

	const double Now = GetWorld()->GetTimeSeconds();
	for (auto It = Requests.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !It.Value().Target.IsValid() || Now - It.Value().Time > 1.0)
		{
			It.RemoveCurrent();
		}
	}
}

void UCombatTokenSubsystem::DrawDebug(double Now) const
{
	UWorld* World = GetWorld();

	// 대상별로 남은 토큰과 기다리는 순번
	TSet<AActor*> Targets;
	for (const TPair<TWeakObjectPtr<AActor>, FTokenHold>& Hold : Holds)
	{
		if (AActor* Target = Hold.Value.Target.Get())
		{
			Targets.Add(Target);
		}
	}
	for (const TPair<TWeakObjectPtr<AActor>, FTurnRequest>& Request : Requests)
	{
		if (AActor* Target = Request.Value.Target.Get(); Target && IsRequestFresh(Request.Value, Now))
		{
			Targets.Add(Target);
		}
	}

	for (AActor* Target : Targets)
	{
		DrawDebugString(World, Target->GetActorLocation() + FVector(0.f, 0.f, 130.f),
			FString::Printf(TEXT("토큰 %d/%d"), GetFreeTokens(Target), GetMaxTokens(Target)), nullptr, FColor::Cyan, 0.f, true);

		TArray<TPair<float, AActor*>> Waiting;
		for (const TPair<TWeakObjectPtr<AActor>, FTurnRequest>& Request : Requests)
		{
			AActor* Requester = Request.Key.Get();
			if (Requester && Request.Value.Target.Get() == Target && IsRequestFresh(Request.Value, Now) && !Holds.Contains(Request.Key))
			{
				Waiting.Emplace(GetPriorityCost(Requester, Target), Requester);
			}
		}
		Waiting.Sort([](const TPair<float, AActor*>& A, const TPair<float, AActor*>& B) { return A.Key < B.Key; });
		for (int32 Rank = 0; Rank < Waiting.Num(); ++Rank)
		{
			DrawDebugString(World, Waiting[Rank].Value->GetActorLocation() + FVector(0.f, 0.f, 130.f),
				FString::Printf(TEXT("대기 %d (%.0f)"), Rank + 1, Waiting[Rank].Key), nullptr, FColor::Yellow, 0.f, true);
		}
	}

	for (const TPair<TWeakObjectPtr<AActor>, FTokenHold>& Hold : Holds)
	{
		const AActor* Holder = Hold.Key.Get();
		const AActor* Target = Hold.Value.Target.Get();
		if (Holder && Target)
		{
			DrawDebugLine(World, Holder->GetActorLocation(), Target->GetActorLocation(), FColor::Red, false, -1.f, 0, 2.f);
			DrawDebugString(World, Holder->GetActorLocation() + FVector(0.f, 0.f, 130.f),
				FString::Printf(TEXT("토큰 %d"), Hold.Value.Count), nullptr, FColor::Red, 0.f, true);
		}
	}
}

TStatId UCombatTokenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCombatTokenSubsystem, STATGROUP_Tickables);
}
