// 입력 액션과 입력 태그의 짝을 담는 데이터 에셋. (Step 2-1)
// 플레이어 입력은 이 표로 입력 액션을 입력 태그로 바꾼 뒤, ASC에 "이 태그의 어빌리티를 실행해 달라"고만 요청한다(설계 규칙 1).
// 어떤 버튼이 어떤 입력 액션인지는 Input Mapping Context(IMC)가, 어떤 입력 액션이 어떤 태그인지는 이 에셋이 정한다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CombatInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FCombatInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<const UInputAction> InputAction = nullptr;

	// 태그 선택 창에 InputTag.* 만 보이도록 제한
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

UCLASS(BlueprintType, Const)
class FIRSTACTION_API UCombatInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// 입력 태그로 어빌리티를 활성화할 입력 액션 목록 (공격, 회피, 가드)
	// 이동, 시점, 점프처럼 어빌리티가 아닌 입력은 넣지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (TitleProperty = "InputTag"))
	TArray<FCombatInputAction> AbilityInputActions;
};
