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

	// [임시] 경직치 감소량 (Step 5-4). GE_Damage에 SetByCaller.PoiseDamage로 전달된다
	// Step 6-4에서 매니저가 상대 충격량(충격 ÷ 맞는 쪽 질량)으로 계산하게 되면 이 칸은 삭제한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Temporary", meta = (ClampMin = "0.0"))
	float PoiseDamage = 0.f;

	// 히트스톱 시간 (Step 6-1). 맞힌 순간 때린 쪽과 맞은 쪽이 각자 이만큼 멈춘다
	// 맞은 쪽에는 Event.HitReact의 EventMagnitude로 전달된다. 6-4부터는 매니저가 양쪽 값을 따로 정할 수 있다
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
