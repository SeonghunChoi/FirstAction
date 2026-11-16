// 무기의 판정 소켓 쌍(blade_start ~ blade_end)을 매 프레임 스윕해 맞은 대상을 찾는 컴포넌트. (PLAN 4.5)
// 언제 판정할지는 모른다. 애니메이션 구간(ANS_HitWindow, Step 5-2)이 BeginTrace/EndTrace를 부른다.
// 한 번 휘두르는 동안(BeginTrace ~ EndTrace) 같은 대상은 한 번만 맞는다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeTraceComponent.generated.h"

class UMeshComponent;

// 대상을 처음 맞힌 순간 한 번 알린다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMeleeTraceHit, const FHitResult& /*Hit*/);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class FIRSTACTION_API UMeleeTraceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMeleeTraceComponent();

	// 판정 소켓을 가진 무기 메시. 기반 캐릭터가 BeginPlay에서 지정한다
	void SetWeaponMesh(UMeshComponent* InWeaponMesh);

	// 한 번의 휘두름 판정 시작: 맞은 대상 목록을 비우고 현재 칼날 위치를 기준점으로 기록한다
	void BeginTrace();

	// 판정 끝
	void EndTrace();

	bool IsTracing() const { return bTracing; }

	FOnMeleeTraceHit OnHit;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// 판정 소켓 이름 (CLAUDE.md 5.1 규약). 메시를 바꿔도 이름은 유지한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee Trace")
	FName BladeStartSocket = TEXT("blade_start");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee Trace")
	FName BladeEndSocket = TEXT("blade_end");

	// 칼날 위에서 스윕할 점 개수 (양 끝 포함). 많을수록 칼날 중간도 촘촘하게 검사한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee Trace", meta = (ClampMin = "2"))
	int32 SamplesAlongBlade = 3;

	// 각 점이 쓰는 구체 반지름 (칼날 두께)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee Trace", meta = (ClampMin = "0.0", Units = "cm"))
	float TraceRadius = 5.f;

private:
	// 칼날 위의 점들을 지금 소켓 위치로 계산한다. 소켓이 없으면 false
	bool ComputeBladeSamples(TArray<FVector>& OutPoints) const;

	TWeakObjectPtr<UMeshComponent> WeaponMesh;

	// 이전 프레임의 칼날 점들. 이번 프레임 점까지 스윕하는 시작점
	TArray<FVector> PreviousSamples;

	// 이번 휘두름에서 이미 맞은 대상
	TSet<TWeakObjectPtr<AActor>> HitActors;

	// 이번 휘두름에서 스윕한 프레임 수. 저 프레임 판정 누락을 확인하는 진단용 (Step 5-6)
	int32 SweepFrameCount = 0;

	bool bTracing = false;
};
