// 적 패턴 데이터. 적 종류 하나 = 이 데이터 한 장 (7-3 결정: AI 성향 값과 패턴 목록을 한 장에)
// 7-3: 인지와 반응(얼마나 멀리 보고, 놓친 대상에 얼마나 빨리 돌아서는지)
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
};
