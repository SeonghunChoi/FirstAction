#include "AI/EnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AI/EnemyCharacter.h"
#include "AI/EnemyPatternDataAsset.h"
#include "Combat/CombatTags.h"
#include "Components/StateTreeAIComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Sight.h"
#include "FirstAction.h"

namespace
{
	TAutoConsoleVariable<int32> CVarShowEnemyAI(
		TEXT("FirstAction.ShowEnemyAI"),
		0,
		TEXT("1이면 적 AI의 시야(회색), 대상 선(초록: 보임, 노랑: 반응 대기, 주황: 가까워 지금 위치를 봄, 보라: 멀어 마지막 위치를 봄), 마지막으로 안 위치(빨강)를 그린다"));

	bool IsActorDead(const AActor* Actor)
	{
		const IAbilitySystemInterface* AbilityActor = Cast<IAbilitySystemInterface>(Actor);
		const UAbilitySystemComponent* ASC = AbilityActor ? AbilityActor->GetAbilitySystemComponent() : nullptr;
		return ASC && ASC->HasMatchingGameplayTag(CombatTags::State_Dead);
	}
}

AEnemyAIController::AEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// 템플릿 ACombatAIController와 같다: 폰에 붙이고, 빙의할 때 StateTree를 시작한다
	bAttachToPawn = true;
	bStartAILogicOnPossess = true;

	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	// 폰이 없을 때(BeginPlay) 먼저 시작하지 않게 한다. 시작은 빙의할 때 한 번
	StateTreeAI->SetStartLogicAutomatically(false);

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	// 시각: 적(팀 태도 Hostile)만 본다. 거리와 각도는 빙의할 때 패턴 데이터로 덮어쓴다
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	// 피해: 맞으면 때린 쪽을 안다. 캐릭터가 Event.HitReact를 받을 때 보고한다
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*DamageConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*Perception);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	// Super 안에서 폰의 PossessedBy가 이 컨트롤러에 팀을 넘긴다 (7-1)
	Super::OnPossess(InPawn);

	ApplyPatternData();

	// Super가 StateTree를 시작한다(bStartAILogicOnPossess). 트리 에셋이 비었거나 컨텍스트가 맞지 않으면 엔진은 조용히 멈추므로 여기서 알린다
	if (StateTreeAI && !StateTreeAI->IsRunning())
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] StateTree가 시작되지 않음 (%s의 StateTreeAI 컴포넌트에 State Tree가 지정되었는지, 스키마가 맞는지 확인)"),
			*GetNameSafe(InPawn), *GetClass()->GetName());
	}

	if (UAIPerceptionComponent* Perception = GetPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AEnemyAIController::HandleTargetPerceptionUpdated);
	}
}

void AEnemyAIController::OnUnPossess()
{
	ClearCombatTarget(TEXT("빙의 해제"));

	// 엔진은 Super에서 폰을 비운 뒤 StateTree를 멈춰 컨텍스트(Actor)를 찾지 못한다는 경고를 남긴다. 폰이 있을 때 먼저 멈춘다
	if (StateTreeAI)
	{
		StateTreeAI->StopLogic(TEXT("빙의 해제"));
	}

	Super::OnUnPossess();
}

const UEnemyPatternDataAsset* AEnemyAIController::GetPatternData() const
{
	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	const UEnemyPatternDataAsset* Data = Enemy ? Enemy->GetPatternData() : nullptr;
	// 지정하지 않았으면 클래스 기본값으로 동작한다 (경고는 ApplyPatternData에서 한 번)
	return Data ? Data : GetDefault<UEnemyPatternDataAsset>();
}

void AEnemyAIController::ApplyPatternData()
{
	const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	if (Enemy && !Enemy->GetPatternData())
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] Pattern Data가 없어 기본값으로 인지한다"), *GetNameSafe(Enemy));
	}

	const UEnemyPatternDataAsset* Data = GetPatternData();
	SightConfig->SightRadius = Data->SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(Data->LoseSightRadius, Data->SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = Data->PeripheralVisionHalfAngle;

	UAIPerceptionComponent* Perception = GetPerceptionComponent();
	if (!Perception)
	{
		return;
	}
	// 바뀐 시각 설정을 다시 읽게 하고(ConfigureSense), 빙의할 때 받은 팀으로 인지 등록을 갱신한다.
	// 인지는 등록 시점의 팀으로 적·아군을 가르므로, 팀을 받기 전(NoTeam)에 등록된 정보를 고쳐야 한다
	Perception->ConfigureSense(*SightConfig);
	Perception->RequestStimuliListenerUpdate();

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 인지: 시야 %.0f/%.0fcm, 반각 %.0f°, 반응 %.2f초, 기억 %.1f초, 근접 %.0fcm, 팀 %d"),
		*GetNameSafe(GetPawn()), SightConfig->SightRadius, SightConfig->LoseSightRadius, SightConfig->PeripheralVisionAngleDegrees,
		Data->ReactionTime, Data->TargetMemoryTime, Data->ProximityAwarenessRadius, GetGenericTeamId().GetId());
}

bool AEnemyAIController::IsPawnDead() const
{
	return IsActorDead(GetPawn());
}

float AEnemyAIController::GetBodyGap(const AActor* Target) const
{
	const APawn* MyPawn = GetPawn();
	if (!MyPawn || !Target)
	{
		return TNumericLimits<float>::Max();
	}
	const float CenterDistance = FVector::Dist2D(MyPawn->GetActorLocation(), Target->GetActorLocation());
	return CenterDistance - MyPawn->GetSimpleCollisionRadius() - Target->GetSimpleCollisionRadius();
}

void AEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !GetWorld() || IsPawnDead() || IsActorDead(Actor))
	{
		return;
	}
	// 피해 감각은 편을 거르지 않으므로 여기서 한 번 더 거른다
	if (GetTeamAttitudeTowards(*Actor) != ETeamAttitude::Hostile)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (!CombatTarget.IsValid())
			{
				SetCombatTarget(Actor, TEXT("보임"));
			}
			else if (Actor == CombatTarget.Get() && !bTargetVisible)
			{
				UE_LOG(LogFirstAction, Log, TEXT("[%s] 대상 다시 보임: %s"), *GetNameSafe(GetPawn()), *Actor->GetName());
			}

			if (Actor == CombatTarget.Get())
			{
				bTargetVisible = true;
				bReactedToLostTarget = false;
				LastSensedTime = Now;
				LastKnownLocation = Actor->GetActorLocation();
			}
		}
		else if (Actor == CombatTarget.Get())
		{
			// 시야에서 사라짐: 반응 시간 동안은 마지막으로 본 위치를 본다
			bTargetVisible = false;
			bReactedToLostTarget = false;
			bTargetNearby = false;
			LostSightTime = Now;
			LastKnownLocation = Stimulus.StimulusLocation;
			UE_LOG(LogFirstAction, Log, TEXT("[%s] 대상 시야에서 사라짐: %s (반응 %.2f초 뒤 돌아섬)"),
				*GetNameSafe(GetPawn()), *Actor->GetName(), GetPatternData()->ReactionTime);
		}
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>() && Stimulus.WasSuccessfullySensed())
	{
		if (!CombatTarget.IsValid())
		{
			SetCombatTarget(Actor, TEXT("맞음"));
		}

		if (Actor == CombatTarget.Get())
		{
			LastSensedTime = Now;
			LastKnownLocation = Actor->GetActorLocation();
			// 맞으면 반응 시간 없이 지금 위치로 돈다 (질문 4 C)
			if (!bTargetVisible && !bReactedToLostTarget)
			{
				bReactedToLostTarget = true;
				UE_LOG(LogFirstAction, Log, TEXT("[%s] 시야 밖에서 맞음: 즉시 %s 쪽으로 돌아섬"), *GetNameSafe(GetPawn()), *Actor->GetName());
			}
		}
	}
}

void AEnemyAIController::SetCombatTarget(AActor* NewTarget, const TCHAR* Reason)
{
	CombatTarget = NewTarget;
	bTargetVisible = false;
	bReactedToLostTarget = true;
	bTargetNearby = false;
	LastSensedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	LastKnownLocation = NewTarget ? NewTarget->GetActorLocation() : FVector::ZeroVector;

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 대상 잡음: %s (%s)"), *GetNameSafe(GetPawn()), *GetNameSafe(NewTarget), Reason);
}

void AEnemyAIController::ClearCombatTarget(const TCHAR* Reason)
{
	if (!CombatTarget.IsValid() && !bTargetVisible)
	{
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("[%s] 대상 놓음: %s (%s)"), *GetNameSafe(GetPawn()), *GetNameSafe(CombatTarget.Get()), Reason);

	CombatTarget.Reset();
	bTargetVisible = false;
	bReactedToLostTarget = false;
	bTargetNearby = false;
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!GetPawn() || !GetWorld())
	{
		return;
	}

	if (IsPawnDead())
	{
		ClearCombatTarget(TEXT("사망"));
		return;
	}

	AActor* Target = CombatTarget.Get();
	if (Target && (Target->IsActorBeingDestroyed() || IsActorDead(Target)))
	{
		ClearCombatTarget(TEXT("대상 사망"));
		Target = nullptr;
	}

	if (Target)
	{
		const UEnemyPatternDataAsset* Data = GetPatternData();
		const double Now = GetWorld()->GetTimeSeconds();

		if (bTargetVisible)
		{
			LastSensedTime = Now;
			LastKnownLocation = Target->GetActorLocation();
		}
		else
		{
			if (!bReactedToLostTarget && Now - LostSightTime >= Data->ReactionTime)
			{
				bReactedToLostTarget = true;
				UE_LOG(LogFirstAction, Log, TEXT("[%s] 반응 시간 지남: 돌아섬 (몸 간격 %.0f, 근접 %.0f)"),
					*GetNameSafe(GetPawn()), GetBodyGap(Target), Data->ProximityAwarenessRadius);
			}

			// 돌아선 뒤: 가까우면 지금 위치를 안다(기억도 새로 센다). 멀면 마지막으로 안 위치만 안다 (질문 7 B)
			if (bReactedToLostTarget)
			{
				const bool bNearbyNow = GetBodyGap(Target) <= Data->ProximityAwarenessRadius;
				if (bNearbyNow != bTargetNearby)
				{
					UE_LOG(LogFirstAction, Log, TEXT("[%s] %s: %s"), *GetNameSafe(GetPawn()), *Target->GetName(),
						bNearbyNow ? TEXT("근접 감지 거리 안, 지금 위치를 앎") : TEXT("근접 감지 거리 밖, 마지막으로 안 위치만 앎"));
				}
				bTargetNearby = bNearbyNow;
				if (bTargetNearby)
				{
					LastSensedTime = Now;
					LastKnownLocation = Target->GetActorLocation();
				}
			}

			if (Now - LastSensedTime > Data->TargetMemoryTime)
			{
				ClearCombatTarget(TEXT("기억 시간 지남"));
			}
		}
	}

	UpdateFocus();
	DrawDebug();
}

void AEnemyAIController::UpdateFocus()
{
	AActor* Target = CombatTarget.Get();
	if (!Target)
	{
		return;
	}

	// 보이거나, 돌아선 뒤 가까우면 대상의 지금 위치를 본다 (질문 6 B, 질문 7 B).
	// 반응 대기 중이거나 멀어졌으면 마지막으로 안 위치를 본다
	if (bTargetVisible || (bReactedToLostTarget && bTargetNearby))
	{
		SetFocus(Target, EAIFocusPriority::Gameplay);
	}
	else
	{
		SetFocalPoint(LastKnownLocation, EAIFocusPriority::Gameplay);
	}
}

void AEnemyAIController::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (CVarShowEnemyAI.GetValueOnGameThread() == 0)
	{
		return;
	}

	const APawn* MyPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!MyPawn || !World)
	{
		return;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// 시야: 정면 기준 좌우 반각 선
	const FVector Forward = MyPawn->GetActorForwardVector().GetSafeNormal2D();
	const float HalfAngle = SightConfig->PeripheralVisionAngleDegrees;
	const float Radius = SightConfig->SightRadius;
	DrawDebugLine(World, EyeLocation, EyeLocation + Forward.RotateAngleAxis(HalfAngle, FVector::UpVector) * Radius, FColor(128, 128, 128), false, -1.f, 0, 1.f);
	DrawDebugLine(World, EyeLocation, EyeLocation + Forward.RotateAngleAxis(-HalfAngle, FVector::UpVector) * Radius, FColor(128, 128, 128), false, -1.f, 0, 1.f);

	const AActor* Target = CombatTarget.Get();
	FString Status = TEXT("대상 없음");
	if (Target)
	{
		const FColor LineColor = bTargetVisible ? FColor::Green
			: !bReactedToLostTarget ? FColor::Yellow
			: bTargetNearby ? FColor::Orange : FColor::Purple;
		DrawDebugLine(World, EyeLocation, Target->GetActorLocation(), LineColor, false, -1.f, 0, 2.f);

		if (!bTargetVisible)
		{
			DrawDebugSphere(World, LastKnownLocation, 20.f, 8, FColor::Red, false, -1.f, 0, 1.f);
			const double Remaining = GetPatternData()->TargetMemoryTime - (World->GetTimeSeconds() - LastSensedTime);
			Status = FString::Printf(TEXT("%s: %s, 기억 %.1f초"), *Target->GetName(),
				!bReactedToLostTarget ? TEXT("반응 대기") : (bTargetNearby ? TEXT("가까움, 지금 위치") : TEXT("멂, 마지막 위치")), FMath::Max(Remaining, 0.0));
		}
		else
		{
			Status = FString::Printf(TEXT("%s: 보임"), *Target->GetName());
		}
	}
	DrawDebugString(World, MyPawn->GetActorLocation() + FVector(0.f, 0.f, 110.f), Status, nullptr, FColor::White, 0.f, true);
#endif
}
