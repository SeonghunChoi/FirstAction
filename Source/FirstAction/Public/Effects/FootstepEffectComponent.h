// 발 디딤·물 가르기 연출 (9-3 배포판 얕은 물). 게임 규칙과 무관한 연출이라 어빌리티·태그를 쓰지 않는다.
//  - 발 디딤: 발 뼈 높이가 들렸다 내려오면 디딤. 바닥 물리 재질이 물이면 물결 고리 + 물방울
//    (Paragon 이동 동작에는 발 디딤 노티파이가 없고, 공격·회피·피격 중에도 튀어야 해 애니메이션 표시 대신 뼈 움직임을 본다)
//  - 물 가르기(항적): 물 위를 빠르게 움직이면(대시 회피, 밀려남) 일정 거리마다 작은 물결과 양옆으로 갈라지는 물보라.
//    대시는 낮은 자세로 미끄러져 발이 들리지 않아 디딤만으로는 물결이 생기지 않았다(9-3 시험)
//  - 물방울은 인스턴스 메시 하나로 그리고 이 컴포넌트가 포물선 운동을 계산한다(Niagara 내부 값은 Python으로 고칠 수 없다).
//    편집기에서 만든 Niagara 물보라를 SplashSystem/WakeSplashSystem 칸에 끼우면 함께 띄운다
// 물 재질 목록이 비어 있으면 틱을 끈다(깃 버전 캐릭터는 아무것도 하지 않는다)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootstepEffectComponent.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class UPhysicalMaterial;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class FIRSTACTION_API UFootstepEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFootstepEffectComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// 디딤을 볼 발 뼈
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep")
	TArray<FName> FootBones = { TEXT("foot_l"), TEXT("foot_r") };

	// 물로 볼 바닥 물리 재질. 비어 있으면 이 컴포넌트는 아무것도 하지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep")
	TArray<TObjectPtr<UPhysicalMaterial>> WaterMaterials;

	// 발이 바닥에 닿아 있을 때 발 뼈가 바닥에서 떨어진 높이(cm). 캐릭터마다 다르다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep", meta = (ClampMin = "0"))
	float FootGroundOffset = 10.f;

	// 바닥에서 이만큼(cm, FootGroundOffset 기준) 들렸다가
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep", meta = (ClampMin = "0"))
	float LiftHeight = 12.f;

	// 이만큼(cm) 안으로 내려오면 디딤
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep", meta = (ClampMin = "0"))
	float PlantHeight = 5.f;

	// 물 바닥에서 물 표면까지 높이(cm). 물결과 물방울을 이 높이에 낸다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep", meta = (ClampMin = "0"))
	float WaterDepth = 25.f;

	// 물결 고리: 평면 메시에 재질의 Age(0~1) 파라미터를 올려 고리가 퍼지며 사라진다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Ripple")
	TObjectPtr<UStaticMesh> RippleMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Ripple")
	TObjectPtr<UMaterialInterface> RippleMaterial;

	// 발 디딤 고리가 다 퍼졌을 때 지름(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Ripple", meta = (ClampMin = "1"))
	float RippleSize = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Ripple", meta = (ClampMin = "0.05"))
	float RippleLifetime = 0.9f;

	// 동시에 남아 있는 고리 수. 넘으면 가장 오래된 것을 다시 쓴다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Ripple", meta = (ClampMin = "1"))
	int32 MaxRipples = 24;

	// 물방울: 작은 구 메시(인스턴스)와 물방울 재질
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Droplet")
	TObjectPtr<UStaticMesh> DropletMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Droplet")
	TObjectPtr<UMaterialInterface> DropletMaterial;

	// 발 디딤 한 번에 튀는 물방울 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Droplet", meta = (ClampMin = "0"))
	int32 DropletsPerStep = 7;

	// 튀어 오르는 속력 범위(cm/s)와 옆으로 퍼지는 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Droplet")
	FVector2D DropletSpeed = FVector2D(140.f, 300.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Droplet", meta = (ClampMin = "0"))
	float DropletSpread = 0.55f;

	// 물방울 지름 범위(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Droplet")
	FVector2D DropletSize = FVector2D(2.5f, 5.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Droplet", meta = (ClampMin = "0"))
	float DropletGravity = 980.f;

	// 물 가르기(항적): 이 속력(cm/s)보다 빠르게 물 위를 움직이면
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Wake", meta = (ClampMin = "0"))
	float WakeSpeed = 450.f;

	// 이 거리(cm)마다 물결 하나와 양옆 물보라
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Wake", meta = (ClampMin = "1"))
	float WakeSpacing = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Wake", meta = (ClampMin = "1"))
	float WakeRippleSize = 120.f;

	// 한쪽에 튀는 물방울 수와 옆·위 속력(cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Wake", meta = (ClampMin = "0"))
	int32 WakeDropletsPerSide = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Wake")
	FVector2D WakeSprayVelocity = FVector2D(170.f, 230.f);

	// Niagara 물보라 (선택). 발 디딤 순간 물 표면에 띄운다. 비우면 계산 물방울만 나온다.
	// 계산 물방울과 함께 나오므로 하나만 쓰려면 DropletsPerStep을 0으로
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Niagara")
	TObjectPtr<UNiagaraSystem> SplashSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Niagara")
	FVector SplashScale = FVector(1.f);

	// 물 가르기 Niagara (선택). 항적 간격마다 진행 방향(시스템 X축)으로 돌려 띄운다.
	// 계산 물보라와 함께 나오므로 하나만 쓰려면 WakeDropletsPerSide를 0으로
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Niagara")
	TObjectPtr<UNiagaraSystem> WakeSplashSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footstep|Niagara")
	FVector WakeSplashScale = FVector(1.f);

private:
	struct FRipple
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		TObjectPtr<UMaterialInstanceDynamic> Material;
		float Age = 0.f;
	};

	struct FDroplet
	{
		FVector Location;
		FVector Velocity;
		float Size = 3.f;
		float SurfaceZ = 0.f;
	};

	// 이 위치 아래 바닥이 물이면 물 표면 높이를 돌려준다
	bool FindWaterSurface(const FVector& Location, float TraceDown, float& OutSurfaceZ) const;
	void TickFootPlants();
	void TickWake(float DeltaTime);
	void SpawnRipple(const FVector& Location, float Size);
	void SpawnDroplet(const FVector& Location, const FVector& Velocity, float SurfaceZ);
	void TickRipples(float DeltaTime);
	void TickDroplets(float DeltaTime);

	// 발마다 들린 상태였는지
	TArray<bool> bLifted;
	TArray<FRipple> Ripples;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> DropletInstances;
	TArray<FDroplet> Droplets;

	// 항적: 마지막 물결 이후 움직인 거리
	float WakeDistance = 0.f;
};
