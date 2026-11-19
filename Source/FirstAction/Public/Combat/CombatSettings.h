// 전투의 "세계 규칙" 튜닝값. 특정 공격이나 특정 캐릭터의 값이 아닌 것만 둔다. (Step 6-4)
// Project Settings에 "Combat" 항목으로 나타나고 Config/DefaultGame.ini에 저장된다.
// 타격마다 GetDefault<UCombatSettings>()로 읽으므로 Play 중에 바꿔도 다음 타격부터 반영된다 (6-6 튜닝용).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CombatSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Combat"))
class FIRSTACTION_API UCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 상대 충격량(cm/s) → 맞은 쪽이 밀리는 거리(cm)의 비례 계수. 단위는 초
	// "충격으로 얻은 속도로 이만큼의 시간 동안 미끄러진 거리"로 읽는다. 0이면 밀리지 않는다 (6-4 결정: 단순 비례)
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Hit Resolve", meta = (ClampMin = "0.0", Units = "s"))
	float KnockbackDistancePerImpulse = 0.f;

	// 상대 충격량(cm/s) → 경직치 감소량의 비례 계수. 0이면 경직치가 줄지 않는다
	// 공격 데이터의 임시 칸 PoiseDamage를 대신한다 (3-1, 5-4 결정)
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Hit Resolve", meta = (ClampMin = "0.0"))
	float PoiseDamagePerImpulse = 0.f;

	// 반응 단계 경계 (상대 충격량, cm/s). 모든 몸이 같은 기준을 쓰고, 버티는 정도는 질량이 정한다 (6-7 결정: 세계 규칙)
	//   0 < I < StaggerImpulseThreshold          → 움찔
	//   StaggerImpulseThreshold ≤ I < Knockdown  → 경직
	//   KnockdownImpulseThreshold ≤ I            → 넘어짐
	// 표시 단위가 m/s로 바뀌지 않도록 ForceUnits로 고정한다
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Hit Reaction", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float StaggerImpulseThreshold = 0.f;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Hit Reaction", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float KnockdownImpulseThreshold = 0.f;

	// 부위 물리 반응 (6-8 결정: 세계 규칙, 반응 단계 경계와 별개)
	//   맞은 바디의 속도 변화(cm/s) = (상대 충격량 − PartialReactionImpulseThreshold) × PartialReactionVelocityPerImpulse
	//   방향은 칼이 지나간 방향. 임계값 이하이거나 계수가 0이면 반응 없음
	// 초과분에 비례하므로 임계값을 막 넘긴 타격은 약하게 흔들린다 (임계값에서 갑자기 튀지 않음)
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Partial Reaction", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float PartialReactionImpulseThreshold = 0.f;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Partial Reaction", meta = (ClampMin = "0.0"))
	float PartialReactionVelocityPerImpulse = 0.f;

	// 패리 순간의 히트스톱 배율. 공격 데이터의 HitStopDuration에 곱해 양쪽에 건다 (6-12 결정: 패리는 더 길게)
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "1.0"))
	float ParryHitStopMultiplier = 1.f;

	// 저스트 회피 보상: 회피한 쪽을 뺀 모든 전투 캐릭터의 시간 배율과 지속 시간 (6-13 결정: 슬로모션, 세계 규칙)
	// 지속 시간은 실제 시간(초). 히트스톱과 겹치면 캐릭터가 더 느린 쪽을 적용한다
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float JustDodgeTimeDilation = 1.f;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0.0", Units = "s"))
	float JustDodgeSlowDuration = 0.f;

	// 공격 토큰 우선순위의 각도 벌점 (7-5 질문 4 C). 단위 cm/도
	// 우선순위 비용 = 대상과의 몸 간격(cm) + 대상 시선에서 벗어난 각도(도) × 이 값. 작은 쪽이 먼저 토큰을 받는다
	// 예: 3이면 등 뒤(180°)의 적은 정면의 적보다 5.4m 먼 것과 같다
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Attack Tokens", meta = (ClampMin = "0.0"))
	float AttackTokenAnglePenalty = 0.f;
};
