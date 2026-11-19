// 적 AI StateTree 조건과 태스크 (Step 7-3 2/2)
// 템플릿 CombatStateTreeUtility의 자리를 대신한다. 모든 노드는 컨텍스트로 AEnemyAIController 하나를 받는다
// (StateTree 에셋 Schema의 AI Controller Class를 EnemyAIController 계열로 두면 자동으로 연결된다)
//
// 7-3 결정
//   - 몸 상태는 조건으로 본다: ASC 태그 (질문 1 A). 경직 상태는 State.Staggered / Downed / Groggy 묶음, 움찔 제외 (2/2 질문 1 A)
//   - 맴돌기는 대상 둘레 옆걸음, 막히기 전에 방향을 바꾼다 (2/2 질문 3 B, 질문 4 C)
//   - 공격은 걸어 들어가 공격의 MaxWarpDistance 안에서 요청한다. 나머지는 워핑이 메운다 (2/2 질문 5 A)
//   - 값은 모두 패턴 데이터에서 읽는다 (질문 5 A)
//   - 규칙 1, 7: 공격 태스크는 공격 요청(Event.AttackRequest)과 입력 태그만 보낸다. 실행은 플레이어와 같은 GA_MeleeAttack
//
// 7-4 결정
//   - 패턴 = 시작 공격 + 이어 갈 입력 태그 순서. 콤보 분기는 ComboLinks가 정한다 (질문 1 B)
//   - 콤보 입력은 구간이 열린 뒤 적마다의 무작위 지연 뒤에 보낸다 (질문 2 B)
//   - 이어 가는 조건은 적마다: 일반형은 끝까지, 엘리트는 맞거나 막히면 잇고 헛치면 멈춤 (질문 3)
//   - 패턴 점수 = 가중치 × 거리 × 반복 방지, 점수에 비례해 무작위로 고른다 (질문 4 C, 질문 5 B). 대상 상태 점수는 배포판에서

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "EnemyStateTreeNodes.generated.h"

class AEnemyAIController;

////////////////////////////////////////////////////////////////////
// 조건

USTRUCT()
struct FStateTreeEnemyHasTagConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller = nullptr;

	// 이 중 하나라도 적의 ASC에 있으면 참
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagContainer Tags;
};

// 적 몸 상태(ASC 태그) 확인. 경직·사망 전이에 쓴다
USTRUCT(DisplayName = "Enemy Has Tag", meta = (Category = "Enemy"))
struct FStateTreeEnemyHasTagCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnemyHasTagConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	// 참이면 결과를 뒤집는다 (태그가 없을 때 참)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

USTRUCT()
struct FStateTreeEnemyControllerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller = nullptr;
};

// 대상이 있는가 (컨트롤러가 인지로 잡은 대상, 7-3 1/2)
USTRUCT(DisplayName = "Enemy Has Target", meta = (Category = "Enemy"))
struct FStateTreeEnemyHasTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnemyControllerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// 비교할 패턴 데이터 거리
UENUM()
enum class EEnemyEngageDistance : uint8
{
	StrafeDistance,     // 맴돌 거리
	MaxStrafeDistance,  // 다시 다가갈 거리
};

// 대상과의 몸 간격이 패턴 데이터의 거리보다 먼가
USTRUCT(DisplayName = "Enemy Target Farther Than", meta = (Category = "Enemy"))
struct FStateTreeEnemyTargetFartherThanCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnemyControllerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EEnemyEngageDistance Distance = EEnemyEngageDistance::StrafeDistance;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// 태스크

// 대상에게 맴돌 거리까지 다가간다 (길찾기 MoveTo, 내비메시 필요). 도착하면 성공, 대상이 없거나 길이 없으면 실패
USTRUCT(meta = (DisplayName = "Enemy Approach", Category = "Enemy"))
struct FStateTreeEnemyApproachTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnemyControllerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

USTRUCT()
struct FStateTreeEnemyStrafeTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller = nullptr;

	// 실행 중 상태: 도는 방향(대상을 볼 때 +1 오른쪽, -1 왼쪽), 남은 시간, 방향 전환 뒤 다시 바꾸기까지의 시간
	float Direction = 1.f;
	float RemainingTime = 0.f;
	float FlipCooldown = 0.f;
};

// 대상을 바라본 채 맴돌 거리를 유지하며 옆걸음으로 돈다. 시간이 끝나면 성공(7-3: 다음은 공격)
USTRUCT(meta = (DisplayName = "Enemy Strafe", Category = "Enemy"))
struct FStateTreeEnemyStrafeTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnemyStrafeTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

USTRUCT()
struct FStateTreeEnemySelectPatternTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller = nullptr;

	// 고른 패턴 번호 (패턴 데이터 Patterns 순서). 같은 상태의 Enemy Attack의 Pattern Index에 바인딩한다
	UPROPERTY(EditAnywhere, Category = "Output")
	int32 PatternIndex = INDEX_NONE;
};

// 유틸리티 점수로 공격 패턴을 고른다 (7-4). 점수 = 가중치 × 거리 × 반복 방지, 점수에 비례해 무작위 (질문 4 C, 질문 5 B)
// 상태에 들어갈 때 한 번 고르고, 상태가 끝날 때까지 결과를 들고 있는다. 상태 완료에는 관여하지 않는다(같은 상태의 Enemy Attack이 끝내야 끝난다)
USTRUCT(meta = (DisplayName = "Enemy Select Pattern", Category = "Enemy"))
struct FStateTreeEnemySelectPatternTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FStateTreeEnemySelectPatternTask();

	using FInstanceDataType = FStateTreeEnemySelectPatternTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

USTRUCT()
struct FStateTreeEnemyAttackTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller = nullptr;

	// 실행할 패턴 번호 (패턴 데이터 Patterns 순서). Enemy Select Pattern의 Pattern Index에 바인딩한다 (7-4)
	// 바인딩하지 않고 값을 넣으면 그 패턴만 쓴다(시험용). Category "Input"은 바인딩 전용이라 "Parameter"로 둔다 (7-3에서 발견)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 PatternIndex = 0;

	// 실행 중 상태
	bool bAttackStarted = false;     // 공격을 요청했는가
	int32 NextComboInput = 0;        // 다음에 보낼 콤보 입력 순서
	bool bComboWindowOpen = false;   // 지난 틱에 콤보 구간이 열려 있었는가 (구간이 열리는 순간을 알기 위해)
	float InputDelayRemaining = -1.f; // 콤보 입력까지 남은 지연. 음수면 기다리는 입력 없음
	int32 ConnectCountAtSwing = 0;   // 이번 휘두름이 시작될 때 공격이 닿은 횟수 (헛침 판단)
	bool bComboStopped = false;      // 헛치거나 구간을 놓쳐 콤보를 멈췄는가
};

// 패턴의 시작 공격 거리(MaxWarpDistance) 안까지 걸어 들어가 공격을 요청하고,
// 콤보 구간마다 패턴의 입력 태그를 지연 뒤 하나씩 보낸다 (7-4). 공격이 끝나면 성공
USTRUCT(meta = (DisplayName = "Enemy Attack", Category = "Enemy"))
struct FStateTreeEnemyAttackTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnemyAttackTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

USTRUCT()
struct FStateTreeEnemyRecoverTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEnemyAIController> Controller = nullptr;

	float RemainingTime = 0.f;
};

// 공격 뒤 제자리에서 대상을 바라보며 패턴 데이터의 회복 시간만큼 쉰다
USTRUCT(meta = (DisplayName = "Enemy Recover", Category = "Enemy"))
struct FStateTreeEnemyRecoverTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnemyRecoverTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
