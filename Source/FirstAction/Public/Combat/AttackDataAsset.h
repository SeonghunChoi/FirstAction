// 공격 하나의 성질을 담는 데이터. "공격 하나는 데이터 한 장" (PLAN 4.3, 설계 규칙 2)
// GA_MeleeAttack은 특정 공격을 모르고, 이 데이터만 읽어서 실행한다.
// 칸은 쓰는 스텝에서 하나씩 추가한다. 이름이나 타입을 바꾸면 이미 입력한 값이 사라지기 때문이다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AttackDataAsset.generated.h"

class UAnimMontage;
class UAttackDataAsset;

// 콤보 분기 하나: 콤보 구간에서 이 입력이 들어오면 다음 공격으로 넘어간다 (Step 4-3)
USTRUCT(BlueprintType)
struct FAttackComboLink
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UAttackDataAsset> NextAttack;
};

UCLASS(BlueprintType, Const)
class FIRSTACTION_API UAttackDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 재생할 공격 몽타주 (Step 3-2)
	// 판정, 콤보, 캔슬 구간의 타이밍은 C++가 아니라 이 몽타주의 노티파이가 가진다(설계 규칙 3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage;

	// 피해량 (Step 5-3). GE_Damage에 SetByCaller.Damage로 전달된다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Damage = 0.f;

	// 충격량 (kg·cm/s, Step 6-4). 방향은 때린 쪽 → 맞은 쪽 수평
	// 타격 해석 매니저가 맞은 쪽 질량으로 나눠 상대 충격량을 만들고, 밀림 거리와 경직치 감소량을 정한다
	// (5-4의 임시 칸 PoiseDamage는 이 계산으로 대체되어 삭제했다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Impact", meta = (ClampMin = "0.0"))
	float ImpactForce = 0.f;

	// 타격 후 몸끼리의 간격 (cm, Step 6-4). 간격은 이 칸에만 있다(설계 규칙 6-1)
	// 콤보 중간 타는 짧게(다음 타 범위 안), 마무리 타는 길게
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Impact", meta = (ClampMin = "0.0", Units = "cm"))
	float PostHitSpacing = 0.f;

	// 타격 후 이동 시간 (초, Step 6-5). 양쪽이 각자의 이동량을 이 시간에 걸쳐 움직인다. 0이면 움직이지 않는다
	// 히트스톱 동안은 캐릭터 시간이 느려지므로 실제로는 정지가 풀린 뒤에 대부분 진행된다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Impact", meta = (ClampMin = "0.0", Units = "s"))
	float PostHitMoveDuration = 0.f;

	// 히트스톱 시간 (Step 6-1). 맞힌 순간 때린 쪽과 맞은 쪽이 각자 이만큼 멈춘다
	// 타격 해석 매니저(6-4)가 양쪽 값(AttackerHitStop, VictimHitStop)을 정하고, 맞은 쪽에는 FHitResolution으로 전달된다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Feel", meta = (ClampMin = "0.0", Units = "s"))
	float HitStopDuration = 0.f;

	// 모션 워핑 최대 거리 (Step 6-2). 타마다 이 거리 안에서 대상을 찾고, 대상 쪽으로 최대 이만큼 붙는다
	// 대상이 더 멀면(락온, AI) 방향만 돌리고 이 거리까지만 붙는다. 0이면 워핑하지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Feel", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxWarpDistance = 0.f;

	// 콤보 분기 (Step 4-3). 콤보 분기 정보는 여기에만 있다(설계 규칙 4)
	// 몽타주의 Window.Combo 구간에서 목록의 입력이 들어오면 그 다음 공격으로 넘어간다. 비어 있으면 콤보 끝
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (TitleProperty = "InputTag"))
	TArray<FAttackComboLink> ComboLinks;
};
