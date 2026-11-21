// 대상 캐릭터를 잠깐 빛나게 하는 GameplayCue (9-4 이후). 공격 예비 동작(GameplayCue.Attack.Telegraph)에 쓴다.
// 에셋은 이 클래스의 블루프린트 자식(Portfolio/GameplayCues/GCN_Attack_Telegraph)으로 만들고 태그·머티리얼·색·소리를 기본값에 둔다.
#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GCN_OverlayFlash.generated.h"

class UMaterialInterface;
class USoundBase;

UCLASS(Abstract)
class FIRSTACTION_API UGCN_OverlayFlash : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	// FlashColor(벡터), Intensity(스칼라) 파라미터가 있는 오버레이 머티리얼
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	FLinearColor FlashColor = FLinearColor(1.f, 0.25f, 0.05f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Flash", meta = (ClampMin = "0.05", Units = "s"))
	float Duration = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Flash", meta = (ClampMin = "0.0"))
	float Intensity = 4.f;

	// 함께 낼 소리 (비어 있으면 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Flash")
	TObjectPtr<USoundBase> Sound;
};
