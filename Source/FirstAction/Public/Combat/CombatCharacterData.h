// 캐릭터 한 종류의 몸의 성질을 담는 데이터. 플레이어와 적이 같은 형식을 쓴다. (PLAN 4.6, 설계 규칙 6-3)
// 조작감 값(입력 버퍼, 히트스톱 배율, 대상 원뿔)은 여기가 아니라 캐릭터 블루프린트 기본값에 둔다(6-3 결정).
// 칸은 쓰는 스텝에서 하나씩 추가한다: 부위 반응 임계값(6-8), 반응 단계 기준과 반응 몽타주(6-7).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatCharacterData.generated.h"

UCLASS(BlueprintType, Const)
class FIRSTACTION_API UCombatCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 질량 (kg). 타격 후 간격을 때린 쪽과 맞은 쪽이 나눠 가지는 비율과 상대 충격량(충격 ÷ 맞는 쪽 질량)에 쓴다 (6-4)
	// 이 값이 정본이다. 캐릭터가 시작할 때 캐릭터 무브먼트의 Mass에 복사한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "1.0", Units = "kg"))
	float Mass = 100.f;
};
