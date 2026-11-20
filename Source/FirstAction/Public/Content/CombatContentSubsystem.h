// 활성 콘텐츠 세트를 들고 기본 에셋을 배포 에셋으로 바꿔 주는 게임 인스턴스 서브시스템 (Step 9-1)
// 게임(PIE 한 번)이 시작될 때 UPortfolioContentSettings의 Release 세트를 한 번 로드한다. 실패하면 Default(치환 없음)
//
// 몽타주를 재생하는 곳(공격·반응·회피·가드·사망 어빌리티)과 메시를 끼우는 곳(캐릭터 시작)이 이 함수를 거친다.
// 표에 없는 에셋은 그대로 돌려주고, 에셋마다 처음 한 번 경고를 남긴다 (9-1 질문 2 B)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CombatContentSubsystem.generated.h"

class UAnimInstance;
class UAnimMontage;
class UCombatContentSet;
class USkeletalMesh;
class UStaticMesh;

UCLASS()
class FIRSTACTION_API UCombatContentSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 활성 세트의 치환. 세트가 없거나(Default) 표에 없으면 받은 에셋을 그대로 돌려준다
	// WorldContext: 어빌리티, 캐릭터 등 월드에 속한 아무 오브젝트
	static UAnimMontage* ResolveMontage(const UObject* WorldContext, UAnimMontage* Montage);
	static USkeletalMesh* ResolveSkeletalMesh(const UObject* WorldContext, USkeletalMesh* Mesh);
	static UStaticMesh* ResolveStaticMesh(const UObject* WorldContext, UStaticMesh* Mesh);
	static TSubclassOf<UAnimInstance> ResolveAnimClass(const UObject* WorldContext, TSubclassOf<UAnimInstance> AnimClass);

	// 지금 쓰는 세트. nullptr이면 Default
	const UCombatContentSet* GetActiveSet() const { return ActiveSet; }

private:
	static UCombatContentSubsystem* Get(const UObject* WorldContext);

	// 표에 없으면 처음 한 번 경고 (Release 세트가 있을 때만)
	void WarnMissing(const UObject* DefaultAsset, const TCHAR* Kind);

	UPROPERTY()
	TObjectPtr<const UCombatContentSet> ActiveSet;

	TSet<FObjectKey> WarnedAssets;
};
