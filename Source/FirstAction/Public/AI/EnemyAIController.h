// 적 AI 컨트롤러 (Step 7-3). 템플릿 ACombatAIController 자리를 대신한다
// 1/2: 인지와 바라보기. 2/2: 무엇을 할지(접근, 맴돌기, 공격, 회복)는 StateTree가 정한다 (UStateTreeAIComponent, 트리 에셋은 BP에서)
//
// 대상 (7-3 결정)
//   - AI Perception 시각 감지로 "보인 적"(팀 태도 Hostile)을 대상으로 잡는다 (질문 2 B)
//   - 맞으면 Damage 감각으로 때린 쪽을 즉시 잡는다 (질문 4 C)
//   - 시야에서 사라지면 반응 시간 동안 마지막으로 본 위치를 보고, 지나면 돌아선다 (질문 4 C)
//     돌아선 뒤 근접 감지 거리 안이면 대상의 지금 위치를, 밖이면 마지막으로 안 위치를 본다 (질문 6 B, 질문 7 B)
//   - 보지도 맞지도 않은 채 기억 시간이 지나면 대상을 놓는다 (질문 3 B)
//   - 값은 폰의 패턴 데이터(UEnemyPatternDataAsset)에서 읽는다 (질문 5 A)
// 몸을 돌리는 것은 컨트롤러의 Focus → 캐릭터 무브먼트 bUseControllerDesiredRotation(회전 속도는 BP의 Rotation Rate)

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class UEnemyPatternDataAsset;
class UStateTreeAIComponent;

UCLASS()
class FIRSTACTION_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaSeconds) override;

	// 지금 쫓는 대상. 없으면 nullptr. StateTree(2/2)와 공격 워핑 대상(6-2 결정)이 쓴다
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }

	// 대상이 지금 시야 안에 있는가
	bool IsCombatTargetVisible() const { return bTargetVisible; }

	// 대상과의 몸 간격 (중심 거리 − 양쪽 캡슐 반지름). 워핑·공격 대상 탐색과 같은 기준. 대상이 없으면 아주 큰 값
	float GetTargetGap() const { return GetBodyGap(CombatTarget.Get()); }

	// 몸 간격 (중심 거리 − 양쪽 캡슐 반지름)
	float GetBodyGap(const AActor* Target) const;

	// 폰의 패턴 데이터. 지정하지 않았으면 클래스 기본값
	const UEnemyPatternDataAsset* GetPatternData() const;

	// 패턴 반복 방지 기록 (7-4). 패턴 번호(패턴 데이터 Patterns 순서)별로 마지막으로 쓴 게임 시간. 쓴 적이 없으면 음수
	double GetPatternLastUsedTime(int32 PatternIndex) const;
	void MarkPatternUsed(int32 PatternIndex);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// 무엇을 할지 고르는 StateTree. 트리 에셋은 블루프린트 자식(BP_EnemyAIController)에서 지정한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// 폰의 패턴 데이터로 시각 감지를 맞추고, 빙의할 때 받은 팀으로 인지를 다시 등록한다
	void ApplyPatternData();

	void SetCombatTarget(AActor* NewTarget, const TCHAR* Reason);
	void ClearCombatTarget(const TCHAR* Reason);
	void UpdateFocus();
	void DrawDebug() const;

	bool IsPawnDead() const;

	TWeakObjectPtr<AActor> CombatTarget;

	// 대상이 시야 안에 있는가 (시각 감지 기준)
	bool bTargetVisible = false;

	// 시야에서 사라진 뒤 반응 시간이 지났거나 맞아서 돌아선 상태인가
	bool bReactedToLostTarget = false;

	// 돌아선 뒤, 근접 감지 거리 안이라 대상의 지금 위치를 아는가
	bool bTargetNearby = false;

	// 마지막으로 보거나, 맞거나, 가까이서 안 시각 (기억 시간 기준)
	double LastSensedTime = 0.0;

	// 시야에서 사라진 시각 (반응 시간 기준)
	double LostSightTime = 0.0;

	// 마지막으로 안 위치: 마지막으로 본 위치, 또는 가까이서 마지막으로 안 위치. 반응 대기 중과 멀어진 뒤 이곳을 본다
	FVector LastKnownLocation = FVector::ZeroVector;

	// 패턴 번호 → 마지막으로 쓴 게임 시간 (반복 방지 점수, 7-4)
	TMap<int32, double> PatternLastUsedTimes;
};
