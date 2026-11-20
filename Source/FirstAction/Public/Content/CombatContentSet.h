// 콘텐츠 세트: 기본 에셋 → 배포 에셋 치환 표 (Step 9-1, PLAN 5.2 규칙 4)
//
// 9-1 결정:
//   질문 1 B: 데이터(공격, 캐릭터, 패턴)는 한 벌만 두고, 몽타주와 메시만 바꿔 끼운다.
//             규칙 값이 두 벌로 갈라지지 않아 "공격 하나는 데이터 한 장"이 배포 빌드에서도 유지된다
//   질문 2 B: 표에 없는 에셋은 기본 에셋을 쓰고, 처음 쓰일 때 경고를 한 번 남긴다
//   질문 3 C: 짝끼리 구간을 비교해, 빠지면 동작이 깨지는 것(태그 구간·판정·워핑 종류, 몽타주 구간 이름,
//             메시 소켓)은 오류, 개수 차이는 경고 (8-3 기준과 같음)
//
// Default 세트는 따로 없다. 치환하지 않는 것이 곧 Default다.
// Release 세트는 Content/ThirdParty/(git 제외)에 두고 UPortfolioContentSettings가 소프트 참조로 고른다.
// ThirdParty → Portfolio 참조(표의 키)는 허용된다. 반대 방향(Portfolio → ThirdParty)만 금지 (에셋 규칙 1)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatContentSet.generated.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMesh;
class UStaticMesh;

UCLASS(BlueprintType, Const)
class FIRSTACTION_API UCombatContentSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 몽타주: 공격(공격 데이터), 반응·회피·가드·사망(캐릭터 데이터). 키 = 데이터가 가리키는 기본 몽타주
	// 배포 몽타주에도 같은 이름의 노티파이 구간을 찍는다 (PLAN 5.2 규칙 3, 검증이 비교한다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TMap<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimMontage>> Montages;

	// 애니메이션 블루프린트 (캐릭터 메시 컴포넌트의 Anim Class)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TMap<TSubclassOf<UAnimInstance>, TSubclassOf<UAnimInstance>> AnimClasses;

	// 캐릭터 몸 메시. 소켓 이름(HandGrip_R 등)은 같아야 한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TMap<TObjectPtr<USkeletalMesh>, TObjectPtr<USkeletalMesh>> SkeletalMeshes;

	// 무기 메시. 판정 소켓 이름(blade_start, blade_end)은 같아야 한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UStaticMesh>> StaticMeshes;

#if WITH_EDITOR
	// 데이터 검증 (9-1 질문 3 C)
	// 오류: 짝의 한쪽이 비어 있음, 기본 몽타주에 있는 태그 구간·판정 구간·워핑 목표·몽타주 구간 이름이 배포 몽타주에 없음,
	//       기본 메시의 소켓이 배포 메시에 없음
	// 경고: 같은 종류의 구간 개수가 다름, 멈출 간격(Attack Reach)이 없음, 스켈레톤이 다름,
	//       Portfolio의 몽타주 중 이 표에 짝이 없는 것 (질문 2 B: 실행 중에는 기본 몽타주로 재생된다)
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
