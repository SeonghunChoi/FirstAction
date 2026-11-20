// 어느 콘텐츠 세트를 쓸지 고르는 프로젝트 설정 (Step 9-1, PLAN 5.2 규칙 5)
// Project Settings에 "Portfolio Content" 항목으로 나타나고 Config/DefaultGame.ini에 저장된다.
// Release 세트는 소프트 참조(경로)라, ThirdParty가 없는 클론에서도 이 설정 때문에 열기가 깨지지 않는다.
// 로드에 실패하거나 끄면 Default(치환 없음)로 폴백한다. 실제 로드와 치환은 UCombatContentSubsystem

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PortfolioContentSettings.generated.h"

class UCombatContentSet;

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Portfolio Content"))
class FIRSTACTION_API UPortfolioContentSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 배포 빌드용 콘텐츠 세트 (Content/ThirdParty/). 비어 있거나 파일이 없으면 Default
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Content Set")
	TSoftObjectPtr<UCombatContentSet> ReleaseContentSet;

	// 끄면 Release 세트가 있어도 Default로 실행한다 (ThirdParty가 있는 PC에서 깃 버전을 확인할 때)
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Content Set")
	bool bUseReleaseContentSet = true;
};
