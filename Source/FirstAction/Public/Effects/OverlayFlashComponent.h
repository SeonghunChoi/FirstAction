// 몸과 무기 메시를 잠깐 빛나게 하는 연출 컴포넌트 (9-4 이후, 공격 예비 동작 번쩍임)
// GameplayCue(UGCN_OverlayFlash)가 대상 액터에 붙여 쓴다. 게임 코드는 이 컴포넌트를 모른다(연출은 Cue, 6-1 결정).
// 메시마다 오버레이 머티리얼(UMeshComponent::SetOverlayMaterial)을 동적 인스턴스로 씌우고 Intensity를 0으로 줄인 뒤 원래대로 되돌린다.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OverlayFlashComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;

UCLASS(ClassGroup = (Effects))
class FIRSTACTION_API UOverlayFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOverlayFlashComponent();

	// 머티리얼의 벡터 파라미터 FlashColor, 스칼라 파라미터 Intensity를 쓴다
	void Flash(UMaterialInterface* Material, const FLinearColor& Color, float Duration, float Intensity);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Restore();

	struct FFlashedMesh
	{
		TWeakObjectPtr<UMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInterface> PreviousOverlay;
	};
	TArray<FFlashedMesh> Flashed;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Instance;

	float Elapsed = 0.f;
	float FlashDuration = 0.f;
	float PeakIntensity = 1.f;
};
