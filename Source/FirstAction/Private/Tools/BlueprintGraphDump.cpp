// 블루프린트(애님 BP 포함)의 그래프를 텍스트로 덤프하는 에디터 전용 콘솔 명령 (9-3, 배포판 작업 도구)
//
//   FirstAction.DumpBlueprintGraph /Game/ThirdParty/Release/Greystone/ABP_Greystone
//
// 결과는 Saved/GraphDumps/<에셋 이름>.txt 에 쓴다. 내용:
//   - 부모 클래스, 블루프린트 변수(이름과 타입)
//   - 모든 그래프(이벤트 그래프, 함수, 매크로, AnimGraph, 상태 머신·상태·전이 규칙 같은 하위 그래프)
//   - 노드마다 제목, 클래스, 편집 가능한 속성 값(애님 노드의 슬롯 이름 등), 핀의 기본값과 연결
//
// 그래프 노드 목록(UEdGraph::Nodes)은 Python에 노출되지 않아 C++로 읽는다
//
//   FirstAction.RemoveBlueprintNodes /Game/경로/에셋 K2Node_DynamicCast_0 K2Node_CallFunction_15 ...
//
// 덤프에 나온 노드 이름으로 노드를 지운다(연결도 끊는다). 컴파일과 저장은 Python에서
// (unreal.BlueprintEditorLibrary.compile_blueprint, unreal.EditorAssetLibrary.save_asset)

#include "CoreMinimal.h"

#if WITH_EDITOR

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "FirstAction.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

namespace BlueprintGraphDump
{
	// 속성 값 한 줄이 너무 길면 자른다 (애님 노드 구조체 전체가 나오는 경우)
	constexpr int32 MaxValueLength = 600;

	FString OneLine(const FString& In)
	{
		return In.Replace(TEXT("\r"), TEXT("")).Replace(TEXT("\n"), TEXT(" / "));
	}

	FString PinTypeText(const FEdGraphPinType& Type)
	{
		FString Text = Type.PinCategory.ToString();
		if (const UObject* Sub = Type.PinSubCategoryObject.Get())
		{
			Text += TEXT(":") + Sub->GetName();
		}
		else if (!Type.PinSubCategory.IsNone())
		{
			Text += TEXT(":") + Type.PinSubCategory.ToString();
		}
		if (Type.ContainerType == EPinContainerType::Array)
		{
			Text = TEXT("Array<") + Text + TEXT(">");
		}
		return Text;
	}

	FString NodeLabel(const UEdGraphNode* Node)
	{
		return FString::Printf(TEXT("%s \"%s\""), *Node->GetName(), *OneLine(Node->GetNodeTitle(ENodeTitleType::ListView).ToString()));
	}

	// 노드 클래스 자신이 선언한 편집 가능한 속성 (UEdGraphNode 공통 속성은 뺀다)
	void DumpNodeProperties(const UEdGraphNode* Node, const FString& Indent, FString& Out)
	{
		for (TFieldIterator<FProperty> It(Node->GetClass(), EFieldIterationFlags::IncludeSuper); It; ++It)
		{
			const FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Edit) || Property->GetOwnerClass() == UEdGraphNode::StaticClass())
			{
				continue;
			}
			FString Value;
			Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Node), nullptr, nullptr, PPF_None);
			if (Value.IsEmpty() || Value == TEXT("None") || Value == TEXT("()"))
			{
				continue;
			}
			Value = OneLine(Value);
			if (Value.Len() > MaxValueLength)
			{
				Value = Value.Left(MaxValueLength) + TEXT(" ...");
			}
			Out += FString::Printf(TEXT("%s  . %s = %s\n"), *Indent, *Property->GetName(), *Value);
		}
	}

	void DumpPins(const UEdGraphNode* Node, const FString& Indent, FString& Out)
	{
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin)
			{
				continue;
			}
			const bool bLinked = Pin->LinkedTo.Num() > 0;
			const bool bHasDefault = !Pin->DefaultValue.IsEmpty() || Pin->DefaultObject || !Pin->DefaultTextValue.IsEmpty();
			if (!bLinked && (!bHasDefault || Pin->bHidden))
			{
				continue;
			}

			FString Line = FString::Printf(TEXT("%s  %s %s [%s]"), *Indent,
				Pin->Direction == EGPD_Input ? TEXT("<-") : TEXT("->"),
				*Pin->PinName.ToString(), *PinTypeText(Pin->PinType));
			if (!bLinked && bHasDefault)
			{
				const FString Default = Pin->DefaultObject ? Pin->DefaultObject->GetPathName()
					: !Pin->DefaultValue.IsEmpty() ? Pin->DefaultValue : Pin->DefaultTextValue.ToString();
				Line += TEXT(" = ") + OneLine(Default);
			}
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				if (Linked && Linked->GetOwningNodeUnchecked())
				{
					Line += FString::Printf(TEXT(" | %s.%s"), *Linked->GetOwningNodeUnchecked()->GetName(), *Linked->PinName.ToString());
				}
			}
			Out += Line + TEXT("\n");
		}
	}

	void DumpGraph(const UEdGraph* Graph, int32 Depth, FString& Out, int32& NodeCount)
	{
		if (!Graph)
		{
			return;
		}
		const FString Indent = FString::ChrN(Depth * 2, TEXT(' '));
		Out += FString::Printf(TEXT("%s== 그래프 %s (%s), 노드 %d\n"), *Indent, *Graph->GetName(), *Graph->GetClass()->GetName(), Graph->Nodes.Num());

		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}
			++NodeCount;
			Out += FString::Printf(TEXT("%s[%s] (%s)\n"), *Indent, *NodeLabel(Node), *Node->GetClass()->GetName());
			if (!Node->NodeComment.IsEmpty())
			{
				Out += FString::Printf(TEXT("%s  # %s\n"), *Indent, *OneLine(Node->NodeComment));
			}
			DumpNodeProperties(Node, Indent, Out);
			DumpPins(Node, Indent, Out);
		}

		// 상태 머신, 상태, 전이 규칙, 접힌 그래프 등
		for (const UEdGraph* SubGraph : Graph->SubGraphs)
		{
			DumpGraph(SubGraph, Depth + 1, Out, NodeCount);
		}
	}

	// "/Game/A/B" 로 받으면 "/Game/A/B.B" 로 바꿔 찾는다
	UBlueprint* LoadBlueprint(const FString& AssetPath)
	{
		FString ObjectPath = AssetPath;
		if (!ObjectPath.Contains(TEXT(".")))
		{
			ObjectPath += TEXT(".") + FPackageName::GetShortName(ObjectPath);
		}
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath);
		if (!Blueprint)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("블루프린트 %s를 찾지 못했습니다"), *ObjectPath);
		}
		return Blueprint;
	}

	void Run(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("사용법: FirstAction.DumpBlueprintGraph /Game/경로/에셋이름"));
			return;
		}

		const UBlueprint* Blueprint = LoadBlueprint(Args[0]);
		if (!Blueprint)
		{
			return;
		}

		FString Out;
		Out += FString::Printf(TEXT("블루프린트 %s (%s)\n"), *Blueprint->GetPathName(), *Blueprint->GetClass()->GetName());
		Out += FString::Printf(TEXT("부모 클래스 %s\n"), *GetNameSafe(Blueprint->ParentClass));

		Out += FString::Printf(TEXT("\n== 변수 %d\n"), Blueprint->NewVariables.Num());
		for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
		{
			Out += FString::Printf(TEXT("  %s [%s]%s\n"), *Variable.VarName.ToString(), *PinTypeText(Variable.VarType),
				Variable.DefaultValue.IsEmpty() ? TEXT("") : *(TEXT(" = ") + OneLine(Variable.DefaultValue)));
		}
		Out += TEXT("\n");

		int32 NodeCount = 0;
		for (const TArray<TObjectPtr<UEdGraph>>* Graphs : { &Blueprint->UbergraphPages, &Blueprint->FunctionGraphs, &Blueprint->MacroGraphs })
		{
			for (const UEdGraph* Graph : *Graphs)
			{
				DumpGraph(Graph, 0, Out, NodeCount);
				Out += TEXT("\n");
			}
		}

		const FString FilePath = FPaths::ProjectSavedDir() / TEXT("GraphDumps") / (Blueprint->GetName() + TEXT(".txt"));
		if (FFileHelper::SaveStringToFile(Out, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			UE_LOG(LogFirstAction, Display, TEXT("그래프 덤프: %s → %s (변수 %d, 노드 %d)"),
				*Blueprint->GetName(), *FPaths::ConvertRelativePathToFull(FilePath), Blueprint->NewVariables.Num(), NodeCount);
		}
		else
		{
			UE_LOG(LogFirstAction, Warning, TEXT("그래프 덤프: %s 파일을 쓰지 못했습니다"), *FilePath);
		}
	}

	void RemoveNodes(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("사용법: FirstAction.RemoveBlueprintNodes /Game/경로/에셋이름 노드이름 [노드이름 ...]"));
			return;
		}

		UBlueprint* Blueprint = LoadBlueprint(Args[0]);
		if (!Blueprint)
		{
			return;
		}

		TSet<FString> Pending;
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			Pending.Add(Args[Index]);
		}

		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		Blueprint->Modify();

		int32 Removed = 0;
		for (UEdGraph* Graph : Graphs)
		{
			// 지우는 동안 배열이 바뀌므로 복사본으로 돈다
			const TArray<TObjectPtr<UEdGraphNode>> Nodes = Graph->Nodes;
			for (UEdGraphNode* Node : Nodes)
			{
				if (Node && Pending.Remove(Node->GetName()) > 0)
				{
					UE_LOG(LogFirstAction, Display, TEXT("노드 삭제: %s / %s"), *Graph->GetName(), *NodeLabel(Node));
					Graph->Modify();
					Node->Modify();
					Graph->RemoveNode(Node, true);
					++Removed;
				}
			}
		}

		for (const FString& Missing : Pending)
		{
			UE_LOG(LogFirstAction, Warning, TEXT("노드 삭제: %s를 찾지 못했습니다"), *Missing);
		}
		Blueprint->MarkPackageDirty();
		UE_LOG(LogFirstAction, Display, TEXT("노드 삭제: %s에서 %d개 (컴파일·저장은 따로)"), *Blueprint->GetName(), Removed);
	}

	FAutoConsoleCommand RemoveCommand(
		TEXT("FirstAction.RemoveBlueprintNodes"),
		TEXT("블루프린트 그래프에서 이름으로 노드를 지운다. 인자: 에셋 경로, 노드 이름들"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&RemoveNodes));

	FAutoConsoleCommand Command(
		TEXT("FirstAction.DumpBlueprintGraph"),
		TEXT("블루프린트 그래프를 Saved/GraphDumps/<이름>.txt 로 덤프한다. 인자: 에셋 경로"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Run));
}

#endif
