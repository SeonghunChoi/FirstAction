// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FirstActionGameMode.generated.h"

class AFirstActionPlayerController;
class ACombatCharacterBase;
class AStageEncounter;

// 게임 흐름 단계 (9-4 이후)
UENUM(BlueprintType)
enum class EGameFlowState : uint8
{
	Loading,    // 맵·텍스처·셰이더가 준비될 때까지. 플레이어 없음
	Menu,       // 맵 풍경 위 시작 메뉴. 플레이어 없음
	Playing,
	Dead,       // 플레이어 사망, 다시 하기 메뉴
	Cleared     // 모든 구간 클리어
};

/**
 *  게임 흐름 (템플릿 GameMode를 고쳐 씀, 9-4 이후)
 *  - 맵을 열면 바로 플레이어를 만들지 않고 로딩 화면을 띄운 채 텍스처 스트리밍·PSO 준비·비동기 로딩이 끝나길 기다린다
 *  - 준비되면 맵 풍경(MenuCamera 태그 카메라, 없으면 플레이어 시작점) 위에 시작 메뉴. 시작하면 그때 플레이어를 만든다
 *  - 플레이어가 죽으면 사망 메뉴: 다시 하기(죽은 구간 입구, AStageEncounter), 처음부터(맵 다시 열기), 메뉴로
 *  - 모든 AStageEncounter를 깨면 클리어 메뉴
 *  맵을 모르고, 맵에 놓인 구간과 카메라만 찾는다. 깃 버전(구간 없음)은 다시 하기가 맵 다시 열기다
 */
UCLASS(abstract)
class AFirstActionGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFirstActionGameMode();

	// 메뉴에서 고른 동작 (컨트롤러가 부른다. 블루프린트·시험 스크립트에서도 부를 수 있다)
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void RetryStage();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void RestartFromBeginning();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void QuitGame();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void SetGamePaused(bool bPaused);

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	EGameFlowState GetFlowState() const { return FlowState; }

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// 로딩 화면 최소 시간 (준비가 빨라도 깜빡이지 않게)
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow", meta = (ClampMin = "0.0", Units = "s"))
	float MinLoadingTime = 1.0f;

	// 이 시간이 지나면 준비가 덜 돼도 넘어간다
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow", meta = (ClampMin = "1.0", Units = "s"))
	float MaxLoadingTime = 30.0f;

	// 사망부터 사망 메뉴까지 (쓰러지는 모습을 보여 주는 시간)
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow", meta = (ClampMin = "0.0", Units = "s"))
	float DeathMenuDelay = 3.0f;

	// 마지막 구간 클리어부터 클리어 메뉴까지
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow", meta = (ClampMin = "0.0", Units = "s"))
	float ClearMenuDelay = 3.0f;

	// 메뉴 배경으로 쓸 카메라 액터의 태그
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	FName MenuCameraTag = TEXT("MenuCamera");

	UPROPERTY(VisibleInstanceOnly, Category = "Game Flow")
	EGameFlowState FlowState = EGameFlowState::Loading;

private:
	void BeginLoading();
	void CheckLoadingDone();
	void EnterMenu();
	void SpawnPlayer(const FTransform* SpawnTransform);
	void HandlePlayerDied(ACombatCharacterBase* DeadCharacter);
	void HandleEncounterStarted(AStageEncounter* Encounter);
	void HandleEncounterCleared(AStageEncounter* Encounter);
	void ShowDeathMenu();
	void ShowClearMenu();
	void UpdateRouteGuide();
	AActor* FindMenuViewTarget() const;
	int32 GetStageCount() const;

	TWeakObjectPtr<AFirstActionPlayerController> Player;
	TArray<TWeakObjectPtr<AStageEncounter>> Encounters;
	TWeakObjectPtr<AStageEncounter> CurrentEncounter;
	bool bSkipMenu = false;
	bool bLoadingStarted = false;
	double LoadingStartTime = 0.0;
	FTimerHandle LoadingTimer;
	FTimerHandle MenuDelayTimer;
};
