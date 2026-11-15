// 공격 하나의 성질을 담는 데이터. "공격 하나는 데이터 한 장" (PLAN 4.3, 설계 규칙 2)
// GA_MeleeAttack은 특정 공격을 모르고, 이 데이터만 읽어서 실행한다.
// 칸은 쓰는 스텝에서 하나씩 추가한다. 이름이나 타입을 바꾸면 이미 입력한 값이 사라지기 때문이다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttackDataAsset.generated.h"

class UAnimMontage;

UCLASS(BlueprintType, Const)
class FIRSTACTION_API UAttackDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 재생할 공격 몽타주 (Step 3-2)
	// 판정, 콤보, 캔슬 구간의 타이밍은 C++가 아니라 이 몽타주의 노티파이가 가진다(설계 규칙 3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage;
};
