#include "Game/StageEncounter.h"

#include "AbilitySystemComponent.h"
#include "AI/EnemyCharacter.h"
#include "Combat/CombatTags.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Game/StageDoor.h"
#include "TimerManager.h"
#include "FirstAction.h"

AStageEncounter::AStageEncounter()
{
	PrimaryActorTick.bCanEverTick = false;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	SetRootComponent(Area);
	Area->SetBoxExtent(FVector(800.f, 800.f, 300.f));
	Area->SetCollisionProfileName(TEXT("Trigger"));
	Area->SetCanEverAffectNavigation(false);
	Area->ShapeColor = FColor(255, 160, 0);
}

void AStageEncounter::BeginPlay()
{
	Super::BeginPlay();
	Area->OnComponentBeginOverlap.AddDynamic(this, &AStageEncounter::OnAreaBeginOverlap);
}

void AStageEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AStageEncounter::OnAreaBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	TryStart(Cast<APawn>(OtherActor));
}

bool AStageEncounter::TryStart(APawn* Pawn)
{
	if (State != EStageEncounterState::Idle || !Pawn || !Pawn->IsPlayerControlled() || !Area->IsOverlappingActor(Pawn))
	{
		return false;
	}

	State = EStageEncounterState::Active;
	PlayerPawn = Pawn;
	NextWaveIndex = 0;
	AliveEnemies.Reset();

	DoorsInitiallyOpen.Reset();
	for (AStageDoor* Door : DoorsToCloseOnStart)
	{
		DoorsInitiallyOpen.Add(Door && Door->IsOpen());
		if (Door)
		{
			Door->SetOpen(false);
		}
	}

	UE_LOG(LogFirstAction, Log, TEXT("[Stage %d] 시작: 웨이브 %d개"), StageNumber, Waves.Num());
	OnStarted.Broadcast(this);
	ScheduleSpawn(FirstSpawnDelay);
	return true;
}

void AStageEncounter::ScheduleSpawn(float Delay)
{
	if (UWorld* World = GetWorld())
	{
		if (Delay <= 0.f)
		{
			SpawnNext();
			return;
		}
		World->GetTimerManager().SetTimer(SpawnTimer, this, &AStageEncounter::SpawnNext, Delay, false);
	}
}

void AStageEncounter::SpawnNext()
{
	if (State != EStageEncounterState::Active)
	{
		return;
	}

	PruneAlive();
	if (AliveEnemies.Num() > 0)
	{
		return;   // 지금 웨이브가 남아 있다
	}

	// 다음 웨이브를 한꺼번에. 생성 위치는 플레이어에게서 먼 곳부터 서로 다르게
	while (AliveEnemies.Num() == 0 && Waves.IsValidIndex(NextWaveIndex))
	{
		const FStageWave& Wave = Waves[NextWaveIndex++];
		TArray<const AActor*> UsedPoints;
		for (const TSubclassOf<AEnemyCharacter>& EnemyClass : Wave.Enemies)
		{
			if (!EnemyClass)
			{
				UE_LOG(LogFirstAction, Warning, TEXT("[Stage %d] %d번째 웨이브에 빈 적 클래스가 있습니다"), StageNumber, NextWaveIndex);
				continue;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			AEnemyCharacter* Enemy = GetWorld()->SpawnActor<AEnemyCharacter>(EnemyClass, ChooseSpawnTransform(UsedPoints), Params);
			if (!Enemy)
			{
				UE_LOG(LogFirstAction, Warning, TEXT("[Stage %d] %s 생성 실패"), StageNumber, *GetNameSafe(EnemyClass));
				continue;
			}
			Enemy->OnEnemyDied.AddDynamic(this, &AStageEncounter::HandleEnemyDied);
			AliveEnemies.Add(Enemy);
			UE_LOG(LogFirstAction, Log, TEXT("[Stage %d] 웨이브 %d/%d: %s 생성"), StageNumber, NextWaveIndex, Waves.Num(), *Enemy->GetName());
		}
	}

	if (AliveEnemies.Num() == 0 && !Waves.IsValidIndex(NextWaveIndex))
	{
		State = EStageEncounterState::Cleared;
		for (AStageDoor* Door : DoorsToOpenOnClear)
		{
			if (Door)
			{
				Door->SetOpen(true);
			}
		}
		UE_LOG(LogFirstAction, Log, TEXT("[Stage %d] 클리어"), StageNumber);
		OnCleared.Broadcast(this);
	}
}

void AStageEncounter::HandleEnemyDied()
{
	PruneAlive();
	if (AliveEnemies.Num() == 0)
	{
		// 웨이브가 다 쓰러지면 다음 웨이브(또는 클리어 판정)를 기다렸다 한다. 마지막 적의 쓰러지는 모습을 보여 주는 시간
		ScheduleSpawn(NextSpawnDelay);
	}
}

void AStageEncounter::PruneAlive()
{
	AliveEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyCharacter>& Enemy)
	{
		if (!Enemy.IsValid())
		{
			return true;
		}
		const UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
		return ASC && ASC->HasMatchingGameplayTag(CombatTags::State_Dead);
	});
}

FTransform AStageEncounter::ChooseSpawnTransform(TArray<const AActor*>& UsedPoints) const
{
	const APawn* Player = PlayerPawn.Get();
	const AActor* Best = nullptr;
	float BestDistSq = -1.f;
	for (const AActor* Point : SpawnPoints)
	{
		// 같은 웨이브가 이미 쓴 곳은 건너뛴다(생성 위치보다 적이 많으면 다시 쓴다)
		if (!Point || (UsedPoints.Contains(Point) && UsedPoints.Num() < SpawnPoints.Num()))
		{
			continue;
		}
		const float DistSq = Player ? FVector::DistSquared2D(Point->GetActorLocation(), Player->GetActorLocation()) : 0.f;
		if (DistSq > BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Point;
		}
	}

	if (Best)
	{
		UsedPoints.Add(Best);
	}
	FVector Location = Best ? Best->GetActorLocation() : GetActorLocation();
	FRotator Rotation = Best ? Best->GetActorRotation() : GetActorRotation();
	if (Player)
	{
		// 플레이어를 보고 나온다
		Rotation = FRotator(0.f, (Player->GetActorLocation() - Location).Rotation().Yaw, 0.f);
	}
	return FTransform(Rotation, Location);
}

void AStageEncounter::ResetEncounter()
{
	if (State != EStageEncounterState::Active)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
	for (const TWeakObjectPtr<AEnemyCharacter>& Enemy : AliveEnemies)
	{
		if (Enemy.IsValid())
		{
			Enemy->OnEnemyDied.RemoveDynamic(this, &AStageEncounter::HandleEnemyDied);
			Enemy->Destroy();
		}
	}
	AliveEnemies.Reset();

	for (int32 i = 0; i < DoorsToCloseOnStart.Num(); ++i)
	{
		if (AStageDoor* Door = DoorsToCloseOnStart[i])
		{
			Door->SetOpen(DoorsInitiallyOpen.IsValidIndex(i) && DoorsInitiallyOpen[i], true);
		}
	}

	State = EStageEncounterState::Idle;
	NextWaveIndex = 0;
	PlayerPawn.Reset();
	UE_LOG(LogFirstAction, Log, TEXT("[Stage %d] 다시 하기로 되돌림"), StageNumber);
}

FVector AStageEncounter::GetEntranceLocation() const
{
	for (const AStageDoor* Door : DoorsToCloseOnStart)
	{
		if (Door)
		{
			return Door->GetActorLocation();
		}
	}
	return GetRespawnTransform().GetLocation();
}

FTransform AStageEncounter::GetRespawnTransform() const
{
	const AActor* Point = RespawnPoint ? RespawnPoint.Get() : this;
	return FTransform(FRotator(0.f, Point->GetActorRotation().Yaw, 0.f), Point->GetActorLocation());
}
