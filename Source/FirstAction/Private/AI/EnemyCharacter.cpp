#include "AI/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Combat/AttackDataAsset.h"
#include "Combat/CombatTags.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "FirstAction.h"

AEnemyCharacter::AEnemyCharacter()
{
	// 레벨에 놓이거나 생성되면 AI 컨트롤러가 빙의한다. 컨트롤러 클래스는 블루프린트에서 바꿀 수 있다(7-2에서 StateTree용으로)
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (TestAttack && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(TestAttackTimer, this, &AEnemyCharacter::RequestTestAttack, TestAttackInterval, true, TestAttackInterval);
	}
}

void AEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TestAttackTimer);
		World->GetTimerManager().ClearTimer(DeathTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AEnemyCharacter::RequestAttack(const UAttackDataAsset* Attack)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !Attack)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = CombatTags::Event_AttackRequest;
	Payload.Instigator = this;
	Payload.Target = this;
	Payload.OptionalObject = Attack;
	ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
}

void AEnemyCharacter::RequestTestAttack()
{
	RequestAttack(TestAttack);
}

void AEnemyCharacter::HandleDeathStarted()
{
	Super::HandleDeathStarted();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TestAttackTimer);
		World->GetTimerManager().SetTimer(DeathTimer, this, &AEnemyCharacter::RemoveFromLevel, FMath::Max(DeathRemovalTime, 0.01f), false);
	}

	OnEnemyDied.Broadcast();
}

void AEnemyCharacter::RemoveFromLevel()
{
	UE_LOG(LogFirstAction, Log, TEXT("[%s] 사망 후 제거"), *GetName());
	Destroy();
}
