// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstActionGameMode.h"

#include "Camera/CameraActor.h"
#include "Combat/CombatCharacterBase.h"
#include "ContentStreaming.h"
#include "EngineUtils.h"
#include "FirstAction.h"
#include "FirstActionPlayerController.h"
#include "Game/StageEncounter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PipelineStateCache.h"
#include "ShaderPipelineCache.h"
#include "TimerManager.h"
#include "UI/GameMenuWidget.h"

static TAutoConsoleVariable<bool> CVarSkipMenu(
	TEXT("FirstAction.SkipMenu"),
	false,
	TEXT("켜면 로딩이 끝난 뒤 시작 메뉴 없이 바로 플레이어를 만든다 (개발 시험용)"));

AFirstActionGameMode::AFirstActionGameMode()
{
}

void AFirstActionGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 처음부터 다시 하기는 맵을 SkipMenu 옵션으로 다시 연다
	bSkipMenu = UGameplayStatics::HasOption(Options, TEXT("SkipMenu"));
}

void AFirstActionGameMode::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AStageEncounter> It(GetWorld()); It; ++It)
	{
		Encounters.Add(*It);
		It->OnStarted.AddUObject(this, &AFirstActionGameMode::HandleEncounterStarted);
		It->OnCleared.AddUObject(this, &AFirstActionGameMode::HandleEncounterCleared);
	}
	Encounters.Sort([](const TWeakObjectPtr<AStageEncounter>& A, const TWeakObjectPtr<AStageEncounter>& B)
	{
		return A->GetStageNumber() < B->GetStageNumber();
	});
	UE_LOG(LogFirstAction, Log, TEXT("게임 흐름: 구간 %d개"), Encounters.Num());

	if (Player.IsValid() && !bLoadingStarted)
	{
		BeginLoading();
	}
}

void AFirstActionGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 플레이어를 바로 만들지 않는다(기본 동작: RestartPlayer). 로딩이 끝나고 메뉴에서 시작할 때 만든다
	Player = Cast<AFirstActionPlayerController>(NewPlayer);
	if (!Player.IsValid())
	{
		UE_LOG(LogFirstAction, Warning, TEXT("게임 흐름: 플레이어 컨트롤러가 AFirstActionPlayerController가 아니라 바로 시작합니다"));
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
		return;
	}

	// 맵을 여는 중에는 액터 BeginPlay 전에 불린다. 위젯과 타이머는 BeginPlay 뒤에 시작한다
	if (HasActorBegunPlay())
	{
		BeginLoading();
	}
}

// ---------------------------------------------------------------- 로딩

void AFirstActionGameMode::BeginLoading()
{
	bLoadingStarted = true;
	FlowState = EGameFlowState::Loading;
	LoadingStartTime = FPlatformTime::Seconds();

	// 메뉴 카메라 쪽에서 텍스처 스트리밍이 일어나도록 먼저 시점을 옮긴다
	if (AActor* ViewTarget = FindMenuViewTarget())
	{
		Player->SetViewTarget(ViewTarget);
	}
	Player->ShowMenu(EGameMenuMode::Loading);

	GetWorldTimerManager().SetTimer(LoadingTimer, this, &AFirstActionGameMode::CheckLoadingDone, 0.25f, true);
}

void AFirstActionGameMode::CheckLoadingDone()
{
	const double Elapsed = FPlatformTime::Seconds() - LoadingStartTime;
	const int32 WantingResources = IStreamingManager::Get().GetNumWantingResources();
	const uint32 PendingPSOPrecache = PipelineStateCache::NumActivePrecacheRequests();
	const uint32 PendingPSOCache = FShaderPipelineCache::NumPrecompilesRemaining();
	const bool bAsyncLoading = IsAsyncLoading();

	const bool bReady = WantingResources == 0 && PendingPSOPrecache == 0 && PendingPSOCache == 0 && !bAsyncLoading;
	if ((bReady && Elapsed >= MinLoadingTime) || Elapsed >= MaxLoadingTime)
	{
		GetWorldTimerManager().ClearTimer(LoadingTimer);
		UE_LOG(LogFirstAction, Log, TEXT("게임 흐름: 로딩 %s (%.1f초, 텍스처 대기 %d, PSO 준비 %u/%u, 비동기 로딩 %d)"),
			bReady ? TEXT("완료") : TEXT("시간 초과"), Elapsed, WantingResources, PendingPSOPrecache, PendingPSOCache, bAsyncLoading ? 1 : 0);

		if (bSkipMenu || CVarSkipMenu.GetValueOnGameThread())
		{
			StartGame();
		}
		else
		{
			EnterMenu();
		}
	}
}

void AFirstActionGameMode::EnterMenu()
{
	FlowState = EGameFlowState::Menu;
	Player->ShowMenu(EGameMenuMode::Main);
}

AActor* AFirstActionGameMode::FindMenuViewTarget() const
{
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(MenuCameraTag))
		{
			return *It;
		}
	}
	return Player.IsValid() ? const_cast<AFirstActionGameMode*>(this)->FindPlayerStart(Player.Get()) : nullptr;
}

// ---------------------------------------------------------------- 시작·다시 하기

void AFirstActionGameMode::StartGame()
{
	if (!Player.IsValid() || Player->GetPawn())
	{
		return;
	}
	SpawnPlayer(nullptr);
}

void AFirstActionGameMode::SpawnPlayer(const FTransform* SpawnTransform)
{
	GetWorldTimerManager().ClearTimer(MenuDelayTimer);
	SetGamePaused(false);

	if (SpawnTransform)
	{
		RestartPlayerAtTransform(Player.Get(), *SpawnTransform);
	}
	else
	{
		RestartPlayer(Player.Get());
	}

	ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(Player->GetPawn());
	if (!Character)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("게임 흐름: 플레이어 생성 실패"));
		return;
	}
	Character->OnDied.AddUObject(this, &AFirstActionGameMode::HandlePlayerDied);

	FlowState = EGameFlowState::Playing;
	Player->ShowMenu(EGameMenuMode::Hidden);

	// 구간 안에서 생겼으면(첫 구간 시작점 등) 겹침 이벤트를 기다리지 않고 바로 시작
	for (const TWeakObjectPtr<AStageEncounter>& Encounter : Encounters)
	{
		if (Encounter.IsValid())
		{
			Encounter->TryStart(Character);
		}
	}
}

void AFirstActionGameMode::RetryStage()
{
	// 아직 깨지 않은 첫 구간의 입구에서 다시. 구간을 깬 직후에 죽었으면 다음 구간 입구(깬 구간 쪽, 문이 열려 있다)
	AStageEncounter* Encounter = nullptr;
	for (const TWeakObjectPtr<AStageEncounter>& Candidate : Encounters)
	{
		if (Candidate.IsValid() && Candidate->GetState() != EStageEncounterState::Cleared)
		{
			Encounter = Candidate.Get();
			break;
		}
	}
	if (!Encounter && Encounters.Num() > 0)
	{
		Encounter = Encounters.Last().Get();
	}
	if (!Player.IsValid() || !Encounter)
	{
		// 구간이 없는 맵(깃 버전 시험 맵)이나 첫 구간 전: 처음부터
		RestartFromBeginning();
		return;
	}

	Encounter->ResetEncounter();
	const FTransform RespawnTransform = Encounter->GetRespawnTransform();

	if (APawn* OldPawn = Player->GetPawn())
	{
		Player->UnPossess();
		OldPawn->Destroy();
	}
	UE_LOG(LogFirstAction, Log, TEXT("게임 흐름: %d구간 입구에서 다시"), Encounter->GetStageNumber());
	SpawnPlayer(&RespawnTransform);
	UpdateRouteGuide();
}

void AFirstActionGameMode::UpdateRouteGuide()
{
	if (!Player.IsValid())
	{
		return;
	}

	// 앞 구간을 깨고 아직 들어가지 않은 다음 구간이 있으면 그 입구로 안내한다
	bool bAnyCleared = false;
	for (const TWeakObjectPtr<AStageEncounter>& Encounter : Encounters)
	{
		if (!Encounter.IsValid())
		{
			continue;
		}
		if (Encounter->GetState() == EStageEncounterState::Cleared)
		{
			bAnyCleared = true;
			continue;
		}
		if (Encounter->GetState() == EStageEncounterState::Idle && bAnyCleared)
		{
			Player->ShowRouteGuide(Encounter->GetEntranceLocation());
			return;
		}
		break;
	}
	Player->HideRouteGuide();
}

void AFirstActionGameMode::RestartFromBeginning()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)), true, TEXT("SkipMenu"));
}

void AFirstActionGameMode::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)), true);
}

void AFirstActionGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, Player.Get(), EQuitPreference::Quit, false);
}

void AFirstActionGameMode::SetGamePaused(bool bPaused)
{
	if (Player.IsValid())
	{
		Player->SetPause(bPaused);
	}
}

// ---------------------------------------------------------------- 사망·구간

void AFirstActionGameMode::HandlePlayerDied(ACombatCharacterBase* DeadCharacter)
{
	if (!Player.IsValid() || FlowState != EGameFlowState::Playing)
	{
		return;
	}
	FlowState = EGameFlowState::Dead;
	GetWorldTimerManager().SetTimer(MenuDelayTimer, this, &AFirstActionGameMode::ShowDeathMenu, FMath::Max(DeathMenuDelay, 0.01f), false);
}

void AFirstActionGameMode::ShowDeathMenu()
{
	if (Player.IsValid() && FlowState == EGameFlowState::Dead)
	{
		Player->ShowMenu(EGameMenuMode::Dead);
	}
}

void AFirstActionGameMode::HandleEncounterStarted(AStageEncounter* Encounter)
{
	CurrentEncounter = Encounter;
	if (Player.IsValid())
	{
		Player->HideRouteGuide();
		Player->ShowBanner(FText::Format(NSLOCTEXT("GameFlow", "StageStart", "STAGE {0} / {1}"), Encounter->GetStageNumber(), GetStageCount()));
	}
}

void AFirstActionGameMode::HandleEncounterCleared(AStageEncounter* Encounter)
{
	const bool bAllCleared = !Encounters.ContainsByPredicate([](const TWeakObjectPtr<AStageEncounter>& E)
	{
		return E.IsValid() && E->GetState() != EStageEncounterState::Cleared;
	});

	if (Player.IsValid())
	{
		Player->ShowBanner(bAllCleared ? NSLOCTEXT("GameFlow", "AllClear", "ALL CLEAR") : NSLOCTEXT("GameFlow", "StageClear", "STAGE CLEAR"));
	}
	UpdateRouteGuide();

	if (bAllCleared && FlowState == EGameFlowState::Playing)
	{
		FlowState = EGameFlowState::Cleared;
		GetWorldTimerManager().SetTimer(MenuDelayTimer, this, &AFirstActionGameMode::ShowClearMenu, FMath::Max(ClearMenuDelay, 0.01f), false);
	}
}

void AFirstActionGameMode::ShowClearMenu()
{
	if (Player.IsValid() && FlowState == EGameFlowState::Cleared)
	{
		Player->ShowMenu(EGameMenuMode::Cleared);
	}
}

int32 AFirstActionGameMode::GetStageCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AStageEncounter>& Encounter : Encounters)
	{
		if (Encounter.IsValid())
		{
			Count = FMath::Max(Count, Encounter->GetStageNumber());
		}
	}
	return Count;
}
