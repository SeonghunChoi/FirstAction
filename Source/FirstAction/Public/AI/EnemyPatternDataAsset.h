// 적 패턴 데이터. 적 종류 하나 = 이 데이터 한 장 (7-3 결정: AI 성향 값과 패턴 목록을 한 장에)
// 7-3: 인지와 반응(얼마나 멀리 보고, 놓친 대상에 얼마나 빨리 돌아서는지), 교전 흐름(다가가기, 맴돌기, 회복)
// 7-4: 공격 패턴 목록(시작 공격 + 이어 갈 입력 태그 순서)과 콤보 입력 성향(지연, 이어 가는 조건)
// Step 8-1 엘리트 적은 이 데이터 한 장과 공격 데이터만 새로 만든다 (C++ 변경 0줄)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EnemyPatternDataAsset.generated.h"

class UAttackDataAsset;

// 콤보를 이어 가는 조건 (7-4 질문 3)
UENUM(BlueprintType)
enum class EEnemyComboContinueRule : uint8
{
	// 앞 타의 결과와 상관없이 패턴의 입력을 끝까지 보낸다 (일반형)
	Always,
	// 앞 타가 맞거나 막혔을 때만 잇고, 헛치면(회피, 빗나감) 멈춘다 (엘리트)
	OnHitOrGuard,
};

// 공격 패턴 하나 = 시작 공격 + 이어 갈 입력 태그 순서 (7-4 질문 1 B)
// 다음에 무슨 공격이 나올지는 지금 공격의 ComboLinks가 정한다(규칙 4). 적은 플레이어처럼 입력만 보낸다(규칙 1)
USTRUCT(BlueprintType)
struct FEnemyAttackPattern
{
	GENERATED_BODY()

	// 로그에 쓰는 이름 (예: Light3)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern")
	FName Name;

	// 첫 공격. 적은 이 공격 데이터로 공격을 요청한다 (Event.AttackRequest, 7-1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern")
	TObjectPtr<const UAttackDataAsset> StartAttack;

	// 이어 갈 입력 태그 순서. 콤보 구간(Window.Combo)마다 하나씩 보낸다. 비우면 단타
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern", meta = (Categories = "InputTag"))
	TArray<FGameplayTag> ComboInputs;

	// 유틸리티 점수 1: 기본 가중치. 자주 쓰는 패턴은 크게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern|Score", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	// 유틸리티 점수 2: 고르는 순간 대상과의 몸 간격이 이 범위 안이면 1, 밖이면 0
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern|Score", meta = (ClampMin = "0.0", Units = "cm"))
	float MinGap = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern|Score", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxGap = 400.f;

	// 유틸리티 점수 3: 반복 방지. 쓴 직후 0에서 이 시간에 걸쳐 1로 돌아온다. 0이면 반복 방지 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pattern|Score", meta = (ClampMin = "0.0", Units = "s"))
	float RepeatCooldown = 4.f;
};

UCLASS(BlueprintType)
class FIRSTACTION_API UEnemyPatternDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 시각 감지: 이 거리 안에서 처음 본다 (AI Perception Sight의 Sight Radius)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float SightRadius = 1500.f;

	// 한 번 본 대상은 이 거리까지 계속 보인다 (Lose Sight Radius). SightRadius보다 크게 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float LoseSightRadius = 1800.f;

	// 시야 반각 (정면에서 한쪽으로). 이 각도 밖(등 뒤)은 보이지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float PeripheralVisionHalfAngle = 70.f;

	// 시야에서 사라진 대상을 놓지 않고 기억하는 시간. 맞아도 다시 센다. 지나면 대상을 놓고 Idle로 (7-3 결정 B)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "s"))
	float TargetMemoryTime = 5.f;

	// 시야에서 사라진 대상에게 돌아서기까지의 시간. 그동안은 마지막으로 본 위치를 보고, 지나면 돌아선다.
	// 맞으면 이 시간 없이 즉시 돈다 (7-3 질문 4 C). 일반형은 길게, 엘리트는 짧게
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "s"))
	float ReactionTime = 0.6f;

	// 근접 감지 거리(몸 간격): 시야 밖이어도 이 안에 있으면 대상의 지금 위치를 안다(반응 시간 뒤). 몸을 맞댄 근접전 기준.
	// 이보다 멀면 마지막으로 안 위치만 안다 (7-3 질문 6 B, 질문 7 B)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float ProximityAwarenessRadius = 300.f;

	// 맴돌 거리(몸 간격). Approach는 이 거리까지 다가가고, Strafe는 이 거리를 유지하며 돈다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "cm"))
	float StrafeDistance = 250.f;

	// 맴도는 중 대상이 이보다 멀어지면 다시 Approach. StrafeDistance보다 크게 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxStrafeDistance = 450.f;

	// 맴돌기 한 번의 시간 범위. 시작할 때 이 안에서 고른다 (7-3: 끝나면 공격, 7-5: 토큰을 얻으면 공격)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "s"))
	float StrafeDurationMin = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "s"))
	float StrafeDurationMax = 3.f;

	// 맴돌 때 걷는 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "cm/s"))
	float StrafeSpeed = 180.f;

	// 다가갈 때(Approach, 공격하러 들어갈 때) 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "cm/s"))
	float ApproachSpeed = 400.f;

	// 맴도는 방향 앞을 이만큼 검사해, 다른 적이나 벽이 있으면 막히기 전에 방향을 바꾼다 (7-3 질문 4 C)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "cm"))
	float StrafeBlockCheckDistance = 80.f;

	// 공격이 끝난 뒤 다시 맴돌기까지 쉬는 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Engage", meta = (ClampMin = "0.0", Units = "s"))
	float RecoverTime = 1.f;

	// 공격 패턴 목록. 공격하러 들어가기 전에 Enemy Select Pattern이 점수(가중치 × 거리 × 반복 방지)에 비례해 무작위로 하나를 고른다 (7-4)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pattern")
	TArray<FEnemyAttackPattern> Patterns;

	// 콤보 입력 지연: 콤보 구간이 열린 뒤 이 범위에서 무작위로 기다렸다가 다음 입력을 보낸다 (7-4 질문 2 B)
	// 일반형은 느슨하게, 엘리트는 빠르게. 지연보다 구간이 먼저 닫히면 콤보를 놓친다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pattern", meta = (ClampMin = "0.0", Units = "s"))
	float ComboInputDelayMin = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pattern", meta = (ClampMin = "0.0", Units = "s"))
	float ComboInputDelayMax = 0.2f;

	// 콤보를 이어 가는 조건 (7-4 질문 3). 일반형 Always, 엘리트 OnHitOrGuard
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pattern")
	EEnemyComboContinueRule ComboContinueRule = EEnemyComboContinueRule::Always;
};
