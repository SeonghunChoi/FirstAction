// 스테이지 구간 하나 (게임 흐름, 9-4 이후)
// 플레이어가 구간(Area 상자)에 들어오면 시작한다: 뒤 문을 닫고 웨이브(Waves)를 차례로 내보낸다.
// 한 웨이브의 적은 한꺼번에(서로 다른 생성 위치에) 나오고, 그 웨이브가 모두 쓰러지면 다음 웨이브가 나온다.
// 다 잡으면 앞 문을 열고 Cleared. 플레이어가 죽어 "다시 하기"를 고르면 ResetEncounter로 시작 전 상태(적 제거, 문 원래대로)로 되돌리고
// 플레이어는 RespawnPoint(구간 입구, 상자 밖)에서 다시 들어온다.
// 템플릿 ACombatEnemySpawner(한 종류를 한 마리씩)를 대신한다. 구간마다 적 종류를 섞고, 문과 다시 하기를 함께 다뤄야 해서 새로 만들었다.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageEncounter.generated.h"

class UBoxComponent;
class AStageDoor;
class AEnemyCharacter;
class AStageEncounter;

UENUM(BlueprintType)
enum class EStageEncounterState : uint8
{
	Idle,       // 아직 들어오지 않음
	Active,     // 전투 중
	Cleared     // 다 잡음
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnStageEncounterEvent, AStageEncounter* /*Encounter*/);

// 함께 나오는 적 묶음 하나
USTRUCT(BlueprintType)
struct FStageWave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	TArray<TSubclassOf<AEnemyCharacter>> Enemies;
};

UCLASS()
class FIRSTACTION_API AStageEncounter : public AActor
{
	GENERATED_BODY()

public:
	AStageEncounter();

	// 플레이어가 구간 안에 있으면 시작한다 (상자 진입, 또는 구간 안에서 생성됐을 때 게임 모드가 부른다)
	bool TryStart(APawn* PlayerPawn);

	// 진행 중이던 구간을 시작 전 상태로 되돌린다. 남은 적을 지우고 문을 원래대로 (다시 하기)
	void ResetEncounter();

	// 다시 하기 때 플레이어가 서는 곳. RespawnPoint가 없으면 구간 상자 위치
	FTransform GetRespawnTransform() const;

	// 구간 입구 (길 안내 목적지): 들어올 때 닫히는 문, 없으면 다시 하기 위치
	FVector GetEntranceLocation() const;

	int32 GetStageNumber() const { return StageNumber; }
	EStageEncounterState GetState() const { return State; }

	FOnStageEncounterEvent OnStarted;
	FOnStageEncounterEvent OnCleared;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 플레이어가 들어오면 시작하는 구역
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage")
	TObjectPtr<UBoxComponent> Area;

	// 화면에 보이는 구간 번호 (1부터)
	UPROPERTY(EditInstanceOnly, Category = "Stage", meta = (ClampMin = "1"))
	int32 StageNumber = 1;

	// 웨이브 (차례로). 한 웨이브의 적은 한꺼번에 나온다
	UPROPERTY(EditInstanceOnly, Category = "Stage")
	TArray<FStageWave> Waves;

	// 적이 나오는 곳. 웨이브마다 플레이어에게서 먼 곳부터 하나씩 쓴다(같은 웨이브는 서로 다른 곳). 비면 구간 상자 위치
	UPROPERTY(EditInstanceOnly, Category = "Stage")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	// 구간 시작부터 첫 웨이브까지
	UPROPERTY(EditInstanceOnly, Category = "Stage", meta = (ClampMin = "0.0", Units = "s"))
	float FirstSpawnDelay = 1.0f;

	// 웨이브의 마지막 적이 쓰러진 뒤 다음 웨이브(또는 클리어)까지
	UPROPERTY(EditInstanceOnly, Category = "Stage", meta = (ClampMin = "0.0", Units = "s"))
	float NextSpawnDelay = 1.5f;

	// 시작할 때 닫는 문 (들어온 입구)
	UPROPERTY(EditInstanceOnly, Category = "Stage")
	TArray<TObjectPtr<AStageDoor>> DoorsToCloseOnStart;

	// 다 잡으면 여는 문 (다음 구간 입구)
	UPROPERTY(EditInstanceOnly, Category = "Stage")
	TArray<TObjectPtr<AStageDoor>> DoorsToOpenOnClear;

	// 다시 하기 때 플레이어가 서는 곳 (구간 상자 밖 입구 앞, 화살표 방향을 본다)
	UPROPERTY(EditInstanceOnly, Category = "Stage")
	TObjectPtr<AActor> RespawnPoint;

	// 실행 중 상태
	UPROPERTY(VisibleInstanceOnly, Category = "Stage|Runtime")
	EStageEncounterState State = EStageEncounterState::Idle;

	UPROPERTY(VisibleInstanceOnly, Category = "Stage|Runtime")
	int32 NextWaveIndex = 0;

private:
	UFUNCTION()
	void OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEnemyDied();

	void ScheduleSpawn(float Delay);
	void SpawnNext();
	void PruneAlive();
	FTransform ChooseSpawnTransform(TArray<const AActor*>& UsedPoints) const;

	TArray<TWeakObjectPtr<AEnemyCharacter>> AliveEnemies;
	TWeakObjectPtr<APawn> PlayerPawn;
	TArray<bool> DoorsInitiallyOpen;   // DoorsToCloseOnStart의 시작 전 상태 (되돌리기용)
	FTimerHandle SpawnTimer;
};
