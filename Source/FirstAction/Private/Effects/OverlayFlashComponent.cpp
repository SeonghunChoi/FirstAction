#include "Effects/OverlayFlashComponent.h"

#include "Components/MeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

UOverlayFlashComponent::UOverlayFlashComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UOverlayFlashComponent::Flash(UMaterialInterface* Material, const FLinearColor& Color, float Duration, float Intensity)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Material || Duration <= 0.f)
	{
		return;
	}

	Restore();

	Instance = UMaterialInstanceDynamic::Create(Material, this);
	Instance->SetVectorParameterValue(TEXT("FlashColor"), Color);
	Instance->SetScalarParameterValue(TEXT("Intensity"), 0.f);

	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		// 보이는 몸·무기 메시만 (위젯, 숨긴 메시 제외)
		if (!Mesh || !Mesh->IsVisible() || Mesh->IsA<UWidgetComponent>() || Mesh->GetNumMaterials() == 0)
		{
			continue;
		}
		Flashed.Add({ Mesh, Mesh->GetOverlayMaterial() });
		Mesh->SetOverlayMaterial(Instance);
	}

	Elapsed = 0.f;
	FlashDuration = Duration;
	PeakIntensity = Intensity;
	SetComponentTickEnabled(true);
}

void UOverlayFlashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 실제 시간으로 흐른다(예비 동작 감속·히트스톱과 상관없이 같은 길이로 보이게)
	const float Dilation = GetOwner() ? FMath::Max(GetOwner()->CustomTimeDilation, 0.01f) : 1.f;
	Elapsed += DeltaTime / Dilation;
	if (Elapsed >= FlashDuration || !Instance)
	{
		Restore();
		return;
	}

	// 짧게 올라갔다가(앞 15%) 천천히 사라진다
	const float T = Elapsed / FlashDuration;
	const float Shape = T < 0.15f ? T / 0.15f : FMath::Square(1.f - (T - 0.15f) / 0.85f);
	Instance->SetScalarParameterValue(TEXT("Intensity"), Shape * PeakIntensity);
}

void UOverlayFlashComponent::Restore()
{
	for (const FFlashedMesh& Entry : Flashed)
	{
		if (UMeshComponent* Mesh = Entry.Mesh.Get())
		{
			if (Mesh->GetOverlayMaterial() == Instance)
			{
				Mesh->SetOverlayMaterial(Entry.PreviousOverlay.Get());
			}
		}
	}
	Flashed.Reset();
	Instance = nullptr;
	SetComponentTickEnabled(false);
}
