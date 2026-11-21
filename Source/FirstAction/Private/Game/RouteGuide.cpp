#include "Game/RouteGuide.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

ARouteGuide::ARouteGuide()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bTickEvenWhenPaused = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	Dots = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Dots"));
	Dots->SetupAttachment(Root);
	Dots->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Dots->SetCastShadow(false);
	Dots->SetCanEverAffectNavigation(false);
	Dots->SetUsingAbsoluteLocation(true);
	Dots->SetUsingAbsoluteRotation(true);

	Pillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar"));
	Pillar->SetupAttachment(Root);
	Pillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pillar->SetCastShadow(false);
	Pillar->SetCanEverAffectNavigation(false);
	Pillar->SetUsingAbsoluteLocation(true);

	SetActorHiddenInGame(true);
}

void ARouteGuide::BeginPlay()
{
	Super::BeginPlay();
	// 월드 원점에 둔다(인스턴스는 월드 좌표)
	SetActorLocation(FVector::ZeroVector);

	if (DotMesh)
	{
		Dots->SetStaticMesh(DotMesh);
	}
	if (DotMaterial)
	{
		Dots->SetMaterial(0, DotMaterial);
	}
	if (PillarMesh)
	{
		Pillar->SetStaticMesh(PillarMesh);
	}
	if (PillarMaterial)
	{
		Pillar->SetMaterial(0, PillarMaterial);
	}
	Pillar->SetRelativeScale3D(FVector(PillarWidthScale, PillarWidthScale, 1.f));
}

void ARouteGuide::ShowRoute(const FVector& InDestination)
{
	Destination = InDestination;
	bShowing = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// 기둥 메시는 높이 100, 가운데가 원점(엔진 기본 실린더)이라 높이의 절반만큼 올린다. 바닥 아래로 조금 묻는다
	const FVector PillarScale = Pillar->GetRelativeScale3D();
	Pillar->SetRelativeScale3D(FVector(PillarScale.X, PillarScale.Y, PillarHeight / 100.f));
	Pillar->SetWorldLocation(Destination + FVector(0.f, 0.f, PillarHeight * 0.5f - 100.f));

	Repath();
	GetWorldTimerManager().SetTimer(RepathTimer, this, &ARouteGuide::Repath, RepathInterval, true);
}

void ARouteGuide::HideRoute()
{
	bShowing = false;
	GetWorldTimerManager().ClearTimer(RepathTimer);
	Dots->ClearInstances();
	PathPoints.Reset();
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void ARouteGuide::Repath()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		PathPoints.Reset();
		return;
	}

	const FVector From = Player->GetActorLocation();
	if (FVector::Dist2D(From, Destination) < ArriveDistance)
	{
		PathPoints.Reset();
		return;
	}

	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), From, Destination);
	if (Path && Path->IsValid() && Path->PathPoints.Num() >= 2)
	{
		PathPoints = Path->PathPoints;
	}
	else
	{
		// 길을 못 찾으면 곧은 줄
		PathPoints = { From - FVector(0.f, 0.f, Player->GetSimpleCollisionHalfHeight()), Destination };
	}
}

void ARouteGuide::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FlowPhase = FMath::Fmod(FlowPhase + FlowSpeed * DeltaSeconds, DotSpacing);
	LayoutDots();
}

void ARouteGuide::LayoutDots()
{
	TArray<FTransform> Transforms;
	if (PathPoints.Num() >= 2)
	{
		// 경로를 따라 StartOffset + FlowPhase부터 DotSpacing마다 점
		float Next = StartOffset + FlowPhase;
		float Walked = 0.f;
		for (int32 i = 1; i < PathPoints.Num(); ++i)
		{
			const FVector A = PathPoints[i - 1];
			const FVector B = PathPoints[i];
			const float SegmentLength = FVector::Dist(A, B);
			while (Next <= Walked + SegmentLength && SegmentLength > KINDA_SMALL_NUMBER)
			{
				const FVector P = FMath::Lerp(A, B, (Next - Walked) / SegmentLength);
				Transforms.Emplace(FRotator(0.f, (B - A).Rotation().Yaw, 0.f), P + FVector(0.f, 0.f, 4.f), DotScale);
				Next += DotSpacing;
			}
			Walked += SegmentLength;
		}
	}

	// 인스턴스 수가 같으면 위치만 바꾸고, 다르면 다시 만든다
	if (Transforms.Num() != Dots->GetInstanceCount())
	{
		Dots->ClearInstances();
		Dots->AddInstances(Transforms, false, true);
	}
	else if (Transforms.Num() > 0)
	{
		Dots->BatchUpdateInstancesTransforms(0, Transforms, true, true);
	}
}
