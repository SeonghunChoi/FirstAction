// 적 캐릭터. 플레이어와 같은 기반 클래스, 같은 어빌리티를 쓴다 (Step 7-1, 핵심 메시지)
// 템플릿 ACombatEnemy의 판정: 전투 코드는 복제하지 않고 기반 클래스를 상속, 사망 알림과 사망 후 제거 타이머만 가져온다 (TemplateReview)
// 무엇을 할지 고르는 AI(StateTree 등)는 7-2 이후. 이 클래스는 "적이라는 몸"과, AI 전 시험용 공격 요청만 가진다.

#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatCharacterBase.h"
#include "EnemyCharacter.generated.h"

class UAttackDataAsset;
class UEnemyPatternDataAsset;
class UWidgetComponent;

// 사망 알림 (템플릿 ACombatEnemy의 FOnEnemyDied와 같은 모양, 이름은 겹치지 않게). 스포너가 전멸을 세는 데 쓴다
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyCharacterDied);

UCLASS()
class FIRSTACTION_API AEnemyCharacter : public ACombatCharacterBase
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual void HandleDeathStarted() override;

	// 공격 워핑 대상 = AI가 쫓는 대상 (6-2 결정). AI 대상이 없으면 기반 클래스의 정면 원뿔 탐색(시험 공격용)
	virtual AActor* FindAttackTarget(float MaxDistance) const override;

	const UEnemyPatternDataAsset* GetPatternData() const { return PatternData; }

	UPROPERTY(BlueprintAssignable, Category = "Combat|Enemy")
	FOnEnemyCharacterDied OnEnemyDied;

	// 공격을 요청한다: 공격 데이터를 실은 Event.AttackRequest를 자기 ASC에 보낸다 (규칙 1: 활성화 요청만)
	// AI(7-4 패턴 선택)와 아래 시험 공격이 이 함수 하나를 쓴다
	void RequestAttack(const UAttackDataAsset* Attack);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 사망 후 레벨에서 제거되기까지의 시간 (초, 템플릿 DeathRemovalTime). 블루프린트 기본값에서 정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Enemy", meta = (ClampMin = "0.0", Units = "s"))
	float DeathRemovalTime = 0.f;

	// 적 종류의 패턴 데이터: 인지·반응 성향(7-3), 패턴 목록(7-4). AI 컨트롤러가 빙의할 때 읽는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Enemy")
	TObjectPtr<const UEnemyPatternDataAsset> PatternData;

	// 시험용 공격 (AI 전): 레벨에 놓은 적마다 정하면 TestAttackInterval마다 이 공격을 요청한다 (규칙 9: 배치별 초기값)
	// 플레이어 쪽 가드·패리·회피·피격 반응을 적 AI 없이 시험한다. 비워 두면 공격하지 않는다
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Combat|Enemy|Test")
	TObjectPtr<const UAttackDataAsset> TestAttack;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Combat|Enemy|Test", meta = (ClampMin = "0.5", Units = "s"))
	float TestAttackInterval = 3.f;

	// 머리 위 체력·경직치 바와 조준 표시 (9-4 이후 타겟팅 UI, UEnemyStatusWidget). 화면 공간, 캡슐 위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Enemy")
	TObjectPtr<UWidgetComponent> StatusWidget;

	// 캡슐 꼭대기에서 표시까지 높이
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Enemy", meta = (Units = "Centimeters"))
	float StatusWidgetHeight = 45.f;

private:
	void RequestTestAttack();
	void RemoveFromLevel();

	FTimerHandle TestAttackTimer;
	FTimerHandle DeathTimer;
};
