// 부위 물리 반응. 맞은 부위 아래만 잠깐 물리로 흔들고 애니메이션으로 되돌린다. (PLAN 4.6 결론 4, Step 6-8)
// FirstVR UHitReaction 방식의 재사용: UPhysicalAnimationComponent가 물리 바디를 애니메이션 자세 쪽으로 당기고,
// 블렌드 가중치를 회복 시간 동안 1 → 0으로 내린다.
//
// 나눠 맡는 것 (6-8 결정):
//   매니저(세계 규칙)  — 줄지 말지와 세기, 방향: FHitResolution::PartialReactionVelocity
//   이 컴포넌트(몸)    — 어느 부위를 흔들지(캐릭터 데이터의 부위 대표 본), 회복 시간, 막는 상태
// 히트스톱·밀림처럼 몸에 생기는 물리 결과라 캐릭터가 부른다. 반응 어빌리티가 막혀도(그로기, 공격 중 움찔) 준다.
// 단 누운 구간과 가드 중에는 막는다(BlockedTags).
// 메시의 물리 상태(시뮬레이션, 블렌드 가중치)는 이 컴포넌트만 만진다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PartialHitReactionComponent.generated.h"

class USkeletalMeshComponent;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class FIRSTACTION_API UPartialHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPartialHitReactionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// HitBone이 속한 부위를 물리로 돌리고 맞은 바디에 Velocity(속도 변화, cm/s)를 준다
	// 이미 흔들리는 부위에 포함되면 그 반응을 처음부터 다시 시작한다. 막는 상태면 무시한다
	// 타격(6-8) 외에 패리(6-12, 공격자의 AttackBone)도 이 함수를 쓴다
	void ApplyPartialHit(FName HitBone, const FVector& Velocity);

protected:
	virtual void BeginPlay() override;

	// 물리 바디를 애니메이션 자세로 당기는 세기
	// 월드 기준(bIsLocalSimulation 끔)이어야 상체를 맞아도 무너지지 않는다 (FirstVR에서 확인)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Partial Reaction")
	FPhysicalAnimationData PhysicalAnimationData;

	// 이 상태에서는 부위 반응을 주지 않는다 (6-8 결정: 누운 구간, 가드)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Partial Reaction")
	FGameplayTagContainer BlockedTags;

private:
	void StopReaction(FName RootBone);

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimation;

	// 흔들리는 중인 부위. 키는 부위 대표 본, 값은 지난 시간
	TMap<FName, float> ActiveReactions;
};
