// 캐릭터 한 종류의 몸의 성질을 담는 데이터. 플레이어와 적이 같은 형식을 쓴다. (PLAN 4.6, 설계 규칙 6-3)
// 조작감 값(입력 버퍼, 히트스톱 배율, 대상 원뿔)은 여기가 아니라 캐릭터 블루프린트 기본값에 둔다(6-3 결정).
// 칸은 쓰는 스텝에서 하나씩 추가한다: 반응 몽타주(6-7), 부위 반응의 부위 본과 회복 시간(6-8), 공격 토큰 수(7-5).
// 반응 단계 경계값과 부위 반응 임계값·세기는 몸이 아니라 세계 규칙이라 여기 두지 않는다(UCombatSettings, 6-7·6-8 결정).

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

	// 이 캐릭터를 동시에 공격할 수 있는 토큰 수 (7-5 질문 2 A). 공격받는 쪽의 성질
	// 적은 패턴의 토큰 비용(FEnemyAttackPattern::TokenCost)만큼 이 대상의 토큰을 쥐어야 공격한다. 관리는 UCombatTokenSubsystem
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack Tokens", meta = (ClampMin = "1"))
	int32 AttackTokens = 1;

	UCombatCharacterData();

	// 반응 단계와 맞은 부위에 맞는 반응 몽타주를 고른다 (6-7)
	// 머리 몽타주가 비어 있으면 몸통 몽타주를 쓴다. 단계에 몽타주가 없으면 nullptr
	UAnimMontage* FindReactionMontage(EHitReactionStage Stage, const USkeletalMeshComponent* Mesh, FName HitBone) const;

	// 부위 물리 반응을 시작할 본 (6-8). 맞은 본에서 위로 올라가며 가장 가까운 부위 대표 본,
	// 없으면(골반, 루트처럼 모든 부위의 부모) PartialReactionFallbackBone
	FName FindPartialReactionRootBone(const USkeletalMeshComponent* Mesh, FName HitBone) const;

	float GetPartialReactionRecoverTime() const { return PartialReactionRecoverTime; }

	UAnimMontage* GetDodgeMontage() const { return DodgeMontage; }

	// 방향별 회피 몽타주가 하나라도 있으면 true (9-3). 이때 회피는 몸을 돌리지 않고 입력 방향에 맞는 몽타주를 고른다
	bool HasDirectionalDodge() const { return DodgeBackMontage || DodgeLeftMontage || DodgeRightMontage; }

	// 몸 정면 기준 입력 각도(도, 오른쪽 +)에 맞는 회피 몽타주. 그 방향이 비어 있으면 DodgeMontage
	UAnimMontage* FindDodgeMontage(float LocalYawDegrees) const;

	float GetDodgeDistance() const { return DodgeDistance; }

	UAnimMontage* GetGuardMontage() const { return GuardMontage; }

	UAnimMontage* GetDeathMontage() const { return DeathMontage; }

	// 공격 예비 동작의 시간 배율 (9-4 이후). 콤보로 이어진 타는 따로
	float GetTelegraphTimeScale(bool bComboContinuation) const { return bComboContinuation ? TelegraphComboTimeScale : TelegraphTimeScale; }
	bool UsesTelegraph() const { return TelegraphTimeScale < 1.f; }
	float GetTelegraphComboStrength() const { return TelegraphComboStrength; }

#if WITH_EDITOR
	// 데이터 검증 (Step 8-3)
	// 오류: 질량 0 이하, 공격 토큰 1 미만
	// 경고: 반응(움찔·경직·넘어짐·그로기)·회피·가드·사망 몽타주 없음, 넘어짐 몽타주에 State.Downed 구간 없음,
	//       회피 몽타주에 State.Invulnerable 구간 없음, 부위 대표 본 없음
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
	// 회피 몽타주 (6-9). 앞으로 구르는 루트 모션 하나를 회피 방향으로 몸을 돌려 쓴다
	// 무적 구간(State.Invulnerable)과 캔슬 구간(Window.Cancel)은 이 몽타주의 ANS_TagWindow가 정한다
	// 반응 몽타주처럼 몸마다 다르므로 캐릭터 데이터에 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;

	// 방향별 회피 (9-3). 하나라도 있으면 몸을 돌리지 않고, 입력이 몸 정면 기준 어느 쪽인지(앞 ±45°, 뒤, 좌, 우)로 몽타주를 고른다.
	// DodgeMontage가 앞쪽이다. 비어 있는 방향은 DodgeMontage를 쓴다. 깃 버전은 비워 두어 "돌고 앞구르기"를 그대로 쓴다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeBackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeLeftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeRightMontage;

	// 코드로 옮기는 회피 거리 (9-3). 0이면 몽타주 루트 모션으로 움직인다(깃 버전).
	// 0보다 크고 몽타주에 루트 모션이 없으면, 몽타주의 Motion Warping 구간 동안 입력 방향으로 이 거리만큼 옮긴다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float DodgeDistance = 0.f;

	// 가드 몽타주 (6-11). 구간 세 개: Start(올림, 앞부분에 Window.Parry) → Loop(유지, 반복) → End(내림, 뒷부분에 Window.Cancel)
	// 버튼을 떼면 End로 넘어가고, 몽타주가 끝나면 가드가 끝난다. 구간 이름은 GA_Guard 기본값과 같아야 한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<UAnimMontage> GuardMontage;

	// 공격 예비 동작(공격 몽타주 시작 ~ 첫 판정 구간) 동안 이 캐릭터의 시간 배율 (9-4 이후, 적 공격을 알아보고 피할 시간)
	// 1이면 그대로(플레이어). 1보다 작으면 그만큼 느려지고 GameplayCue.Attack.Telegraph(번쩍임)를 낸다.
	// 타이밍은 몽타주의 판정 구간이 정한다(규칙 3). 공격 데이터는 플레이어와 적이 함께 쓰므로 몸의 데이터에 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TelegraphTimeScale = 1.f;

	// 콤보로 이어진 타의 예비 동작 시간 배율. 보통 첫 타보다 1에 가깝게 (콤보 속도감 유지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TelegraphComboTimeScale = 1.f;

	// 몽타주에 예비 동작 구간(ANS_Telegraph)이 있으면 위 두 배율 대신 구간의 배율을 쓴다(9-4 이후 2차).
	// 콤보로 이어진 타는 이 비율만큼만 적용한다 (0 = 감속 없음, 1 = 구간 배율 그대로). 첫 타는 늘 구간 배율 그대로
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TelegraphComboStrength = 0.5f;

	// 사망 몽타주 (7-1). 끝나면 래그돌로 넘어간다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 부위 대표 본. 이 본과 그 아래가 함께 흔들린다 (6-8 결정: 부위 단위, 스켈레톤마다 다르므로 몸의 데이터)
	// 기본값은 마네킹: 양팔, 양다리, 머리, 상체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Partial Reaction")
	TArray<FName> PartialReactionRootBones;

	// 어느 부위 대표 본의 아래도 아닌 본(골반 등)을 맞았을 때 흔들 부위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Partial Reaction")
	FName PartialReactionFallbackBone = TEXT("spine_01");

	// 흔들린 부위가 애니메이션 자세로 완전히 돌아오는 시간 (6-8 결정: 몸마다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Partial Reaction", meta = (ClampMin = "0.05", Units = "s"))
	float PartialReactionRecoverTime = 0.4f;

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
