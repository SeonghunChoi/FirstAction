// 캐릭터 한 종류의 몸의 성질을 담는 데이터. 플레이어와 적이 같은 형식을 쓴다. (PLAN 4.6, 설계 규칙 6-3)
// 조작감 값(입력 버퍼, 히트스톱 배율, 대상 원뿔)은 여기가 아니라 캐릭터 블루프린트 기본값에 둔다(6-3 결정).
// 칸은 쓰는 스텝에서 하나씩 추가한다: 반응 몽타주(6-7), 부위 반응 임계값(6-8).
// 반응 단계 경계값은 몸이 아니라 세계 규칙이라 여기 두지 않는다(UCombatSettings, 6-7 결정).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/HitReactionTypes.h"
#include "CombatCharacterData.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

UCLASS(BlueprintType, Const)
class FIRSTACTION_API UCombatCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 질량 (kg). 타격 후 간격을 때린 쪽과 맞은 쪽이 나눠 가지는 비율과 상대 충격량(충격 ÷ 맞는 쪽 질량)에 쓴다 (6-4)
	// 이 값이 정본이다. 캐릭터가 시작할 때 캐릭터 무브먼트의 Mass에 복사한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "1.0", Units = "kg"))
	float Mass = 100.f;

	// 반응 단계와 맞은 부위에 맞는 반응 몽타주를 고른다 (6-7)
	// 머리 몽타주가 비어 있으면 몸통 몽타주를 쓴다. 단계에 몽타주가 없으면 nullptr
	UAnimMontage* FindReactionMontage(EHitReactionStage Stage, const USkeletalMeshComponent* Mesh, FName HitBone) const;

protected:
	// 움찔: 공격 중이 아닐 때만 재생한다 (공격 중이면 몽타주 없이 연출만, 6-7 결정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UAnimMontage> FlinchMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UAnimMontage> FlinchHeadMontage;

	// 경직: 공격을 끊는다. 몽타주가 재생되는 동안 State.Staggered
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UAnimMontage> StaggerMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UAnimMontage> StaggerHeadMontage;

	// 넘어짐: 누운 구간에 ANS_TagWindow(State.Downed)를 둔다. 그 구간에는 다시 맞아도 반응하지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UAnimMontage> KnockdownMontage;

	// 그로기: 몽타주 길이가 곧 그로기 시간이다 (규칙 3, 6-7 결정). 끝나면 경직치를 채운다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction")
	TObjectPtr<UAnimMontage> GroggyMontage;

	// 이 본이나 그 아래 본에 맞으면 머리 몽타주를 쓴다 (마네킹은 neck_01 → neck_02 → head)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reaction")
	FName HeadRootBone = TEXT("neck_01");
};
