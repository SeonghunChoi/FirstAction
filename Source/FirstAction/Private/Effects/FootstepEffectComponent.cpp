#include "Effects/FootstepEffectComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

static const FName RippleAgeParam(TEXT("Age"));

UFootstepEffectComponent::UFootstepEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 애니메이션이 뼈를 갱신한 뒤에 본다
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UFootstepEffectComponent::BeginPlay()
{
	Super::BeginPlay();
	bLifted.Init(false, FootBones.Num());
	if (WaterMaterials.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

void UFootstepEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickRipples(DeltaTime);
	TickDroplets(DeltaTime);
	TickFootPlants();
	TickWake(DeltaTime);
}

bool UFootstepEffectComponent::FindWaterSurface(const FVector& Location, float TraceDown, float& OutSurfaceZ) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootstepEffect), true, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0, 0, 50), Location - FVector(0, 0, TraceDown), ECC_Visibility, Params)
		|| !WaterMaterials.Contains(Hit.PhysMaterial.Get()))
	{
		return false;
	}
	OutSurfaceZ = Hit.ImpactPoint.Z + WaterDepth;
	return true;
}

void UFootstepEffectComponent::TickFootPlants()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh || !Character->GetCharacterMovement() || Character->GetCharacterMovement()->IsFalling())
	{
		bLifted.Init(false, FootBones.Num());
		return;
	}

	// 바닥 높이는 캐릭터 발밑 바닥(캡슐 아래). 발마다 따로 쏘지 않고, 디딘 순간에만 발 밑을 쏴 재질을 본다
	const float FloorZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	for (int32 Index = 0; Index < FootBones.Num(); ++Index)
	{
		if (Mesh->GetBoneIndex(FootBones[Index]) == INDEX_NONE)
		{
			continue;
		}
		const FVector Foot = Mesh->GetSocketLocation(FootBones[Index]);
		const float Height = Foot.Z - FloorZ - FootGroundOffset;
		if (Height > LiftHeight)
		{
			bLifted[Index] = true;
		}
		else if (bLifted[Index] && Height < PlantHeight)
		{
			bLifted[Index] = false;
			float SurfaceZ = 0.f;
			if (FindWaterSurface(Foot, 80.f, SurfaceZ))
			{
				const FVector Surface(Foot.X, Foot.Y, SurfaceZ);
				SpawnRipple(Surface, RippleSize);
				if (SplashSystem)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SplashSystem, Surface, FRotator::ZeroRotator, SplashScale,
						true, true, ENCPoolMethod::AutoRelease);
				}
				for (int32 I = 0; I < DropletsPerStep; ++I)
				{
					// 위로 튀되 사방으로 조금씩 퍼진다
					const FVector2D Dir = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
					const float Speed = FMath::FRandRange(DropletSpeed.X, DropletSpeed.Y);
					const FVector Velocity(Dir.X * Speed * DropletSpread, Dir.Y * Speed * DropletSpread, Speed);
					SpawnDroplet(Surface + FVector(Dir.X, Dir.Y, 0) * 6.f, Velocity, SurfaceZ);
				}
			}
		}
	}
}

void UFootstepEffectComponent::TickWake(float DeltaTime)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	// 물 가르기는 코드가 몸을 밀어 옮길 때(Root Motion Source: 대시 회피, 맞아서 밀려남, 공격 접근)만.
	// 평소 걷고 뛸 때는 발 디딤 물결로 충분하고, 속도만 보면 뛰기에도 걸려 너무 많이 났다(9-3 시험)
	if (!Movement || Movement->IsFalling() || !Movement->HasRootMotionSources())
	{
		WakeDistance = 0.f;
		return;
	}
	const FVector Velocity2D(Character->GetVelocity().X, Character->GetVelocity().Y, 0.f);
	const float Speed = Velocity2D.Size();
	if (Speed < WakeSpeed)
	{
		WakeDistance = 0.f;
		return;
	}

	WakeDistance += Speed * DeltaTime;
	if (WakeDistance < WakeSpacing)
	{
		return;
	}
	WakeDistance = FMath::Fmod(WakeDistance, WakeSpacing);

	const FVector Location = Character->GetActorLocation();
	float SurfaceZ = 0.f;
	if (!FindWaterSurface(Location, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 60.f, SurfaceZ))
	{
		return;
	}

	// 몸이 지나간 자리에 물결, 양옆으로 갈라지는 물보라
	const FVector Forward = Velocity2D / Speed;
	const FVector Right(-Forward.Y, Forward.X, 0.f);
	const FVector Surface(Location.X, Location.Y, SurfaceZ);
	SpawnRipple(Surface - Forward * 20.f, WakeRippleSize);
	if (WakeSplashSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WakeSplashSystem, Surface, Forward.Rotation(), WakeSplashScale,
			true, true, ENCPoolMethod::AutoRelease);
	}
	for (const float Side : { -1.f, 1.f })
	{
		for (int32 I = 0; I < WakeDropletsPerSide; ++I)
		{
			const float Out = WakeSprayVelocity.X * FMath::FRandRange(0.7f, 1.2f);
			const float Up = WakeSprayVelocity.Y * FMath::FRandRange(0.7f, 1.2f);
			const FVector Velocity = Right * Side * Out + Forward * Speed * 0.25f + FVector(0, 0, Up);
			SpawnDroplet(Surface + Right * Side * 18.f, Velocity, SurfaceZ);
		}
	}
}

void UFootstepEffectComponent::SpawnRipple(const FVector& Location, float Size)
{
	if (!RippleMesh || !RippleMaterial)
	{
		return;
	}

	// 가장 오래된 고리를 다시 쓰거나 새로 만든다
	FRipple* Ripple = nullptr;
	if (Ripples.Num() < MaxRipples)
	{
		FRipple& New = Ripples.AddDefaulted_GetRef();
		New.Mesh = NewObject<UStaticMeshComponent>(GetOwner());
		New.Mesh->SetStaticMesh(RippleMesh);
		New.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		New.Mesh->SetCanEverAffectNavigation(false);
		New.Mesh->SetCastShadow(false);
		New.Mesh->SetUsingAbsoluteLocation(true);
		New.Mesh->SetUsingAbsoluteRotation(true);
		New.Mesh->SetUsingAbsoluteScale(true);
		New.Mesh->RegisterComponent();
		New.Material = New.Mesh->CreateDynamicMaterialInstance(0, RippleMaterial);
		Ripple = &New;
	}
	else
	{
		Ripple = &Ripples[0];
		for (FRipple& R : Ripples)
		{
			if (!R.Mesh->IsVisible())
			{
				Ripple = &R;
				break;
			}
			if (R.Age > Ripple->Age)
			{
				Ripple = &R;
			}
		}
	}

	// 엔진 평면 메시는 100cm 한 변
	const float Scale = Size / FMath::Max(RippleMesh->GetBoundingBox().GetSize().X, 1.f);
	Ripple->Mesh->SetWorldLocationAndRotation(Location, FRotator::ZeroRotator);
	Ripple->Mesh->SetWorldScale3D(FVector(Scale, Scale, 1.f));
	Ripple->Mesh->SetVisibility(true);
	Ripple->Age = 0.f;
	if (Ripple->Material)
	{
		Ripple->Material->SetScalarParameterValue(RippleAgeParam, 0.f);
	}
}

void UFootstepEffectComponent::SpawnDroplet(const FVector& Location, const FVector& Velocity, float SurfaceZ)
{
	if (!DropletMesh || !DropletMaterial)
	{
		return;
	}
	if (!DropletInstances)
	{
		// 월드 원점에 둔 인스턴스 메시 하나에 모든 물방울을 그린다
		DropletInstances = NewObject<UInstancedStaticMeshComponent>(GetOwner());
		DropletInstances->SetStaticMesh(DropletMesh);
		DropletInstances->SetMaterial(0, DropletMaterial);
		DropletInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DropletInstances->SetCanEverAffectNavigation(false);
		DropletInstances->SetCastShadow(false);
		DropletInstances->SetUsingAbsoluteLocation(true);
		DropletInstances->SetUsingAbsoluteRotation(true);
		DropletInstances->SetUsingAbsoluteScale(true);
		DropletInstances->RegisterComponent();
		DropletInstances->SetWorldTransform(FTransform::Identity);
	}
	FDroplet& Drop = Droplets.AddDefaulted_GetRef();
	Drop.Location = Location;
	Drop.Velocity = Velocity;
	Drop.Size = FMath::FRandRange(DropletSize.X, DropletSize.Y);
	Drop.SurfaceZ = SurfaceZ;
}

void UFootstepEffectComponent::TickRipples(float DeltaTime)
{
	for (FRipple& Ripple : Ripples)
	{
		if (!Ripple.Mesh || !Ripple.Mesh->IsVisible())
		{
			continue;
		}
		Ripple.Age += DeltaTime / RippleLifetime;
		if (Ripple.Age >= 1.f)
		{
			Ripple.Age = 1.f;
			Ripple.Mesh->SetVisibility(false);
		}
		if (Ripple.Material)
		{
			Ripple.Material->SetScalarParameterValue(RippleAgeParam, Ripple.Age);
		}
	}
}

void UFootstepEffectComponent::TickDroplets(float DeltaTime)
{
	if (!DropletInstances)
	{
		return;
	}

	// 포물선 운동. 물 표면 아래로 떨어지면 사라진다
	TArray<FTransform> Transforms;
	Transforms.Reserve(Droplets.Num());
	const float MeshSize = FMath::Max(DropletMesh->GetBoundingBox().GetSize().X, 1.f);
	for (int32 Index = Droplets.Num() - 1; Index >= 0; --Index)
	{
		FDroplet& Drop = Droplets[Index];
		Drop.Velocity.Z -= DropletGravity * DeltaTime;
		Drop.Location += Drop.Velocity * DeltaTime;
		if (Drop.Location.Z < Drop.SurfaceZ && Drop.Velocity.Z < 0.f)
		{
			Droplets.RemoveAtSwap(Index);
			continue;
		}
		// 움직이는 방향으로 조금 늘인 물방울
		const float Speed = Drop.Velocity.Size();
		const float Stretch = 1.f + FMath::Min(Speed / 400.f, 1.5f);
		const float Scale = Drop.Size / MeshSize;
		Transforms.Emplace(Drop.Velocity.ToOrientationQuat(), Drop.Location, FVector(Scale * Stretch, Scale, Scale));
	}
	DropletInstances->ClearInstances();
	if (!Transforms.IsEmpty())
	{
		DropletInstances->AddInstances(Transforms, false, true);
	}
}
