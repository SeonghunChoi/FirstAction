#include "AI/StateTree/EnemyStateTreeNodes.h"

#include "AbilitySystemComponent.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyCharacter.h"
#include "AI/EnemyPatternDataAsset.h"
#include "Combat/AttackDataAsset.h"
#include "Combat/CombatTags.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"
#include "FirstAction.h"

namespace
{
	AEnemyCharacter* GetEnemy(const AEnemyAIController* Controller)
	{
		return Controller ? Cast<AEnemyCharacter>(Controller->GetPawn()) : nullptr;
	}

	UAbilitySystemComponent* GetEnemyASC(const AEnemyAIController* Controller)
	{
		const AEnemyCharacter* Enemy = GetEnemy(Controller);
		return Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	}

	void SetMoveSpeed(const AEnemyAIController* Controller, float Speed)
	{
		if (AEnemyCharacter* Enemy = GetEnemy(Controller))
		{
			Enemy->GetCharacterMovement()->MaxWalkSpeed = Speed;
		}
	}

	// 대상에게 길찾기 이동을 건다. 이미 이동 중이면 그대로 둔다(MoveToActor는 움직이는 대상을 따라간다)
	bool MoveTowardTarget(AEnemyAIController* Controller, float AcceptanceGap)
	{
		AActor* Target = Controller->GetCombatTarget();
		if (!Target)
		{
			return false;
		}
		if (Controller->GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			return true;
		}
		// bStopOnOverlap: 수락 거리에 양쪽 캡슐 반지름을 더해 잰다(몸 간격 기준과 맞춤)
		const EPathFollowingRequestResult::Type Result = Controller->MoveToActor(Target, AcceptanceGap, true, true, true);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("[%s] 대상에게 가는 길을 찾지 못함 (레벨에 Nav Mesh Bounds Volume이 있는지 확인)"),
				*GetNameSafe(Controller->GetPawn()));
			return false;
		}
		return true;
	}
}

////////////////////////////////////////////////////////////////////
// 조건

bool FStateTreeEnemyHasTagCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UAbilitySystemComponent* ASC = GetEnemyASC(InstanceData.Controller);
	const bool bHas = ASC && ASC->HasAnyMatchingGameplayTags(InstanceData.Tags);
	return bInvert ? !bHas : bHas;
}

bool FStateTreeEnemyHasTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHas = InstanceData.Controller && InstanceData.Controller->GetCombatTarget() != nullptr;
	return bInvert ? !bHas : bHas;
}

bool FStateTreeEnemyTargetFartherThanCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AEnemyAIController* Controller = InstanceData.Controller;
	if (!Controller || !Controller->GetCombatTarget())
	{
		return false;
	}

	const UEnemyPatternDataAsset* Data = Controller->GetPatternData();
	const float Limit = Distance == EEnemyEngageDistance::StrafeDistance ? Data->StrafeDistance : Data->MaxStrafeDistance;
	const bool bFarther = Controller->GetTargetGap() > Limit;
	return bInvert ? !bFarther : bFarther;
}

////////////////////////////////////////////////////////////////////
// Approach

EStateTreeRunStatus FStateTreeEnemyApproachTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	if (!Controller || !Controller->GetCombatTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	const UEnemyPatternDataAsset* Data = Controller->GetPatternData();
	SetMoveSpeed(Controller, Data->ApproachSpeed);
	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 다가감 (몸 간격 %.0f → %.0f)"), *GetNameSafe(Controller->GetPawn()), Controller->GetTargetGap(), Data->StrafeDistance);

	return MoveTowardTarget(Controller, Data->StrafeDistance) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FStateTreeEnemyApproachTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	if (!Controller || !Controller->GetCombatTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	const UEnemyPatternDataAsset* Data = Controller->GetPatternData();
	if (Controller->GetTargetGap() <= Data->StrafeDistance)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 경로가 끝났는데 아직 멀면(대상이 움직임) 다시 건다
	return MoveTowardTarget(Controller, Data->StrafeDistance) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

void FStateTreeEnemyApproachTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.Controller)
	{
		InstanceData.Controller->StopMovement();
	}
}

////////////////////////////////////////////////////////////////////
// Strafe

EStateTreeRunStatus FStateTreeEnemyStrafeTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	if (!Controller || !Controller->GetCombatTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 방향과 시간은 시작할 때 정하고 끝까지 유지한다 (7-2 질문 3)
	const UEnemyPatternDataAsset* Data = Controller->GetPatternData();
	InstanceData.Direction = FMath::RandBool() ? 1.f : -1.f;
	InstanceData.RemainingTime = FMath::FRandRange(Data->StrafeDurationMin, FMath::Max(Data->StrafeDurationMin, Data->StrafeDurationMax));
	InstanceData.FlipCooldown = 0.f;
	SetMoveSpeed(Controller, Data->StrafeSpeed);

	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 맴돌기 %s, %.1f초 (몸 간격 %.0f)"), *GetNameSafe(Controller->GetPawn()),
		InstanceData.Direction > 0.f ? TEXT("오른쪽") : TEXT("왼쪽"), InstanceData.RemainingTime, Controller->GetTargetGap());
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeEnemyStrafeTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	AEnemyCharacter* Enemy = GetEnemy(Controller);
	AActor* Target = Controller ? Controller->GetCombatTarget() : nullptr;
	if (!Enemy || !Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.RemainingTime -= DeltaTime;
	if (InstanceData.RemainingTime <= 0.f)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	InstanceData.FlipCooldown = FMath::Max(0.f, InstanceData.FlipCooldown - DeltaTime);

	const UEnemyPatternDataAsset* Data = Controller->GetPatternData();
	const FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return EStateTreeRunStatus::Running;
	}

	// 접선(대상 둘레) 방향. Up × 대상 방향 = 대상을 볼 때 오른쪽
	FVector Tangent = FVector::CrossProduct(FVector::UpVector, ToTarget) * InstanceData.Direction;

	// 막히기 전에 방향을 바꾼다 (질문 4 C): 진행 방향 앞을 몸 크기 구로 짧게 쓸어 본다
	if (InstanceData.FlipCooldown <= 0.f && Data->StrafeBlockCheckDistance > 0.f)
	{
		const float Radius = Enemy->GetSimpleCollisionRadius();
		const FVector Start = Enemy->GetActorLocation();
		const FVector End = Start + Tangent * (Radius + Data->StrafeBlockCheckDistance);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyStrafeBlockCheck), false, Enemy);
		Params.AddIgnoredActor(Target);
		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		FHitResult Hit;
		if (Enemy->GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius * 0.9f), Params))
		{
			InstanceData.Direction *= -1.f;
			InstanceData.FlipCooldown = 0.5f;
			Tangent *= -1.f;
			UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 맴돌기 방향 전환 (앞에 %s)"), *Enemy->GetName(), *GetNameSafe(Hit.GetActor()));
		}
	}

	// 맴돌 거리를 유지하도록 안팎으로 보정한다. 1m 벗어나면 보정이 접선만큼 커진다
	const float RadialError = Controller->GetTargetGap() - Data->StrafeDistance;
	const float RadialWeight = FMath::Clamp(RadialError / 100.f, -1.f, 1.f);
	const FVector MoveDirection = (Tangent + ToTarget * RadialWeight).GetSafeNormal2D();

	Enemy->AddMovementInput(MoveDirection, 1.f);
	return EStateTreeRunStatus::Running;
}

void FStateTreeEnemyStrafeTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.Controller)
	{
		InstanceData.Controller->StopMovement();
	}
}

////////////////////////////////////////////////////////////////////
// Select Pattern (7-4)

FStateTreeEnemySelectPatternTask::FStateTreeEnemySelectPatternTask()
{
	// 들어갈 때 한 번 고르고 결과만 들고 있는다. 매 틱 할 일이 없고, 상태 완료는 같은 상태의 Enemy Attack이 정한다
	bShouldCallTick = false;
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FStateTreeEnemySelectPatternTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.PatternIndex = INDEX_NONE;

	const AEnemyAIController* Controller = InstanceData.Controller;
	const UEnemyPatternDataAsset* Data = Controller ? Controller->GetPatternData() : nullptr;
	const UWorld* World = Controller ? Controller->GetWorld() : nullptr;
	if (!Data || !World || !Controller->GetCombatTarget())
	{
		return EStateTreeRunStatus::Failed;
	}
	if (Data->Patterns.IsEmpty())
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] AI: 패턴 데이터 %s에 패턴이 없음"), *GetNameSafe(Controller->GetPawn()), *GetNameSafe(Data));
		return EStateTreeRunStatus::Failed;
	}

	// 유틸리티 점수 (질문 4 C): 가중치 × 거리 × 반복 방지. 각 요소는 0~1(가중치만 크기)
	const float Gap = Controller->GetTargetGap();
	const double Now = World->GetTimeSeconds();
	const int32 Num = Data->Patterns.Num();

	TArray<float> WeightScores, DistanceScores, RepeatScores;
	WeightScores.SetNumZeroed(Num);
	DistanceScores.SetNumZeroed(Num);
	RepeatScores.SetNumZeroed(Num);
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const FEnemyAttackPattern& Pattern = Data->Patterns[Index];
		if (!Pattern.StartAttack)
		{
			continue; // 시작 공격이 없는 패턴은 후보에서 뺀다 (가중치 0)
		}
		WeightScores[Index] = FMath::Max(0.f, Pattern.Weight);
		DistanceScores[Index] = (Gap >= Pattern.MinGap && Gap <= Pattern.MaxGap) ? 1.f : 0.f;

		const double LastUsed = Controller->GetPatternLastUsedTime(Index);
		RepeatScores[Index] = (LastUsed < 0.0 || Pattern.RepeatCooldown <= 0.f)
			? 1.f
			: FMath::Clamp(static_cast<float>((Now - LastUsed) / Pattern.RepeatCooldown), 0.f, 1.f);
	}

	// 점수가 모두 0이면 공격을 거르지 않도록 요소를 하나씩 빼고 다시 센다: 거리 → 반복 방지 순
	auto ScoreOf = [&](int32 Index, bool bUseDistance, bool bUseRepeat)
	{
		return WeightScores[Index] * (bUseDistance ? DistanceScores[Index] : 1.f) * (bUseRepeat ? RepeatScores[Index] : 1.f);
	};
	bool bUseDistance = true;
	bool bUseRepeat = true;
	auto Total = [&]()
	{
		float Sum = 0.f;
		for (int32 Index = 0; Index < Num; ++Index)
		{
			Sum += ScoreOf(Index, bUseDistance, bUseRepeat);
		}
		return Sum;
	};

	float TotalScore = Total();
	const TCHAR* Fallback = TEXT("");
	if (TotalScore <= 0.f)
	{
		bUseDistance = false;
		TotalScore = Total();
		Fallback = TEXT(", 거리에 맞는 패턴 없음 → 거리 무시");
	}
	if (TotalScore <= 0.f)
	{
		bUseRepeat = false;
		TotalScore = Total();
		Fallback = TEXT(", 모두 반복 방지 중 → 가중치만");
	}
	if (TotalScore <= 0.f)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] AI: 고를 수 있는 패턴이 없음 (시작 공격과 가중치 확인)"), *GetNameSafe(Controller->GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	// 점수에 비례해 무작위 (질문 5 B)
	float Pick = FMath::FRandRange(0.f, TotalScore);
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const float Score = ScoreOf(Index, bUseDistance, bUseRepeat);
		if (Score <= 0.f)
		{
			continue;
		}
		InstanceData.PatternIndex = Index;
		Pick -= Score;
		if (Pick <= 0.f)
		{
			break;
		}
	}

	// 로그: 고른 패턴과 후보별 점수(확률)
	FString Candidates;
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const float Score = ScoreOf(Index, bUseDistance, bUseRepeat);
		Candidates += FString::Printf(TEXT("%s%s %.0f%% (가중치 %.1f × 거리 %.0f × 반복 %.2f)"), Index > 0 ? TEXT(", ") : TEXT(""),
			*Data->Patterns[Index].Name.ToString(), Score / TotalScore * 100.f, WeightScores[Index], DistanceScores[Index], RepeatScores[Index]);
	}
	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 패턴 선택 %s (몸 간격 %.0f%s) 후보: %s"), *GetNameSafe(Controller->GetPawn()),
		*Data->Patterns[InstanceData.PatternIndex].Name.ToString(), Gap, Fallback, *Candidates);

	return EStateTreeRunStatus::Running;
}

////////////////////////////////////////////////////////////////////
// Attack

namespace
{
	const FEnemyAttackPattern* GetPattern(const AEnemyAIController* Controller, int32 PatternIndex)
	{
		const UEnemyPatternDataAsset* Data = Controller ? Controller->GetPatternData() : nullptr;
		return Data && Data->Patterns.IsValidIndex(PatternIndex) ? &Data->Patterns[PatternIndex] : nullptr;
	}

	// 콤보 입력 (7-4). 콤보 구간이 열리는 순간 이어 갈지 정하고, 지연 뒤 입력 태그를 하나 보낸다
	void UpdateComboInput(FStateTreeEnemyAttackTaskInstanceData& InstanceData, const FEnemyAttackPattern& Pattern,
		const UEnemyPatternDataAsset& Data, AEnemyCharacter& Enemy, const UAbilitySystemComponent& ASC, float DeltaTime)
	{
		if (InstanceData.bComboStopped || InstanceData.NextComboInput >= Pattern.ComboInputs.Num())
		{
			return;
		}

		const bool bWindowOpen = ASC.HasMatchingGameplayTag(CombatTags::Window_Combo);
		const FGameplayTag& InputTag = Pattern.ComboInputs[InstanceData.NextComboInput];

		// 구간이 열리는 순간: 이어 갈지 보고(질문 3), 이어 가면 지연을 고른다(질문 2 B)
		if (bWindowOpen && !InstanceData.bComboWindowOpen)
		{
			const bool bConnected = Enemy.GetAttackConnectCount() > InstanceData.ConnectCountAtSwing;
			if (Data.ComboContinueRule == EEnemyComboContinueRule::OnHitOrGuard && !bConnected)
			{
				InstanceData.bComboStopped = true;
				UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 콤보 멈춤 (헛침, %s 보내지 않음)"), *Enemy.GetName(), *InputTag.ToString());
			}
			else
			{
				InstanceData.InputDelayRemaining = FMath::FRandRange(Data.ComboInputDelayMin, FMath::Max(Data.ComboInputDelayMin, Data.ComboInputDelayMax));
			}
		}

		if (bWindowOpen && InstanceData.InputDelayRemaining >= 0.f)
		{
			// 히트스톱·슬로모션 중에는 몽타주(구간)도 느려지므로 지연도 같은 배율로 센다
			InstanceData.InputDelayRemaining -= DeltaTime * Enemy.CustomTimeDilation;
			if (InstanceData.InputDelayRemaining <= 0.f)
			{
				InstanceData.InputDelayRemaining = -1.f;
				InstanceData.ConnectCountAtSwing = Enemy.GetAttackConnectCount();
				++InstanceData.NextComboInput;
				UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 콤보 입력 %s (%d/%d)"), *Enemy.GetName(), *InputTag.ToString(),
					InstanceData.NextComboInput, Pattern.ComboInputs.Num());

				// 플레이어 버튼과 같은 통로: 눌렀다 뗀다 (규칙 1). 다음 공격은 GA_MeleeAttack이 ComboLinks로 고른다
				Enemy.AbilityInputTagPressed(InputTag);
				Enemy.AbilityInputTagReleased(InputTag);
			}
		}
		else if (!bWindowOpen && InstanceData.bComboWindowOpen && InstanceData.InputDelayRemaining >= 0.f)
		{
			// 지연보다 구간이 먼저 닫혔다: 콤보를 놓친다
			InstanceData.InputDelayRemaining = -1.f;
			InstanceData.bComboStopped = true;
			UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 콤보 입력 놓침 (지연보다 구간이 먼저 닫힘, %s)"), *Enemy.GetName(), *InputTag.ToString());
		}

		InstanceData.bComboWindowOpen = bWindowOpen;
	}
}

EStateTreeRunStatus FStateTreeEnemyAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	InstanceData.bAttackStarted = false;
	InstanceData.NextComboInput = 0;
	InstanceData.bComboWindowOpen = false;
	InstanceData.InputDelayRemaining = -1.f;
	InstanceData.ConnectCountAtSwing = 0;
	InstanceData.bComboStopped = false;

	if (!Controller || !Controller->GetCombatTarget() || !GetEnemy(Controller))
	{
		return EStateTreeRunStatus::Failed;
	}

	const FEnemyAttackPattern* Pattern = GetPattern(Controller, InstanceData.PatternIndex);
	if (!Pattern || !Pattern->StartAttack)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] AI: Enemy Attack의 패턴 %d이 없거나 시작 공격이 비어 있음"),
			*GetNameSafe(Controller->GetPawn()), InstanceData.PatternIndex);
		return EStateTreeRunStatus::Failed;
	}

	SetMoveSpeed(Controller, Controller->GetPatternData()->ApproachSpeed);
	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 공격하러 들어감 %s (몸 간격 %.0f → 공격 거리 %.0f)"), *GetNameSafe(Controller->GetPawn()),
		*Pattern->Name.ToString(), Controller->GetTargetGap(), Pattern->StartAttack->MaxWarpDistance);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeEnemyAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	AEnemyCharacter* Enemy = GetEnemy(Controller);
	UAbilitySystemComponent* ASC = GetEnemyASC(Controller);
	const FEnemyAttackPattern* Pattern = GetPattern(Controller, InstanceData.PatternIndex);
	if (!Enemy || !ASC || !Pattern || !Pattern->StartAttack)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 공격 중: 콤보 입력을 보내고, State.Attacking이 떨어지면(콤보 전체가 끝나면) 성공
	if (InstanceData.bAttackStarted)
	{
		if (!ASC->HasMatchingGameplayTag(CombatTags::State_Attacking))
		{
			return EStateTreeRunStatus::Succeeded;
		}
		UpdateComboInput(InstanceData, *Pattern, *Controller->GetPatternData(), *Enemy, *ASC, DeltaTime);
		return EStateTreeRunStatus::Running;
	}

	if (!Controller->GetCombatTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 공격 거리 밖: 걸어 들어간다 (7-3 질문 5 A). 공격 거리 = 시작 공격의 워핑 최대 거리, 나머지는 워핑이 메운다
	const float Reach = Pattern->StartAttack->MaxWarpDistance;
	if (Controller->GetTargetGap() > Reach)
	{
		return MoveTowardTarget(Controller, Reach) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
	}

	// 공격 거리 안: 멈추고 요청만 보낸다 (규칙 1, 7). 막혀 있으면(경직 등) 어빌리티가 시작되지 않는다
	Controller->StopMovement();
	Enemy->RequestAttack(Pattern->StartAttack);
	if (!ASC->HasMatchingGameplayTag(CombatTags::State_Attacking))
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 공격 요청이 시작되지 않음 %s"), *Enemy->GetName(), *Pattern->StartAttack->GetName());
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.bAttackStarted = true;
	InstanceData.ConnectCountAtSwing = Enemy->GetAttackConnectCount();
	Controller->MarkPatternUsed(InstanceData.PatternIndex);
	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 공격 요청 %s: %s + 입력 %d개 (몸 간격 %.0f)"), *Enemy->GetName(), *Pattern->Name.ToString(),
		*Pattern->StartAttack->GetName(), Pattern->ComboInputs.Num(), Controller->GetTargetGap());
	return EStateTreeRunStatus::Running;
}

void FStateTreeEnemyAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 이동만 멈춘다. 이미 시작한 공격은 어빌리티가 끝까지 책임진다(경직 등으로 끊기는 것도 어빌리티 쪽 규칙)
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.Controller && !InstanceData.bAttackStarted)
	{
		InstanceData.Controller->StopMovement();
	}
}

////////////////////////////////////////////////////////////////////
// Recover

EStateTreeRunStatus FStateTreeEnemyRecoverTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Controller)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.Controller->StopMovement();
	InstanceData.RemainingTime = InstanceData.Controller->GetPatternData()->RecoverTime;
	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 회복 %.1f초"), *GetNameSafe(InstanceData.Controller->GetPawn()), InstanceData.RemainingTime);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeEnemyRecoverTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingTime -= DeltaTime;
	return InstanceData.RemainingTime <= 0.f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

////////////////////////////////////////////////////////////////////
// 에디터 설명

#if WITH_EDITOR
FText FStateTreeEnemyHasTagCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	const FString TagText = InstanceData ? InstanceData->Tags.ToStringSimple() : FString();
	return FText::FromString(FString::Printf(TEXT("%sHas Tag: %s"), bInvert ? TEXT("Not ") : TEXT(""), *TagText));
}

FText FStateTreeEnemyHasTargetCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(bInvert ? TEXT("No Target") : TEXT("Has Target"));
}

FText FStateTreeEnemyTargetFartherThanCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const TCHAR* DistanceText = Distance == EEnemyEngageDistance::StrafeDistance ? TEXT("Strafe Distance") : TEXT("Max Strafe Distance");
	return FText::FromString(FString::Printf(TEXT("Target %s Than %s"), bInvert ? TEXT("Not Farther") : TEXT("Farther"), DistanceText));
}

FText FStateTreeEnemyApproachTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("Approach To Strafe Distance"));
}

FText FStateTreeEnemyStrafeTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("Strafe Around Target"));
}

FText FStateTreeEnemySelectPatternTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("Select Pattern (Utility)"));
}

FText FStateTreeEnemyAttackTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("Attack With Pattern"));
}

FText FStateTreeEnemyRecoverTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("Recover"));
}
#endif
