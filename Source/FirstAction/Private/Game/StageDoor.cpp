#include "Game/StageDoor.h"

#include "Components/StaticMeshComponent.h"

AStageDoor::AStageDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	DoorMesh->SetCanEverAffectNavigation(false);
	DoorMesh->SetMobility(EComponentMobility::Movable);
}

void AStageDoor::BeginPlay()
{
	Super::BeginPlay();

	ClosedLocation = DoorMesh->GetRelativeLocation();
	SetOpen(bStartsOpen, true);
}

void AStageDoor::SetOpen(bool bOpen, bool bImmediate)
{
	bIsOpen = bOpen;
	if (bImmediate)
	{
		Alpha = bOpen ? 1.f : 0.f;
		ApplyAlpha();
		SetActorTickEnabled(false);
		return;
	}

	// 닫히기 시작하면 바로 막는다. 열릴 때는 다 열린 뒤에 충돌을 끈다(ApplyAlpha)
	if (!bOpen)
	{
		DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	SetActorTickEnabled(true);
}

void AStageDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Target = bIsOpen ? 1.f : 0.f;
	Alpha = FMath::FInterpConstantTo(Alpha, Target, DeltaSeconds, 1.f / FMath::Max(MoveDuration, 0.05f));
	ApplyAlpha();
	if (FMath::IsNearlyEqual(Alpha, Target))
	{
		SetActorTickEnabled(false);
	}
}

void AStageDoor::ApplyAlpha()
{
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	DoorMesh->SetRelativeLocation(ClosedLocation + OpenOffset * Eased);
	DoorMesh->SetCollisionEnabled(Alpha >= 1.f ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	DoorMesh->SetVisibility(Alpha < 1.f);
}
