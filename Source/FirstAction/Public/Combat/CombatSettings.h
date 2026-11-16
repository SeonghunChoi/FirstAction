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
};
