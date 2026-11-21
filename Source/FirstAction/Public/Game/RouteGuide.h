// 다음 구간 길 안내 (9-4 이후)
// 플레이어 발밑부터 목적지(다음 구간 입구 문)까지 내비 경로를 따라 바닥에 빛나는 점선을 깔고, 목적지에 세로 빛기둥을 세운다.
// 경로는 RepathInterval마다 다시 구하고, 점은 경로를 따라 흐른다. 플레이어 컨트롤러가 만들고 켜고 끈다(연출 전용, 게임 규칙 없음)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RouteGuide.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class FIRSTACTION_API ARouteGuide : public AActor
{
	GENERATED_BODY()

public:
	ARouteGuide();

	// 안내 시작 (같은 목적지면 그대로)
	void ShowRoute(const FVector& Destination);
	void HideRoute();
	bool IsShowing() const { return bShowing; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Route Guide")
	TObjectPtr<UInstancedStaticMeshComponent> Dots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Route Guide")
	TObjectPtr<UStaticMeshComponent> Pillar;

	// 점과 기둥 모양 (BeginPlay에 컴포넌트에 넣는다). 블루프린트 자식(Portfolio/Effects/BP_RouteGuide) 기본값
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide")
	TObjectPtr<UStaticMesh> DotMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Route Guide")
	TObjectPtr<UMaterialInterface> DotMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Route Guide")
	TObjectPtr<UStaticMesh> PillarMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Route Guide")
	TObjectPtr<UMaterialInterface> PillarMaterial;

	// 기둥 굵기 (메시 X·Y 스케일)
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide")
	float PillarWidthScale = 0.9f;

	// 점 간격
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide", meta = (ClampMin = "20.0", Units = "Centimeters"))
	float DotSpacing = 110.f;

	// 점 크기 (메시 스케일)
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide")
	FVector DotScale = FVector(0.22f, 0.22f, 0.02f);

	// 점이 경로를 따라 흐르는 속도
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide", meta = (Units = "CentimetersPerSecond"))
	float FlowSpeed = 160.f;

	// 플레이어 발밑에서 이 거리 안은 점을 그리지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide", meta = (Units = "Centimeters"))
	float StartOffset = 120.f;

	// 경로를 다시 구하는 간격
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide", meta = (ClampMin = "0.1", Units = "s"))
	float RepathInterval = 0.4f;

	// 빛기둥 높이 (메시 Z 스케일 = 높이 / 100)
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide", meta = (Units = "Centimeters"))
	float PillarHeight = 1800.f;

	// 이 거리 안으로 들어오면 점선을 그리지 않는다 (기둥만)
	UPROPERTY(EditDefaultsOnly, Category = "Route Guide", meta = (Units = "Centimeters"))
	float ArriveDistance = 250.f;

private:
	void Repath();
	void LayoutDots();

	TArray<FVector> PathPoints;
	FVector Destination = FVector::ZeroVector;
	FTimerHandle RepathTimer;
	float FlowPhase = 0.f;
	bool bShowing = false;
};
