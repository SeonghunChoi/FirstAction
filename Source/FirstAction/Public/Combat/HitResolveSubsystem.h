// 타격 해석 매니저. 한 번의 타격을 받아 양쪽이 각자 적용할 결과(FHitResolution)를 계산한다. (PLAN 4.6, 설계 규칙 6)
// 계산만 하고 상태를 기억하지 않는다. 결과는 공격 어빌리티가 받아 자기 몫을 적용하고,
// 맞은 쪽 몫은 Event.HitReact의 TargetData(FGameplayAbilityTargetData_HitResolution)로 보낸다 (6-4 결정).
//
// 간격 분배 (6-4 결정):
//   상대 충격량  I = ImpactForce / 맞은 쪽 질량                 ← 여기서 한 번만 계산 (규칙 6-2)
//   맞은 쪽 밀림 d_v = KnockbackDistancePerImpulse × I          ← 작은 적일수록 멀리
//   때린 쪽 후퇴 d_a = (PostHitSpacing − 맞은 순간 간격) − d_v  ← 음수면 전진. 최종 간격은 항상 PostHitSpacing

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Combat/HitReactionTypes.h"
#include "Engine/HitResult.h"
#include "Subsystems/WorldSubsystem.h"
#include "HitResolveSubsystem.generated.h"

// 매니저 입력. 공격 어빌리티는 공격 데이터의 값과 판정 결과를 넘기기만 한다 (규칙 6)
USTRUCT(BlueprintType)
struct FHitResolveInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	TObjectPtr<AActor> Attacker;

	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	TObjectPtr<AActor> Victim;

	// 판정 결과 (맞은 위치, 맞은 본)
	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	FHitResult Hit;

	// 충격량 (kg·cm/s). 방향은 때린 쪽 → 맞은 쪽 수평
	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	float ImpactForce = 0.f;

	// 타격 후 몸끼리의 간격 (cm). 공격 데이터에만 있는 값 (규칙 6-1)
	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	float PostHitSpacing = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	float Damage = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	float HitStopDuration = 0.f;

	// 타격 후 이동 시간 (초). 공격 데이터 값 (6-5)
	UPROPERTY(BlueprintReadWrite, Category = "Hit Resolve")
	float MoveDuration = 0.f;
};

// 매니저 출력. 때린 쪽과 맞은 쪽이 각자 자기 칸만 적용한다 (규칙 6)
USTRUCT(BlueprintType)
struct FHitResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	TObjectPtr<AActor> Attacker;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	TObjectPtr<AActor> Victim;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	FHitResult Hit;

	// 타격 결과 종류 (6-9). Hit가 아니면 아래 값들은 그 규칙대로 바뀌어 있다 (Evaded: 모두 0)
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	EHitOutcome Outcome = EHitOutcome::Hit;

	// 때린 쪽 → 맞은 쪽 수평 단위 방향. 맞은 쪽은 이 방향으로, 때린 쪽은 반대 방향으로 물러난다
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	FVector Direction = FVector::ForwardVector;

	// 맞은 순간 몸끼리의 간격 (cm)
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float InitialGap = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float PostHitSpacing = 0.f;

	// 상대 충격량 (cm/s) = 충격 ÷ 맞은 쪽 질량. 경직치, 반응 단계(6-7), 부위 반응(6-8)이 이 값을 쓴다 (규칙 6-2)
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float RelativeImpulse = 0.f;

	// 반응 단계. 상대 충격량을 세계 규칙 경계값(UCombatSettings)으로 나눈 결과 (6-7). 그로기는 여기서 나오지 않는다(Event.Groggy)
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	EHitReactionStage ReactionStage = EHitReactionStage::None;

	// 부위 물리 반응: 맞은 바디(Hit.BoneName)에 줄 속도 변화 (cm/s). 0이면 반응 없음 (6-8)
	// 크기는 상대 충격량의 임계값 초과분 × 계수, 방향은 칼이 지나간 방향. 어느 부위를 흔들지는 맞은 쪽 몸이 정한다
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	FVector PartialReactionVelocity = FVector::ZeroVector;

	// 맞은 쪽이 Direction으로 밀리는 거리 (cm) — 맞은 쪽이 적용 (6-5)
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float VictimDisplacement = 0.f;

	// 때린 쪽이 Direction 반대로 물러나는 거리 (cm). 음수면 앞으로 전진 — 때린 쪽이 적용 (6-5)
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float AttackerDisplacement = 0.f;

	// 양쪽 이동이 걸리는 시간 (초). 같은 시간에 움직여야 이동 중에도 간격 비율이 유지된다 (6-5)
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float MoveDuration = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float Damage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float PoiseDamage = 0.f;

	// 히트스톱 시간. 지금은 양쪽이 공격 데이터 값 그대로. 방어 규칙(6-11~)에서 달라질 수 있다
	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float AttackerHitStop = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	float VictimHitStop = 0.f;
};

// FHitResolution을 게임플레이 이벤트의 TargetData에 싣기 위한 포장 (6-4 결정)
// 5-3에서 판정 결과를 FGameplayAbilityTargetData_SingleTargetHit로 실어 보낸 것과 같은 방식
// 싱글플레이 전제라 네트워크 직렬화(NetSerialize)는 구현하지 않는다. 이 데이터는 복제되지 않는다
USTRUCT(BlueprintType)
struct FIRSTACTION_API FGameplayAbilityTargetData_HitResolution : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	FGameplayAbilityTargetData_HitResolution() = default;
	explicit FGameplayAbilityTargetData_HitResolution(const FHitResolution& InResolution) : Resolution(InResolution) {}

	UPROPERTY(BlueprintReadOnly, Category = "Hit Resolve")
	FHitResolution Resolution;

	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

	// 판정 결과도 함께 꺼낼 수 있게 한다 (GetHitResult를 쓰는 기존 코드와 호환)
	virtual bool HasHitResult() const override { return true; }
	virtual const FHitResult* GetHitResult() const override { return &Resolution.Hit; }

	virtual TArray<TWeakObjectPtr<AActor>> GetActors() const override
	{
		TArray<TWeakObjectPtr<AActor>> Actors;
		Actors.Add(Resolution.Victim.Get());
		return Actors;
	}
};

UCLASS()
class FIRSTACTION_API UHitResolveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 한 번의 타격을 해석한다. 상태를 바꾸지 않는다
	FHitResolution Resolve(const FHitResolveInput& Input) const;

	// 이벤트 페이로드의 TargetData에서 타격 해석 결과를 찾는다. 없으면 nullptr (맞은 쪽, 6-5, 6-7에서 사용)
	static const FHitResolution* FindHitResolution(const FGameplayAbilityTargetDataHandle& TargetData);

private:
	// 캐릭터 데이터의 질량 (kg). 없으면 경고 후 기본값
	static float GetMass(const AActor* Actor);

	// 상대 충격량 → 반응 단계. 경계값은 프로젝트 설정 Combat > Hit Reaction (6-7)
	static EHitReactionStage GetReactionStage(float RelativeImpulse);
};
