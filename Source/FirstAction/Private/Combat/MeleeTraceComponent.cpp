#include "Combat/MeleeTraceComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "FirstAction.h"

namespace
{
	// 콘솔에서 FirstAction.ShowMeleeTrace 1 → 스윕 경로와 맞은 위치를 화면에 그린다
	TAutoConsoleVariable<bool> CVarShowMeleeTrace(
		TEXT("FirstAction.ShowMeleeTrace"),
		false,
		TEXT("근접 판정 스윕을 화면에 그린다 (0: 끔, 1: 켬)"));
}

UMeleeTraceComponent::UMeleeTraceComponent()
{
	// 판정 구간에서만 틱한다
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 애니메이션으로 칼날 위치가 정해진 뒤에 읽는다
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UMeleeTraceComponent::SetWeaponMesh(UMeshComponent* InWeaponMesh)
{
	WeaponMesh = InWeaponMesh;
}

bool UMeleeTraceComponent::ComputeBladeSamples(TArray<FVector>& OutPoints) const
{
	const UMeshComponent* Mesh = WeaponMesh.Get();
	if (!Mesh || !Mesh->DoesSocketExist(BladeStartSocket) || !Mesh->DoesSocketExist(BladeEndSocket))
	{
		return false;
	}

	const FVector Start = Mesh->GetSocketLocation(BladeStartSocket);
	const FVector End = Mesh->GetSocketLocation(BladeEndSocket);

	const int32 Count = FMath::Max(2, SamplesAlongBlade);
	OutPoints.SetNum(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Count - 1);
		OutPoints[Index] = FMath::Lerp(Start, End, Alpha);
	}
	return true;
}

void UMeleeTraceComponent::BeginTrace()
{
	HitActors.Reset();
	PreviousSamples.Reset();

	if (!ComputeBladeSamples(PreviousSamples))
	{
		// 무기가 없는 캐릭터(더미 등)는 판정하지 않는다
		UE_LOG(LogFirstAction, Verbose, TEXT("[%s] MeleeTrace: 무기 메시나 판정 소켓이 없어 판정하지 않습니다"), *GetNameSafe(GetOwner()));
		return;
	}

	bTracing = true;
	SetComponentTickEnabled(true);
}

void UMeleeTraceComponent::EndTrace()
{
	bTracing = false;
	SetComponentTickEnabled(false);
	PreviousSamples.Reset();
}

void UMeleeTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	TArray<FVector> CurrentSamples;
	if (!bTracing || !World || !ComputeBladeSamples(CurrentSamples) || CurrentSamples.Num() != PreviousSamples.Num())
	{
		return;
	}

	const bool bDebugDraw = CVarShowMeleeTrace.GetValueOnGameThread();

	// 캐릭터(Pawn 오브젝트 타입)만 맞힌다. 캡슐과 메시가 모두 Pawn이지만,
	// 아래에서 스켈레탈 메시(피직스 바디)에 맞은 결과만 인정한다 → 실제 몸에 닿은 순간, 맞은 본 이름과 함께
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeTrace), false, GetOwner());
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(TraceRadius);

	// 칼날 위 점마다 "이전 프레임 위치 → 이번 프레임 위치"를 구체로 스윕한다
	// (프레임이 낮으면 이 직선 경로가 실제 호를 크게 벗어난다 → Step 5-6에서 서브스텝 보간으로 해결)
	for (int32 Index = 0; Index < CurrentSamples.Num(); ++Index)
	{
		const FVector& From = PreviousSamples[Index];
		const FVector& To = CurrentSamples[Index];

		TArray<FHitResult> Hits;
		World->SweepMultiByObjectType(Hits, From, To, FQuat::Identity, ObjectParams, Sphere, QueryParams);

		if (bDebugDraw)
		{
			DrawDebugLine(World, From, To, FColor::Yellow, false, 1.f, 0, 1.f);
		}

		for (const FHitResult& Hit : Hits)
		{
			// 캡슐은 몸보다 커서 칼이 허공에 있을 때 먼저 걸리고 본 정보도 없다. 메시에 맞은 것만 받는다
			if (!Cast<USkeletalMeshComponent>(Hit.GetComponent()))
			{
				continue;
			}

			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActors.Contains(HitActor))
			{
				continue;
			}

			HitActors.Add(HitActor);

			UE_LOG(LogFirstAction, Log, TEXT("[%s] 판정: %s (본 %s)"),
				*GetNameSafe(GetOwner()), *GetNameSafe(HitActor), *Hit.BoneName.ToString());

			if (bDebugDraw)
			{
				DrawDebugSphere(World, Hit.ImpactPoint, TraceRadius * 2.f, 12, FColor::Red, false, 1.f);
			}

			OnHit.Broadcast(Hit);
		}
	}

	if (bDebugDraw)
	{
		DrawDebugLine(World, CurrentSamples[0], CurrentSamples.Last(), FColor::Cyan, false, 1.f, 0, 1.5f);
	}

	PreviousSamples = MoveTemp(CurrentSamples);
}
