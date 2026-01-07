// // Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "UEd/K2Node_SpawnActorFromPool.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "KismetCompilerMisc.h"
#include "FunctionLibrary\LazyDynamicObjectPoolUnCookOnlyLibrary.h"
#include "FunctionLibrary/LazyDynamicObjectPoolLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"

struct FK2Nod_SpawnActorFromPoolHelper
{
    //static pins
    static const FName ActorClassPinName;
    static const FName ActorClassTransformPinName;
    static const FName OwnerPinName;
};

const FName FK2Nod_SpawnActorFromPoolHelper::ActorClassPinName(TEXT("ActorClass"));
const FName FK2Nod_SpawnActorFromPoolHelper::ActorClassTransformPinName(TEXT("SpawnTransform"));
const FName FK2Nod_SpawnActorFromPoolHelper::OwnerPinName(TEXT("Owner"));

#define LOCTEXT_NAMESPACE "K2Node_GetActorFromPool"

void UK2Node_SpawnActorFromPool::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();

    // Create the transform pin
    UScriptStruct* TransformStruct = TBaseStructure<FTransform>::Get();
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, TransformStruct, FK2Nod_SpawnActorFromPoolHelper::ActorClassTransformPinName);

    UEdGraphPin* OwnerPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, AActor::StaticClass(), FK2Nod_SpawnActorFromPoolHelper::OwnerPinName);
    OwnerPin->bAdvancedView = true;
    if (ENodeAdvancedPins::NoPins == AdvancedPinDisplay)
    {
        AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
    }
}

void UK2Node_SpawnActorFromPool::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    // Create the "Get Subsystem" node
    UEdGraphPin* WorldContextPin = nullptr;
    const UK2Node_CallFunction* GetSubsystemNode = CreateGetSubsystemNode(CompilerContext, SourceGraph, WorldContextPin);
    if(!IsValid(GetSubsystemNode)) return;
    UEdGraphPin* SubsystemInstancePin = GetSubsystemNode->GetReturnValuePin();

    // Connect the WorldContext pin
    if (UEdGraphPin* MyWorldContextPin = GetWorldContextPin())
    {
        CompilerContext.MovePinLinksToIntermediate(*MyWorldContextPin, *WorldContextPin);
    }

    UK2Node_SpawnActorFromPool* SpawnPoolNode = this;
    UEdGraphPin* ExecPin = SpawnPoolNode->GetExecPin();
    UEdGraphPin* ThenPin = SpawnPoolNode->GetThenPin();
    UEdGraphPin* ClassPin = SpawnPoolNode->GetClassPin();
    UEdGraphPin* SpawnWorldContextPin = SpawnPoolNode->GetWorldContextPin();
    UEdGraphPin* TransformPin = SpawnPoolNode->GetActorTransformPin();
    UEdGraphPin* OwnerPin = SpawnPoolNode->GetOwnerPin();
    UEdGraphPin* ResultPin = SpawnPoolNode->GetResultPin();

    // Cache the class to spawn. Note, this is the compile time class that the pin was set to or the variable type it was connected to. Runtime it could be a child.
    UClass* ClassToSpawn = GetClassToSpawn();

    UClass* SpawnClass = (ClassPin != nullptr) ? Cast<UClass>(ClassPin->DefaultObject) : nullptr;
    if ( !ClassPin || ((0 == ClassPin->LinkedTo.Num()) && (nullptr == SpawnClass)))
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("SpawnActorFromPoolMissingClass_Error", "Spawn Actor From Pool node @@ must have a @@ specified.").ToString(), SpawnPoolNode, ClassPin);
        // we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
        SpawnPoolNode->BreakAllNodeLinks();
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    // create 'begin spawn' call node
    UK2Node_CallFunction* GetActorFromPoolFunc = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnPoolNode, SourceGraph);
    GetActorFromPoolFunc->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(ULazyDynamicObjectPoolSubsystem, InitializeActorFromPool), ULazyDynamicObjectPoolSubsystem::StaticClass());
    GetActorFromPoolFunc->AllocateDefaultPins();

    UEdGraphPin* CallActorFromPoolExec = GetActorFromPoolFunc->GetExecPin();
    UEdGraphPin* CallActorFromPoolActorClassPin = GetActorFromPoolFunc->FindPinChecked(FName("ActorClass"));
    UEdGraphPin* CallActorFromPoolOwnerPin = GetActorFromPoolFunc->FindPinChecked(FName("NewOwner"));
    UEdGraphPin* CallActorFromPoolResult = GetActorFromPoolFunc->GetReturnValuePin();	
    
    // Move 'exec' connection from spawn node to 'initialise actor'
    CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallActorFromPoolExec);

    // Connect the GetSubsystem node to...
    UEdGraphPin* CallActorFromPoolTargetPin = GetActorFromPoolFunc->FindPinChecked(UEdGraphSchema_K2::PN_Self);
    CallActorFromPoolTargetPin->MakeLinkTo( SubsystemInstancePin );

    if(ClassPin->LinkedTo.Num() > 0)
    {
        // Copy the 'blueprint' connection from the spawn node to 'begin spawn'
        CompilerContext.MovePinLinksToIntermediate(*ClassPin, *CallActorFromPoolActorClassPin);
    }
    else
    {
        // Copy blueprint literal onto begin spawn call 
        CallActorFromPoolActorClassPin->DefaultObject = SpawnClass;
    }

    if (OwnerPin != nullptr)
    {
        CompilerContext.MovePinLinksToIntermediate(*OwnerPin, *CallActorFromPoolOwnerPin);
    }

    //////////////////////////////////////////////////////////////////////////
    // create 'Finish Initialize Actor' call node
    UK2Node_CallFunction* CallFinishInitializeActorNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnPoolNode, SourceGraph);
    CallFinishInitializeActorNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(ULazyDynamicObjectPoolSubsystem, FinishInitializeActorFromPool), ULazyDynamicObjectPoolSubsystem::StaticClass());
    CallFinishInitializeActorNode->AllocateDefaultPins();

    UEdGraphPin* CallFinishActorFromPoolExec = CallFinishInitializeActorNode->GetExecPin();
    UEdGraphPin* CallFinishActorFromPoolThen = CallFinishInitializeActorNode->GetThenPin();
    UEdGraphPin* CallFinishActorFromPoolActor = CallFinishInitializeActorNode->FindPinChecked(FName("Actor"));
    UEdGraphPin* CallFinishActorFromPoolTransform = CallFinishInitializeActorNode->FindPinChecked(FName("NewTransform"));
    UEdGraphPin* CallFinishActorFromPoolResult = CallFinishInitializeActorNode->GetReturnValuePin();
    // add more for bsweep,OutSweepHitResult and teleport.

    UEdGraphPin* CallFinishActorFromPoolTargetPin = CallFinishInitializeActorNode->FindPinChecked(UEdGraphSchema_K2::PN_Self);
    CallFinishActorFromPoolTargetPin->MakeLinkTo( SubsystemInstancePin );

    // Move 'then' connection from init node to 'finish init'
    CompilerContext.MovePinLinksToIntermediate(*ThenPin, *CallFinishActorFromPoolThen);

    // Copy transform connection
    CompilerContext.CopyPinLinksToIntermediate(*TransformPin, *CallFinishActorFromPoolTransform);

    // Connect output actor from 'init' to 'finish init'
    CallActorFromPoolResult->MakeLinkTo(CallFinishActorFromPoolActor);

    // Move result connection from spawn node to 'finish spawn'
    CallFinishActorFromPoolResult->PinType = ResultPin->PinType; // Copy type so it uses the right actor subclass
    CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CallFinishActorFromPoolResult);

    // Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
    /*UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, GetActorFromPoolFunc, SpawnPoolNode, CallActorFromPoolResult, ClassToSpawn );*/
    UEdGraphPin* LastThen = ULazyDynamicObjectPoolUnCookOnlyLibrary::GenerateAssignmentNodesForPoolActor(CompilerContext, SourceGraph, GetActorFromPoolFunc, SpawnPoolNode, CallActorFromPoolResult, ClassToSpawn);

    // Make exec connection between 'then' on last node and 'finish'
    LastThen->MakeLinkTo(CallFinishActorFromPoolExec);

    // Break any links to the expanded node
    SpawnPoolNode->BreakAllNodeLinks();
}

FText UK2Node_SpawnActorFromPool::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    FText NodeTitle = NSLOCTEXT("K2Node", "SpawnActor_BaseTitle", "Spawn Actor from Pool");
    if (TitleType != ENodeTitleType::MenuTitle)
    {
        if (UEdGraphPin* ClassPin = GetClassPin())
        {
            if (ClassPin->LinkedTo.Num() > 0)
            {
                // Blueprint will be determined dynamically, so we don't have the name in this case
                NodeTitle = NSLOCTEXT("K2Node", "SpawnActor_Title_Unknown", "SpawnActor");
            }
            else if (ClassPin->DefaultObject == nullptr)
            {
                NodeTitle = NSLOCTEXT("K2Node", "SpawnActor_Title_NONE", "SpawnActor NONE From Pool");
            }
            else
            {
                if (CachedNodeTitle.IsOutOfDate(this))
                {
                    FText ClassName;
                    if (const UClass* PickedClass = Cast<UClass>(ClassPin->DefaultObject))
                    {
                        ClassName = PickedClass->GetDisplayNameText();
                    }

                    FFormatNamedArguments Args;
                    Args.Add(TEXT("ClassName"), ClassName);

                    // FText::Format() is slow, so we cache this to save on performance
                    CachedNodeTitle.SetCachedText(FText::Format(NSLOCTEXT("K2Node", "SpawnActor_Title_Class", "SpawnActor {ClassName} From Pool"), Args), this);
                }
                NodeTitle = CachedNodeTitle;
            } 
        }
        else
        {
            NodeTitle = NSLOCTEXT("K2Node", "SpawnActor_Title_NONE", "SpawnActor NONE");
        }
    }
    return NodeTitle;
}

FText UK2Node_SpawnActorFromPool::GetTooltipText() const
{
    return LOCTEXT("GetActorFromPool_Tooltip", "Spawns an actor from the object pool or creates a new one if the pool is empty.\n\nWARNING: This node returns a raw actor pointer. Consider using 'Spawn Pooled Actor (Safe)' instead, which returns a safe handle that prevents accidental use of actors returned to the pool.");
}

FSlateIcon UK2Node_SpawnActorFromPool::GetIconAndTint(FLinearColor& OutColor) const
{
    static FSlateIcon Icon("EditorStyle", "ClassIcon.Actor");
    return Icon;
}

bool UK2Node_SpawnActorFromPool::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
    const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    return Super::IsCompatibleWithGraph(Graph) && (!Blueprint || (FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != Graph && Blueprint->GeneratedClass->GetDefaultObject()->ImplementsGetWorld()));
}

void UK2Node_SpawnActorFromPool::GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const
{
    const UClass* ClassToSpawn = GetClassToSpawn();
    const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT( "InvalidClass" );
    OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Type" ), TEXT( "SpawnActorFromClass" ) ));
    OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Class" ), GetClass()->GetName() ));
    OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Name" ), GetName() ));
    OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "ActorClass" ), ClassToSpawnStr ));
}

FNodeHandlingFunctor* UK2Node_SpawnActorFromPool::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
    return new FNodeHandlingFunctor(CompilerContext);
}

void UK2Node_SpawnActorFromPool::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    const UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

FText UK2Node_SpawnActorFromPool::GetMenuCategory() const
{
    return LOCTEXT("GetActorFromPool_MenuCategory", "Object Pool");
}

UClass* UK2Node_SpawnActorFromPool::GetClassPinBaseClass() const
{
    return AActor::StaticClass();
}

UEdGraphPin* UK2Node_SpawnActorFromPool::GetActorTransformPin() const
{
    UEdGraphPin* Pin = FindPinChecked(FK2Nod_SpawnActorFromPoolHelper::ActorClassTransformPinName);
    check(Pin->Direction == EGPD_Input);
    return Pin;
}

UEdGraphPin* UK2Node_SpawnActorFromPool::GetOwnerPin() const
{
    UEdGraphPin* Pin = FindPin(FK2Nod_SpawnActorFromPoolHelper::OwnerPinName);
    check(Pin == nullptr || Pin->Direction == EGPD_Input);
    return Pin;
}

bool UK2Node_SpawnActorFromPool::IsSpawnVarPin(UEdGraphPin* Pin) const
{
    const UEdGraphPin* ParentPin = Pin->ParentPin;
    while (ParentPin)
    {
        if (ParentPin->PinName == FK2Nod_SpawnActorFromPoolHelper::ActorClassTransformPinName)
        {
            return false;
        }
        ParentPin = ParentPin->ParentPin;
    }

    return(	Super::IsSpawnVarPin(Pin) &&
            Pin->PinName != FK2Nod_SpawnActorFromPoolHelper::ActorClassTransformPinName && 
            Pin->PinName != FK2Nod_SpawnActorFromPoolHelper::OwnerPinName );
}

UK2Node_CallFunction* UK2Node_SpawnActorFromPool::CreateGetSubsystemNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin*& OutWorldContextPin)
{
    UK2Node_CallFunction* GetSubsystemNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    GetSubsystemNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(ULazyDynamicObjectPoolLibrary, GetSubsystem), ULazyDynamicObjectPoolLibrary::StaticClass());
    GetSubsystemNode->AllocateDefaultPins();
    
    return GetSubsystemNode;
}



#undef LOCTEXT_NAMESPACE
