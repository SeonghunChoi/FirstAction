// 공격 토큰 관리자 (Step 7-5). 동시 공격 제어는 여기 한 곳에서만 한다 (설계 규칙 8)
// 유니티로 치면 씬 단위 싱글톤 매니저가 "공격 허가증"을 빌려주고 돌려받는 구조
//
// 7-5 결정
//   - 토큰은 공격받는 대상마다 따로 있다 (질문 1 B)
//   - 대상 하나의 토큰 수는 대상의 UCombatCharacterData::AttackTokens (질문 2 A)
//   - 걸어 들어가기 시작 ~ 회복 끝까지 쥔다 (질문 3 B). 얻기와 돌려주기는 AI 태스크가 요청한다
//   - 비었을 때 기다리던 적 중 대상에게 가깝고 대상의 시선 정면에 있는 적이 먼저 (질문 4 C)
//   - 패턴마다 토큰 비용이 다르다 (질문 5 B). 비용이 남은 토큰보다 큰 패턴은 고를 수 없다
//   - 경직·사망한 보유자의 토큰은 관리자가 상태 태그를 보고 회수한다 (7-2 결정). AI는 이 반납을 모른다
//
// 차례 규칙: 차례를 기다리는 적은 매 틱 RequestTurn을 부른다. 같은 대상을 기다리는 적 중 우선순위 1등만 차례를 받는다.
// 1등에게 맞는 토큰이 모자라면 다른 적도 받지 못한다(비싼 패턴을 기다리는 적이 계속 밀리지 않게)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatTokenSubsystem.generated.h"

UCLASS()
class FIRSTACTION_API UCombatTokenSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// 차례 요청. 기다리는 동안 매 틱 부른다. 이 대상을 기다리는 요청자 중 우선순위 1등이고 남은 토큰이 MinCost 이상이면 true
	// MinCost: 요청자가 가진 패턴 중 가장 싼 토큰 비용
	bool RequestTurn(AActor* Requester, AActor* Target, int32 MinCost);

	// 기다리기를 그만둔다 (맴돌기를 벗어날 때)
	void CancelTurnRequest(AActor* Requester);

	// 토큰을 Cost개 쥔다. 남은 토큰이 모자라거나 이미 쥐고 있으면 실패
	bool AcquireTokens(AActor* Requester, AActor* Target, int32 Cost);

	// 쥔 토큰을 모두 돌려준다. 쥔 것이 없으면 아무 일도 하지 않는다. Reason은 로그용
	void ReleaseTokens(AActor* Requester, const TCHAR* Reason);

	int32 GetMaxTokens(const AActor* Target) const;
	int32 GetFreeTokens(const AActor* Target) const;
	int32 GetHeldTokens(AActor* Requester) const;

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	struct FTokenHold
	{
		TWeakObjectPtr<AActor> Target;
		int32 Count = 0;
	};

	struct FTurnRequest
	{
		TWeakObjectPtr<AActor> Target;
		double Time = 0.0;
	};

	// 우선순위 비용: 몸 간격(cm) + 대상 시선에서 벗어난 각도(도) × 각도 벌점. 작을수록 먼저
	float GetPriorityCost(const AActor* Requester, const AActor* Target) const;

	// 지금도 기다리는 중인 요청인가 (최근 몇 틱 안에 다시 불렀는가)
	bool IsRequestFresh(const FTurnRequest& Request, double Now) const;

	// 경직·사망·사라진 보유자의 토큰 회수, 오래된 요청 정리
	void ReclaimTokens();

	void DrawDebug(double Now) const;

	// 보유자 → 쥔 토큰
	TMap<TWeakObjectPtr<AActor>, FTokenHold> Holds;

	// 요청자 → 기다리는 대상과 마지막 요청 시각
	TMap<TWeakObjectPtr<AActor>, FTurnRequest> Requests;
};
