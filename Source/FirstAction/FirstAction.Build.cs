// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FirstAction : ModuleRules
{
	public FirstAction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			// GAS (Step 1-1): 공개 헤더(CombatCharacterBase.h 등)에서 GAS 타입을 쓰므로 Public
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			// 모션 워핑 (Step 6-2): 기반 캐릭터가 UMotionWarpingComponent를 가진다
			"MotionWarping",
			// 프로젝트 설정 (Step 6-4): UCombatSettings가 UDeveloperSettings를 상속한다
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// 에디터 도구 (9-3): 래그돌 물리 에셋 생성(FirstActionEditorLibrary). 게임 빌드에는 넣지 않는다
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "PhysicsUtilities", "MeshUtilitiesEngine", "MeshUtilitiesCommon", "AssetRegistry" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"FirstAction",
			"FirstAction/Variant_Combat",
			"FirstAction/Variant_Combat/AI",
			"FirstAction/Variant_Combat/Animation",
			"FirstAction/Variant_Combat/Gameplay",
			"FirstAction/Variant_Combat/Interfaces",
			"FirstAction/Variant_Combat/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
