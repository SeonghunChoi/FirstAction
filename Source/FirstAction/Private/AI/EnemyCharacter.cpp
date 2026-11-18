#include "AI/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "AI/EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/AttackDataAsset.h"
#include "Combat/CombatTags.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "FirstAction.h"

AEnemyCharacter::AEnemyCharacter()
{
	// 레벨에 놓이거나 생성되면 AI 컨트롤러가 빙의한다 (7-3: 인지와 StateTree를 가진 AEnemyAIController)
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	// 몸은 컨트롤러가 보는 쪽(Focus)으로 회전 속도만큼 돈다. 이동 방향으로 돌지 않아 대상을 보며 맴돌 수 있다.
	// 회전 속도는 블루프린트의 Character Movement > Rotation Rate (공격 몽타주 루트 모션 중에는 돌지 않는다)
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
}

AActor* AEnemyCharacter::FindAttackTarget(float MaxDistance) const
{
	if (const AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		if (AActor* Target = AIController->GetCombatTarget())
		{
			// 범위 밖이어도 돌려준다. 워핑은 방향을 돌리고 최대 거리까지만 붙는다 (6-2 결정)
			return Target;
		}
	}
	return Super::FindAttackTarget(MaxDistance);
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
