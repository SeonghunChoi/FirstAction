// 플레이어와 적이 함께 상속하는 전투 캐릭터 기반 클래스. (PLAN 4.1, 4.2)
// ASC와 속성 세트를 갖고, 시작 어빌리티와 시작 이펙트를 부여한다.
// 입력이나 AI는 여기에 직접 공격 함수를 부르지 않고, ASC에 어빌리티 활성화만 요청한다(설계 규칙 1).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "CombatCharacterBase.generated.h"

class UAbilitySystemComponent;
class UCombatAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UPrimaryDataAsset;
class UStaticMeshComponent;
class UMeleeTraceComponent;
class UMotionWarpingComponent;
class UPhysicalAnimationComponent;
class UPartialHitReactionComponent;
class UCombatCharacterData;
class UCharacterMovementComponent;
struct FGameplayEventData;

// 시작할 때 부여할 어빌리티 한 줄. 같은 어빌리티 클래스를 다른 입력 태그로 여러 번 부여할 수 있다.
USTRUCT(BlueprintType)
struct FCombatAbilityGrant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> Ability;

	// 이 어빌리티를 실행할 입력 태그. 입력 없이 이벤트로만 발동하는 어빌리티(피격 반응 등)는 비워 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	// 이 부여 건에 묶을 데이터 (예: GA_MeleeAttack의 시작 공격 데이터). 스펙의 SourceObject가 된다
	// 비우면 캐릭터 자신이 SourceObject가 된다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UPrimaryDataAsset> AbilityData;
};

// 입력이 버퍼에 기록될 때 알린다. 실행 중인 어빌리티가 구간 안에서 바로 꺼내 가는 데 쓴다 (Step 4-3)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatInputBuffered, const FGameplayTag& /*InputTag*/);

// 입력 버퍼에 보관 중인 입력 하나 (Step 4-2)
USTRUCT(BlueprintType)
struct FCombatBufferedInput
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Input")
	FGameplayTag InputTag;

	// 누른 시각 (월드 시간, 초)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Input")
	double PressedTime = 0.0;
};

UCLASS(Abstract)
class FIRSTACTION_API ACombatCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACombatCharacterBase();

	// IAbilitySystemInterface: 다른 시스템이 이 액터의 ASC를 찾는 표준 통로
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	UMeleeTraceComponent* GetMeleeTrace() const { return MeleeTrace; }
	UMotionWarpingComponent* GetMotionWarping() const { return MotionWarping; }
	UPartialHitReactionComponent* GetPartialHitReaction() const { return PartialHitReaction; }

	// 몸의 성질 데이터 (Step 6-3). 타격 해석 매니저(6-4)가 양쪽 질량을 여기서 읽는다
	const UCombatCharacterData* GetCharacterData() const { return CharacterData; }

	// 공격 대상을 고른다 (Step 6-2). 공격 어빌리티가 콤보 타마다 묻는다. 없으면 nullptr
	// MaxDistance: 공격 데이터의 MaxWarpDistance. 캐릭터는 공격을 모르고 이 거리만 받는다
	// 기본 구현: 기준 방향(GetAttackTargetDirection) 원뿔 안, 몸끼리 간격이 MaxDistance 이하인 대상 중 각도가 가장 작은 대상
	// 플레이어는 락온 중이면 락온 대상(6-14), AI는 쫓는 대상(Step 7)을 돌려주도록 재정의한다. 이 둘은 거리 밖이어도 돌려준다
	virtual AActor* FindAttackTarget(float MaxDistance) const;

	// 가드·패리가 성립하는 정면 각도(전체, 도). 0이면 가드가 막지 못한다 (6-10 결정: 캐릭터마다)
	float GetGuardAngle() const { return GuardAngle; }

	// 입력 태그의 버튼이 지금 눌려 있는가 (가드처럼 누르고 있는 동안 유지하는 어빌리티용, 6-11)
	bool IsInputTagHeld(const FGameplayTag& InputTag) const { return HeldInputTags.Contains(InputTag); }

	// 이동 입력 입구. 가드 중(State.Guarding)이면 이동 대신 그 방향으로 몸만 돌린다 (6-11 결정)
	// 플레이어 스틱도, 나중의 AI 이동 요청도 이 입구를 지나므로 가드 어빌리티는 누가 움직이는지 모른다(규칙 1)
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;

	virtual void Tick(float DeltaSeconds) override;

	// 회피 방향 (Step 6-9). 기본은 공격 대상 탐색과 같은 기준: 이동 입력(스틱)이 있으면 그 방향, 없으면 정면 (6-9 결정)
	// AI(Step 7)는 필요하면 재정의한다
	virtual FVector GetDodgeDirection() const { return GetAttackTargetDirection(); }

	// 입력 태그로 어빌리티 실행을 요청한다. 플레이어 입력도 AI도 이 두 함수만 부른다(설계 규칙 1)
	//   눌림: 짝인 어빌리티가 실행 중이 아니면 활성화를 시도하고, 실행 중이면 입력 눌림을 전달한다
	//   뗌:   실행 중인 어빌리티에 입력 뗌을 전달한다 (가드처럼 누르고 있는 동안만 유지하는 어빌리티용)
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

	// 입력 버퍼에서 받아들일 수 있는 입력을 꺼낸다 (Step 4-2)
	// 보관 시간 안에 들어왔고 AcceptedInputTags 중 하나이면 OutInputTag에 담고 버퍼를 비운 뒤 true.
	// 받아들일 수 없는 입력(예: 공격 콤보 구간에서의 회피 입력)은 다른 소비자를 위해 버퍼에 남긴다
	bool ConsumeBufferedInput(const FGameplayTagContainer& AcceptedInputTags, FGameplayTag& OutInputTag);

	// 입력이 버퍼에 기록된 직후 호출된다
	FOnCombatInputBuffered OnInputBuffered;

	// 히트스톱: Duration초 동안 이 캐릭터만 CustomTimeDilation을 낮춘다 (Step 6-1)
	// 애니메이션, 노티파이 구간, 이동이 함께 멈추므로 콤보 타이밍은 어긋나지 않는다.
	// 이미 멈춰 있으면 남은 시간과 새 시간 중 긴 쪽으로 맞춘다 (한 휘두름에 여러 명을 맞혀도 한 번처럼)
	// 각자 자기에게만 건다: 때린 쪽은 공격 어빌리티가, 맞은 쪽은 Event.HitReact를 받은 자신이 부른다(6-1 결정)
	void ApplyHitStop(float Duration);

	// 잠시 느려짐: Duration초(실제 시간) 동안 시간 배율 Dilation (Step 6-13, 저스트 회피 보상)
	// 히트스톱과 따로 관리하고, 둘이 겹치면 더 느린 배율을 적용한다. 이미 더 길고 더 느린 슬로모션이 걸려 있으면 무시한다
	void ApplyTimeSlow(float Duration, float Dilation);

	// 타격 후 이동: Direction(수평) 쪽으로 Distance(cm, 음수면 반대쪽)를 Duration초에 걸쳐 움직인다 (Step 6-5)
	// 루트 모션 몽타주 재생 중: 애니메이션 루트 모션에 남은 이동량을 더한다 (애니메이션 전진은 살아 있음)
	// 그 밖: Root Motion Source(Override)로 같은 거리를 옮긴다. 엔진은 애니메이션 루트 모션이 있으면 Root Motion Source를 무시한다
	// 각자 자기에게만 건다: 때린 쪽은 공격 어빌리티가, 맞은 쪽은 Event.HitReact를 받은 자신이 부른다(규칙 6)
	// 새 타격이 오면 남은 이동은 버리고 새 이동으로 바꾼다
	void ApplyHitDisplacement(const FVector& Direction, float Distance, float Duration);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// 이 캐릭터의 몸의 성질 (질량 등). 플레이어와 적이 같은 형식 (Step 6-3, 설계 규칙 6-3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Data")
	TObjectPtr<const UCombatCharacterData> CharacterData;

	// 시작할 때 부여할 어빌리티와 그 입력 태그 (공격, 회피, 피격 반응 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Startup", meta = (TitleProperty = "InputTag"))
	TArray<FCombatAbilityGrant> StartupAbilities;

	// 시작할 때 자신에게 적용할 이펙트 (속성 초기값 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Startup")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	// StartupEffects를 자신에게 적용한다. 시작할 때 한 번, 훈련 더미는 리필할 때 다시 부른다 (Step 5-5)
	void ApplyStartupEffects();

	// 입력 버퍼 보관 시간(초). 이보다 오래된 입력은 버린다. 0이면 버퍼를 쓰지 않는다
	// 튜닝값이므로 C++ 기본값은 0이고, 블루프린트 기본값에서 정한다(설계 규칙 3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Input", meta = (ClampMin = "0.0", Units = "s"))
	float InputBufferDuration = 0.f;

	// 히트스톱 동안의 시간 배율. 0에 가까울수록 완전히 멈춘다. 1이면 히트스톱을 쓰지 않는다
	// 얼마나 멈출지(시간)는 공격 데이터가, 얼마나 깊게 멈출지(배율)는 캐릭터가 정한다. 튜닝값이므로 블루프린트 기본값에서 정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Feel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopTimeDilation = 1.f;

	// 공격 대상 탐색 원뿔의 전체 각도 (Step 6-2). 기준 방향 좌우로 절반씩. 0이면 대상을 찾지 않는다(워핑 없음)
	// 튜닝값이므로 블루프린트 기본값에서 정한다. 공격과 상관없이 캐릭터마다 하나(6-2 결정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Targeting", meta = (ClampMin = "0.0", ClampMax = "360.0", Units = "Degrees"))
	float AttackTargetConeAngle = 0.f;

	// 가드·패리가 성립하는 정면 각도(전체, 도). 좌우로 절반씩 (6-10 결정). 튜닝값이므로 블루프린트 기본값에서 정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Defense", meta = (ClampMin = "0.0", ClampMax = "360.0", Units = "Degrees"))
	float GuardAngle = 0.f;

	// 가드 중 스틱 방향으로 도는 속도 (도/초). 0이면 가드 중 돌지 않는다 (6-11 결정). 블루프린트 기본값에서 정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Defense", meta = (ClampMin = "0.0", Units = "DegreesPerSecond"))
	float GuardTurnRate = 0.f;

	// 대상 탐색의 기준 방향. 이동 입력(플레이어의 스틱)이 있으면 그 방향, 없으면 캐릭터 정면 (6-2 결정)
	// 이동 입력은 템플릿 Move가 카메라 기준으로 만든 월드 방향이다. AI는 이동 입력이 없으므로 정면이 된다
	virtual FVector GetAttackTargetDirection() const;

private:
	// 맞은 쪽 통로: Event.HitReact의 타격 해석 결과(FHitResolution, 6-4)에서 자기 몫을 꺼내 스스로 적용 (Step 6-1)
	// 6-7의 GA_HitReact도 같은 이벤트를 받아 반응 몽타주를 재생한다. 히트스톱은 캐릭터, 반응은 어빌리티
	void HandleHitReactEvent(const FGameplayEventData* Payload);

	void EndHitStop();
	void EndTimeSlow();

	// 시간 배율 출처(히트스톱, 슬로모션) 중 가장 느린 것을 CustomTimeDilation에 적용한다
	void UpdateTimeDilation();

	// 저스트 회피 성공(6-13): 자신을 뺀 모든 전투 캐릭터를 잠시 느리게 한다. 각 캐릭터는 자기 ApplyTimeSlow로 스스로 느려진다
	void StartJustDodgeSlow();

	FTimerHandle HitStopTimer;
	FTimerHandle TimeSlowTimer;

	// 지금 걸려 있는 출처별 시간 배율 (1이면 없음)
	float ActiveHitStopDilation = 1.f;
	float ActiveTimeSlowDilation = 1.f;

	// 캐릭터 무브먼트의 ProcessRootMotionPostConvertToWorld에 연결 (6-5). 월드 기준 애니메이션 루트 모션에 이번 프레임 몫을 더한다
	// Motion Warping(6-2)은 Pre 쪽을 쓰므로 겹치지 않는다. 애니메이션 루트 모션이 있는 프레임에만 불린다
	FTransform ProcessHitDisplacementRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	// 루트 모션에 더할 남은 이동량(월드, cm)과 남은 시간(캐릭터 시간, 초)
	FVector PendingHitDisplacement = FVector::ZeroVector;
	float PendingHitDisplacementTime = 0.f;

	// 몽타주가 이동 도중 끝나 루트 모션이 끊기면 남은 이동이 다음 몽타주로 새지 않도록 버리는 월드 시각
	double PendingHitDisplacementExpireTime = 0.0;

	void GrantStartupAbilitiesAndEffects();

	// 무기 판정이 대상을 맞힘 → 자신의 ASC에 Event.Hit 게임플레이 이벤트를 보낸다 (Step 5-3)
	// 판정 컴포넌트는 GAS를 모르고, 공격 어빌리티는 판정 컴포넌트를 모른다. 둘을 잇는 다리
	void HandleMeleeTraceHit(const FHitResult& Hit);

	// 새 어빌리티를 시작하지 못한 입력을 기록한다. 가장 최근 입력 하나만 남긴다
	void BufferInput(const FGameplayTag& InputTag);

	// Window.Cancel이 붙는 순간: 버퍼에 남은 입력으로 어빌리티를 다시 시도한다 (6-9 결정: 회피 선입력)
	// 공격 콤보는 공격 어빌리티가 Window.Combo에서 꺼내고, 그 밖의 행동(회피 등)은 여기서 살아난다
	void OnCancelWindowTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 버퍼의 입력과 짝인 어빌리티를 시작해 본다. 하나라도 시작하면 버퍼를 비운다
	void TryActivateBufferedInput();

	// 실행 중 상태값: 디버깅용으로 보기만 (설계 규칙 9)
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Input")
	FCombatBufferedInput BufferedInput;

	// 지금 눌려 있는 입력 태그 (눌림에 추가, 뗌에 제거)
	TSet<FGameplayTag> HeldInputTags;

	// 가드 중 이번 프레임에 들어온 이동 입력. Tick에서 그 방향으로 돌고 비운다
	// (템플릿 이동은 앞뒤·좌우 두 번 나눠 부르므로 모아서 한 번에 처리한다)
	FVector GuardTurnInput = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatAttributeSet> CombatAttributeSet;

	// 무기 메시. 손 소켓(HandGrip_R)에 붙는다. 블루프린트에서 메시 에셋만 고르고, 무기가 없으면 비워 둔다 (Step 5-1)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 무기 판정 (Step 5-1)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMeleeTraceComponent> MeleeTrace;

	// 모션 워핑 (Step 6-2). 공격 어빌리티가 워핑 목표를 등록하고, 몽타주의 Motion Warping 구간이 루트 모션을 보정한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	// 물리 바디를 애니메이션 자세로 당긴다. 부위 반응이 흔드는 동안 쓴다 (Step 6-8)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimation;

	// 부위 물리 반응 (Step 6-8). 타격 해석 결과의 속도 변화를 맞은 부위에 준다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPartialHitReactionComponent> PartialHitReaction;

	// 시작 어빌리티/이펙트를 두 번 부여하지 않도록
	bool bStartupGranted = false;
};
