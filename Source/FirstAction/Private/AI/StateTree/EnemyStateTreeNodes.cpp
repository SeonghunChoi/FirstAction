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
// Attack

EStateTreeRunStatus FStateTreeEnemyAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	InstanceData.bAttackStarted = false;

	if (!Controller || !Controller->GetCombatTarget() || !GetEnemy(Controller))
	{
		return EStateTreeRunStatus::Failed;
	}
	if (!InstanceData.Attack)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[%s] AI: Enemy Attack 태스크에 Attack이 없음"), *GetNameSafe(Controller->GetPawn()));
		return EStateTreeRunStatus::Failed;
	}

	SetMoveSpeed(Controller, Controller->GetPatternData()->ApproachSpeed);
	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 공격하러 들어감 %s (몸 간격 %.0f → 공격 거리 %.0f)"), *GetNameSafe(Controller->GetPawn()),
		*InstanceData.Attack->GetName(), Controller->GetTargetGap(), InstanceData.Attack->MaxWarpDistance);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeEnemyAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AEnemyAIController* Controller = InstanceData.Controller;
	AEnemyCharacter* Enemy = GetEnemy(Controller);
	UAbilitySystemComponent* ASC = GetEnemyASC(Controller);
	if (!Enemy || !ASC || !InstanceData.Attack)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 공격 중: State.Attacking이 떨어지면(콤보 없이 한 번) 끝
	if (InstanceData.bAttackStarted)
	{
		return ASC->HasMatchingGameplayTag(CombatTags::State_Attacking) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
	}

	if (!Controller->GetCombatTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 공격 거리 밖: 걸어 들어간다 (질문 5 A). 공격 거리 = 그 공격의 워핑 최대 거리, 나머지는 워핑이 메운다
	const float Reach = InstanceData.Attack->MaxWarpDistance;
	if (Controller->GetTargetGap() > Reach)
	{
		return MoveTowardTarget(Controller, Reach) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
	}

	// 공격 거리 안: 멈추고 요청만 보낸다 (규칙 1, 7). 막혀 있으면(경직 등) 어빌리티가 시작되지 않는다
	Controller->StopMovement();
	Enemy->RequestAttack(InstanceData.Attack);
	if (!ASC->HasMatchingGameplayTag(CombatTags::State_Attacking))
	{
		UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 공격 요청이 시작되지 않음 %s"), *Enemy->GetName(), *InstanceData.Attack->GetName());
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.bAttackStarted = true;
	UE_LOG(LogFirstAction, Log, TEXT("[%s] AI: 공격 요청 %s (몸 간격 %.0f)"), *Enemy->GetName(), *InstanceData.Attack->GetName(), Controller->GetTargetGap());
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

FText FStateTreeEnemyAttackTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	return FText::FromString(FString::Printf(TEXT("Attack: %s"), InstanceData && InstanceData->Attack ? *InstanceData->Attack->GetName() : TEXT("(none)")));
}

FText FStateTreeEnemyRecoverTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("Recover"));
}
#endif
