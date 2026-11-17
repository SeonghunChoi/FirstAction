// 락온. 플레이어 전용 컴포넌트 (Step 6-14, 2026-11-15 결정: 락온은 플레이어 클래스에)
// 대상 하나를 고정해 카메라 좌우를 그쪽으로 돌리고, 공격 대상 질의(FindAttackTarget)에 이 대상을 돌려주게 한다(6-2 결정).
//
// 6-14 결정:
//   잡기   — R3 토글. 카메라 정면에서 각도가 가장 작은 전투 캐릭터, AcquireDistance 안
//   바꾸기 — 락온 중 오른쪽 스틱을 튕긴 방향(좌/우)의 다음 대상. 스틱을 놓아야 다시 튕길 수 있다
//   카메라 — 좌우(Yaw)만 대상 쪽으로 CameraYawSpeed로. 위아래는 그대로
//   이동   — 평소처럼 (몸은 이동 방향을 본다). 이 컴포넌트는 이동을 건드리지 않는다
//   풀기   — R3를 다시 누름 / 대상이 LoseDistance 밖 / 대상이 죽으면 다음 대상으로(없으면 풀림)
// 값은 모두 조작감 값이라 블루프린트 기본값에서 정한다 (C++ 기본값 0)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class FIRSTACTION_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// R3: 꺼져 있으면 카메라 정면 대상을 잡고, 켜져 있으면 푼다
	void ToggleLockOn();

	// 락온 중 오른쪽 스틱 입력. 좌우로 문턱값 이상 튕기면 그쪽 다음 대상으로 바꾼다
	void HandleSwitchInput(const FVector2D& StickAxis);

	// 스틱을 놓음: 다음 튕김을 받을 준비
	void ResetSwitchInput() { bSwitchArmed = true; }

	bool IsLockedOn() const { return LockTarget.IsValid(); }
	AActor* GetLockTarget() const { return LockTarget.Get(); }

protected:
	// 락온을 잡는 최대 거리 (cm, 몸 중심 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock On", meta = (ClampMin = "0.0", Units = "cm"))
	float AcquireDistance = 0.f;

	// 이 거리 밖으로 벗어나면 풀린다. 잡는 거리보다 길게 둬야 경계에서 잡혔다 풀렸다 하지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock On", meta = (ClampMin = "0.0", Units = "cm"))
	float LoseDistance = 0.f;

	// 카메라가 대상 쪽으로 도는 속도 (도/초, 좌우만)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock On", meta = (ClampMin = "0.0", Units = "DegreesPerSecond"))
	float CameraYawSpeed = 0.f;

	// 대상 바꾸기로 받아들이는 스틱 좌우 기울기 (0~1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock On", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SwitchStickThreshold = 0.f;

private:
	// 카메라 정면 방향
	FVector GetViewDirection() const;

	// 락온할 수 있는 대상인가: 자신이 아닌 살아 있는 전투 캐릭터, MaxDistance 안
	bool IsValidTarget(const AActor* Candidate, float MaxDistance) const;

	// 카메라 정면에서 각도가 가장 작은 대상 (Exclude 제외)
	AActor* FindBestTarget(const AActor* Exclude) const;

	// 지금 대상 기준 Side(+1 오른쪽, -1 왼쪽)에 있는 대상 중 각도가 가장 가까운 대상
	AActor* FindSwitchTarget(float Side) const;

	void SetLockTarget(AActor* NewTarget);

	// 실행 중 상태값 (규칙 9)
	UPROPERTY(VisibleInstanceOnly, Category = "Lock On")
	TWeakObjectPtr<AActor> LockTarget;

	bool bSwitchArmed = true;
};
