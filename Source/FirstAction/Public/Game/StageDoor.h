// 스테이지 사이의 문 (게임 흐름, 9-4 이후)
// 닫혀 있으면 길을 막고, 열리면 문 메시가 OpenOffset만큼 움직인다(기본: 바닥 아래로 내려감).
// 열고 닫는 것은 AStageEncounter가 한다(구간 시작에 뒤 문을 닫고, 다 잡으면 앞 문을 연다).
// 내비게이션에는 영향을 주지 않는다. 적은 자기 구간 안에서만 싸우고, 문이 열릴 때는 그 구간 적이 없다.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageDoor.generated.h"

class UStaticMeshComponent;

UCLASS()
class FIRSTACTION_API AStageDoor : public AActor
{
	GENERATED_BODY()

public:
	AStageDoor();

	// 문을 열거나 닫는다. bImmediate면 움직이지 않고 바로 그 자리로 (다시 하기에서 되돌릴 때)
	void SetOpen(bool bOpen, bool bImmediate = false);
	bool IsOpen() const { return bIsOpen; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<USceneComponent> Root;

	// 문 메시. 메시와 크기는 배치마다 고른다 (깃 버전은 기본 큐브, 배포판은 Paragon 메시)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	// 열렸을 때 문 메시의 이동량 (액터 기준, cm)
	UPROPERTY(EditAnywhere, Category = "Door")
	FVector OpenOffset = FVector(0.f, 0.f, -600.f);

	// 열리고 닫히는 데 걸리는 시간
	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0.05", Units = "s"))
	float MoveDuration = 1.2f;

	// 게임 시작 때 열려 있는지 (첫 구간 입구 등)
	UPROPERTY(EditInstanceOnly, Category = "Door")
	bool bStartsOpen = false;

private:
	void ApplyAlpha();

	FVector ClosedLocation = FVector::ZeroVector;
	bool bIsOpen = false;
	float Alpha = 0.f;   // 0 닫힘, 1 열림
};
