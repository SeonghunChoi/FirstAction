// 적 패턴 데이터. 적 종류 하나 = 이 데이터 한 장 (7-3 결정: AI 성향 값과 패턴 목록을 한 장에)
// 7-3: 인지와 반응(얼마나 멀리 보고, 놓친 대상에 얼마나 빨리 돌아서는지), 교전 흐름(다가가기, 맴돌기, 회복)
// 7-4: 패턴 목록(어떤 공격을 어느 거리에서)을 추가한다. 칸은 쓰는 스텝에서 하나씩 추가한다
// Step 8-1 엘리트 적은 이 데이터 한 장과 공격 데이터만 새로 만든다 (C++ 변경 0줄)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyPatternDataAsset.generated.h"

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
};
