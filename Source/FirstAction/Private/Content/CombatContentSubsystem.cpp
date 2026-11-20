#include "Content/CombatContentSubsystem.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Content/CombatContentSet.h"
#include "Content/PortfolioContentSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FirstAction.h"

void UCombatContentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UPortfolioContentSettings* Settings = GetDefault<UPortfolioContentSettings>();
	if (!Settings->bUseReleaseContentSet)
	{
		UE_LOG(LogFirstAction, Log, TEXT("콘텐츠 세트: Default (설정에서 Release 끔)"));
		return;
	}
	if (Settings->ReleaseContentSet.IsNull())
	{
		UE_LOG(LogFirstAction, Log, TEXT("콘텐츠 세트: Default (Release 세트 지정 없음)"));
		return;
	}

	// 게임 시작 때 한 번만 로드한다. 세트가 가리키는 배포 에셋도 함께 로드된다
	ActiveSet = Settings->ReleaseContentSet.LoadSynchronous();
	if (!ActiveSet)
	{
		// 폴백 (PLAN 5.2 규칙 5): ThirdParty가 없는 클론, 지운 파일, 잘못된 경로
		UE_LOG(LogFirstAction, Warning, TEXT("콘텐츠 세트: Release 세트 %s를 불러오지 못해 Default로 폴백"),
			*Settings->ReleaseContentSet.ToString());
		return;
	}

	UE_LOG(LogFirstAction, Log, TEXT("콘텐츠 세트: Release %s (몽타주 %d, 애님 BP %d, 스켈레탈 메시 %d, 스태틱 메시 %d)"),
		*ActiveSet->GetName(), ActiveSet->Montages.Num(), ActiveSet->AnimClasses.Num(),
		ActiveSet->SkeletalMeshes.Num(), ActiveSet->StaticMeshes.Num());
}

UCombatContentSubsystem* UCombatContentSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UCombatContentSubsystem>() : nullptr;
}

void UCombatContentSubsystem::WarnMissing(const UObject* DefaultAsset, const TCHAR* Kind)
{
	if (!ActiveSet || !DefaultAsset)
	{
		return;
	}
	// 배포 전용 에셋(배포 BP의 Kwang 데이터가 직접 가리키는 몽타주, Greystone 메시 등)은 치환 대상이 아니다 (9-3)
	// 경고는 깃에 있는 기본 에셋이 표에서 빠졌을 때만 남긴다
	if (DefaultAsset->GetPathName().StartsWith(TEXT("/Game/ThirdParty/")))
	{
		return;
	}
	bool bAlreadyWarned = false;
	WarnedAssets.Add(FObjectKey(DefaultAsset), &bAlreadyWarned);
	if (!bAlreadyWarned)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("콘텐츠 세트 %s: %s %s의 Release 치환 없음, 기본 사용"),
			*ActiveSet->GetName(), Kind, *DefaultAsset->GetName());
	}
}

UAnimMontage* UCombatContentSubsystem::ResolveMontage(const UObject* WorldContext, UAnimMontage* Montage)
{
	UCombatContentSubsystem* Content = Get(WorldContext);
	if (!Content || !Content->ActiveSet || !Montage)
	{
		return Montage;
	}
	if (const TObjectPtr<UAnimMontage>* Found = Content->ActiveSet->Montages.Find(Montage))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	Content->WarnMissing(Montage, TEXT("몽타주"));
	return Montage;
}

USkeletalMesh* UCombatContentSubsystem::ResolveSkeletalMesh(const UObject* WorldContext, USkeletalMesh* Mesh)
{
	UCombatContentSubsystem* Content = Get(WorldContext);
	if (!Content || !Content->ActiveSet || !Mesh)
	{
		return Mesh;
	}
	if (const TObjectPtr<USkeletalMesh>* Found = Content->ActiveSet->SkeletalMeshes.Find(Mesh))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	Content->WarnMissing(Mesh, TEXT("스켈레탈 메시"));
	return Mesh;
}

UStaticMesh* UCombatContentSubsystem::ResolveStaticMesh(const UObject* WorldContext, UStaticMesh* Mesh)
{
	UCombatContentSubsystem* Content = Get(WorldContext);
	if (!Content || !Content->ActiveSet || !Mesh)
	{
		return Mesh;
	}
	if (const TObjectPtr<UStaticMesh>* Found = Content->ActiveSet->StaticMeshes.Find(Mesh))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	Content->WarnMissing(Mesh, TEXT("스태틱 메시"));
	return Mesh;
}

TSubclassOf<UAnimInstance> UCombatContentSubsystem::ResolveAnimClass(const UObject* WorldContext, TSubclassOf<UAnimInstance> AnimClass)
{
	UCombatContentSubsystem* Content = Get(WorldContext);
	if (!Content || !Content->ActiveSet || !AnimClass)
	{
		return AnimClass;
	}
	if (const TSubclassOf<UAnimInstance>* Found = Content->ActiveSet->AnimClasses.Find(AnimClass))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	Content->WarnMissing(AnimClass.Get(), TEXT("애님 BP"));
	return AnimClass;
}
