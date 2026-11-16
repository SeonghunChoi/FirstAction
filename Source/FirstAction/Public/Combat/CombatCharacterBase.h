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

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// 시작할 때 부여할 어빌리티와 그 입력 태그 (공격, 회피, 피격 반응 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Startup", meta = (TitleProperty = "InputTag"))
	TArray<FCombatAbilityGrant> StartupAbilities;

	// 시작할 때 자신에게 적용할 이펙트 (속성 초기값 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Startup")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	// 입력 버퍼 보관 시간(초). 이보다 오래된 입력은 버린다. 0이면 버퍼를 쓰지 않는다
	// 튜닝값이므로 C++ 기본값은 0이고, 블루프린트 기본값에서 정한다(설계 규칙 3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Input", meta = (ClampMin = "0.0", Units = "s"))
	float InputBufferDuration = 0.f;

private:
	void GrantStartupAbilitiesAndEffects();

	// 무기 판정이 대상을 맞힘 → 자신의 ASC에 Event.Hit 게임플레이 이벤트를 보낸다 (Step 5-3)
	// 판정 컴포넌트는 GAS를 모르고, 공격 어빌리티는 판정 컴포넌트를 모른다. 둘을 잇는 다리
	void HandleMeleeTraceHit(const FHitResult& Hit);

	// 새 어빌리티를 시작하지 못한 입력을 기록한다. 가장 최근 입력 하나만 남긴다
	void BufferInput(const FGameplayTag& InputTag);

	// 실행 중 상태값: 디버깅용으로 보기만 (설계 규칙 9)
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Input")
	FCombatBufferedInput BufferedInput;

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

	// 시작 어빌리티/이펙트를 두 번 부여하지 않도록
	bool bStartupGranted = false;
};
