// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "UEd/K2Node_GetPooledActorHandle.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Core/PooledActorHandle.h"
#include "EdGraphSchema_K2.h"
#include "FunctionLibrary/LazyDynamicObjectPoolLibrary.h"
#include "FunctionLibrary/LazyDynamicObjectPoolUnCookOnlyLibrary.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "KismetCompilerMisc.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"


struct FK2Node_GetPooledActorHandleHelper {
  // Static pins
  static const FName ActorClassPinName;
  static const FName ActorClassTransformPinName;
  static const FName OwnerPinName;
};

const FName
    FK2Node_GetPooledActorHandleHelper::ActorClassPinName(TEXT("ActorClass"));
const FName FK2Node_GetPooledActorHandleHelper::ActorClassTransformPinName(
    TEXT("SpawnTransform"));
const FName FK2Node_GetPooledActorHandleHelper::OwnerPinName(TEXT("Owner"));

#define LOCTEXT_NAMESPACE "K2Node_GetPooledActorHandle"

void UK2Node_GetPooledActorHandle::AllocateDefaultPins() {
  Super::AllocateDefaultPins();

  // Create the transform pin
  UScriptStruct *TransformStruct = TBaseStructure<FTransform>::Get();
  CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, TransformStruct,
            FK2Node_GetPooledActorHandleHelper::ActorClassTransformPinName);

  // Create the owner pin (advanced)
  UEdGraphPin *OwnerPin =
      CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, AActor::StaticClass(),
                FK2Node_GetPooledActorHandleHelper::OwnerPinName);
  OwnerPin->bAdvancedView = true;
  if (ENodeAdvancedPins::NoPins == AdvancedPinDisplay) {
    AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
  }

  // Override the result pin type to be FPooledActorHandle struct instead of
  // Actor pointer The super class creates it as an actor pointer, but we need
  // it to be a struct
  if (UEdGraphPin *ResultPin = GetResultPin()) {
    ResultPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    ResultPin->PinType.PinSubCategoryObject =
        FPooledActorHandle::StaticStruct();
    ResultPin->PinType.PinSubCategory = NAME_None;
  }
}

void UK2Node_GetPooledActorHandle::ExpandNode(
    FKismetCompilerContext &CompilerContext, UEdGraph *SourceGraph) {
  Super::ExpandNode(CompilerContext, SourceGraph);

  // Create the "Get Subsystem" node
  UEdGraphPin *WorldContextPin = nullptr;
  const UK2Node_CallFunction *GetSubsystemNode =
      CreateGetSubsystemNode(CompilerContext, SourceGraph, WorldContextPin);
  if (!IsValid(GetSubsystemNode))
    return;
  UEdGraphPin *SubsystemInstancePin = GetSubsystemNode->GetReturnValuePin();

  // Connect the WorldContext pin
  if (UEdGraphPin *MyWorldContextPin = GetWorldContextPin()) {
    CompilerContext.MovePinLinksToIntermediate(*MyWorldContextPin,
                                               *WorldContextPin);
  }

  UK2Node_GetPooledActorHandle *SpawnPoolNode = this;
  UEdGraphPin *ExecPin = SpawnPoolNode->GetExecPin();
  UEdGraphPin *ThenPin = SpawnPoolNode->GetThenPin();
  UEdGraphPin *ClassPin = SpawnPoolNode->GetClassPin();
  UEdGraphPin *SpawnWorldContextPin = SpawnPoolNode->GetWorldContextPin();
  UEdGraphPin *TransformPin = SpawnPoolNode->GetActorTransformPin();
  UEdGraphPin *OwnerPin = SpawnPoolNode->GetOwnerPin();
  UEdGraphPin *ResultPin = SpawnPoolNode->GetResultPin();

  // Cache the class to spawn
  UClass *ClassToSpawn = GetClassToSpawn();

  UClass *SpawnClass =
      (ClassPin != nullptr) ? Cast<UClass>(ClassPin->DefaultObject) : nullptr;
  if (!ClassPin ||
      ((0 == ClassPin->LinkedTo.Num()) && (nullptr == SpawnClass))) {
    CompilerContext.MessageLog.Error(
        *LOCTEXT("GetPooledActorHandleMissingClass_Error",
                 "Get Pooled Actor Handle node @@ must have a @@ specified.")
             .ToString(),
        SpawnPoolNode, ClassPin);
    SpawnPoolNode->BreakAllNodeLinks();
    return;
  }

  //////////////////////////////////////////////////////////////////////////
  // Step 1: Create 'InitializeActorFromPool' call node (same as unsafe node)
  // This retrieves an actor from the pool WITHOUT adding it to InUseObjects yet
  UK2Node_CallFunction *GetActorFromPoolFunc =
      CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnPoolNode,
                                                                  SourceGraph);
  GetActorFromPoolFunc->FunctionReference.SetExternalMember(
      GET_FUNCTION_NAME_CHECKED(ULazyDynamicObjectPoolSubsystem,
                                InitializeActorFromPool),
      ULazyDynamicObjectPoolSubsystem::StaticClass());
  GetActorFromPoolFunc->AllocateDefaultPins();

  UEdGraphPin *CallActorFromPoolExec = GetActorFromPoolFunc->GetExecPin();
  UEdGraphPin *CallActorFromPoolActorClassPin =
      GetActorFromPoolFunc->FindPinChecked(FName("ActorClass"));
  UEdGraphPin *CallActorFromPoolOwnerPin =
      GetActorFromPoolFunc->FindPinChecked(FName("NewOwner"));
  UEdGraphPin *CallActorFromPoolResult =
      GetActorFromPoolFunc->GetReturnValuePin();

  // Move 'exec' connection from spawn node to 'InitializeActorFromPool'
  CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallActorFromPoolExec);

  // Connect the GetSubsystem node
  UEdGraphPin *CallActorFromPoolTargetPin =
      GetActorFromPoolFunc->FindPinChecked(UEdGraphSchema_K2::PN_Self);
  CallActorFromPoolTargetPin->MakeLinkTo(SubsystemInstancePin);

  if (ClassPin->LinkedTo.Num() > 0) {
    // Copy the 'blueprint' connection from the spawn node
    CompilerContext.MovePinLinksToIntermediate(*ClassPin,
                                               *CallActorFromPoolActorClassPin);
  } else {
    // Copy blueprint literal
    CallActorFromPoolActorClassPin->DefaultObject = SpawnClass;
  }

  if (OwnerPin != nullptr) {
    CompilerContext.MovePinLinksToIntermediate(*OwnerPin,
                                               *CallActorFromPoolOwnerPin);
  }

  //////////////////////////////////////////////////////////////////////////
  // Step 2: Generate assignment nodes for exposed properties using the
  // retrieved actor
  UEdGraphPin *LastThen = ULazyDynamicObjectPoolUnCookOnlyLibrary::
      GenerateAssignmentNodesForPoolActor(
          CompilerContext, SourceGraph, GetActorFromPoolFunc, SpawnPoolNode,
          CallActorFromPoolResult, ClassToSpawn);

  //////////////////////////////////////////////////////////////////////////
  // Step 3: Create 'FinishInitializeActorFromPool' call node
  // This adds the actor to InUseObjects and activates it (same as unsafe node)
  UK2Node_CallFunction *CallFinishInitializeActorNode =
      CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnPoolNode,
                                                                  SourceGraph);
  CallFinishInitializeActorNode->FunctionReference.SetExternalMember(
      GET_FUNCTION_NAME_CHECKED(ULazyDynamicObjectPoolSubsystem,
                                FinishInitializeActorFromPool),
      ULazyDynamicObjectPoolSubsystem::StaticClass());
  CallFinishInitializeActorNode->AllocateDefaultPins();

  UEdGraphPin *CallFinishActorFromPoolExec =
      CallFinishInitializeActorNode->GetExecPin();
  UEdGraphPin *CallFinishActorFromPoolThen =
      CallFinishInitializeActorNode->GetThenPin();
  UEdGraphPin *CallFinishActorFromPoolActor =
      CallFinishInitializeActorNode->FindPinChecked(FName("Actor"));
  UEdGraphPin *CallFinishActorFromPoolTransform =
      CallFinishInitializeActorNode->FindPinChecked(FName("NewTransform"));
  UEdGraphPin *CallFinishActorFromPoolResult =
      CallFinishInitializeActorNode->GetReturnValuePin();

  UEdGraphPin *CallFinishActorFromPoolTargetPin =
      CallFinishInitializeActorNode->FindPinChecked(UEdGraphSchema_K2::PN_Self);
  CallFinishActorFromPoolTargetPin->MakeLinkTo(SubsystemInstancePin);

  // Make exec connection between assignment nodes and
  // 'FinishInitializeActorFromPool'
  LastThen->MakeLinkTo(CallFinishActorFromPoolExec);

  // Copy transform connection
  CompilerContext.CopyPinLinksToIntermediate(*TransformPin,
                                             *CallFinishActorFromPoolTransform);

  // Connect output actor from 'InitializeActorFromPool' to
  // 'FinishInitializeActorFromPool'
  CallActorFromPoolResult->MakeLinkTo(CallFinishActorFromPoolActor);

  //////////////////////////////////////////////////////////////////////////
  // Step 4: Create 'MakePooledActorHandleFromSubsystem' to wrap the result in a
  // safe handle
  UK2Node_CallFunction *MakeHandleFunc =
      CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnPoolNode,
                                                                  SourceGraph);
  MakeHandleFunc->FunctionReference.SetExternalMember(
      GET_FUNCTION_NAME_CHECKED(ULazyDynamicObjectPoolLibrary,
                                MakePooledActorHandleFromSubsystem),
      ULazyDynamicObjectPoolLibrary::StaticClass());
  MakeHandleFunc->AllocateDefaultPins();

  UEdGraphPin *MakeHandleActorPin =
      MakeHandleFunc->FindPinChecked(FName("Actor"));
  UEdGraphPin *MakeHandleSubsystemPin =
      MakeHandleFunc->FindPinChecked(FName("PoolSubsystem"));
  UEdGraphPin *MakeHandleResult = MakeHandleFunc->GetReturnValuePin();

  // Connect the finished actor to the handle maker
  CallFinishActorFromPoolResult->MakeLinkTo(MakeHandleActorPin);
  SubsystemInstancePin->MakeLinkTo(MakeHandleSubsystemPin);

  // Move 'then' connection from spawn node to 'FinishInitializeActorFromPool'
  CompilerContext.MovePinLinksToIntermediate(*ThenPin,
                                             *CallFinishActorFromPoolThen);

  // Move result connection - the result is now a FPooledActorHandle struct
  MakeHandleResult->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
  MakeHandleResult->PinType.PinSubCategoryObject =
      FPooledActorHandle::StaticStruct();

  // Now move the connections
  CompilerContext.MovePinLinksToIntermediate(*ResultPin, *MakeHandleResult);

  // Break any links to the expanded node
  SpawnPoolNode->BreakAllNodeLinks();
}

FText UK2Node_GetPooledActorHandle::GetNodeTitle(
    ENodeTitleType::Type TitleType) const {
  FText NodeTitle = NSLOCTEXT("K2Node", "SpawnActor_BaseTitle",
                              "Spawn Actor from Pool (Safe)");
  if (TitleType != ENodeTitleType::MenuTitle) {
    if (UEdGraphPin *ClassPin = GetClassPin()) {
      if (ClassPin->LinkedTo.Num() > 0) {
        // Blueprint will be determined dynamically, so we don't have the name
        // in this case
        NodeTitle =
            NSLOCTEXT("K2Node", "SpawnActor_Title_Unknown", "SpawnActor");
      } else if (ClassPin->DefaultObject == nullptr) {
        NodeTitle = NSLOCTEXT("K2Node", "SpawnActor_Title_NONE",
                              "SpawnActor NONE From Pool (Safe)");
      } else {
        if (CachedNodeTitle.IsOutOfDate(this)) {
          FText ClassName;
          if (const UClass *PickedClass =
                  Cast<UClass>(ClassPin->DefaultObject)) {
            ClassName = PickedClass->GetDisplayNameText();
          }

          FFormatNamedArguments Args;
          Args.Add(TEXT("ClassName"), ClassName);

          // FText::Format() is slow, so we cache this to save on performance
          CachedNodeTitle.SetCachedText(
              FText::Format(
                  NSLOCTEXT("K2Node", "SpawnActor_Title_Class",
                            "SpawnActor {ClassName} From Pool (Safe)"),
                  Args),
              this);
        }
        NodeTitle = CachedNodeTitle;
      }
    } else {
      NodeTitle =
          NSLOCTEXT("K2Node", "SpawnActor_Title_NONE", "SpawnActor NONE");
    }
  }
  return NodeTitle;
}

FText UK2Node_GetPooledActorHandle::GetTooltipText() const {
  return LOCTEXT(
      "GetPooledActorHandle_Tooltip",
      "Spawns an actor from the object pool with safe handle validation. The "
      "returned handle prevents accidental use of actors returned to the pool. "
      "This is the recommended way to spawn pooled actors.");
}

FSlateIcon
UK2Node_GetPooledActorHandle::GetIconAndTint(FLinearColor &OutColor) const {
  static FSlateIcon Icon("EditorStyle", "ClassIcon.Actor");
  return Icon;
}

bool UK2Node_GetPooledActorHandle::IsCompatibleWithGraph(
    UEdGraph const *Graph) const {
  const UBlueprint *Blueprint =
      FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
  return Super::IsCompatibleWithGraph(Graph) &&
         (!Blueprint || (FBlueprintEditorUtils::FindUserConstructionScript(
                             Blueprint) != Graph &&
                         Blueprint->GeneratedClass->GetDefaultObject()
                             ->ImplementsGetWorld()));
}

void UK2Node_GetPooledActorHandle::GetNodeAttributes(
    TArray<TKeyValuePair<FString, FString>> &OutNodeAttributes) const {
  const UClass *ClassToSpawn = GetClassToSpawn();
  const FString ClassToSpawnStr =
      ClassToSpawn ? ClassToSpawn->GetName() : TEXT("InvalidClass");
  OutNodeAttributes.Add(TKeyValuePair<FString, FString>(
      TEXT("Type"), TEXT("GetPooledActorHandle")));
  OutNodeAttributes.Add(
      TKeyValuePair<FString, FString>(TEXT("Class"), GetClass()->GetName()));
  OutNodeAttributes.Add(
      TKeyValuePair<FString, FString>(TEXT("Name"), GetName()));
  OutNodeAttributes.Add(
      TKeyValuePair<FString, FString>(TEXT("ActorClass"), ClassToSpawnStr));
}

FNodeHandlingFunctor *UK2Node_GetPooledActorHandle::CreateNodeHandler(
    FKismetCompilerContext &CompilerContext) const {
  return new FNodeHandlingFunctor(CompilerContext);
}

void UK2Node_GetPooledActorHandle::GetMenuActions(
    FBlueprintActionDatabaseRegistrar &ActionRegistrar) const {
  const UClass *ActionKey = GetClass();
  if (ActionRegistrar.IsOpenForRegistration(ActionKey)) {
    UBlueprintNodeSpawner *NodeSpawner =
        UBlueprintNodeSpawner::Create(GetClass());
    check(NodeSpawner != nullptr);
    ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
  }
}

FText UK2Node_GetPooledActorHandle::GetMenuCategory() const {
  return LOCTEXT("GetPooledActorHandle_MenuCategory", "Object Pool");
}

UClass *UK2Node_GetPooledActorHandle::GetClassPinBaseClass() const {
  return AActor::StaticClass();
}

UEdGraphPin *UK2Node_GetPooledActorHandle::GetActorTransformPin() const {
  UEdGraphPin *Pin = FindPinChecked(
      FK2Node_GetPooledActorHandleHelper::ActorClassTransformPinName);
  check(Pin->Direction == EGPD_Input);
  return Pin;
}

UEdGraphPin *UK2Node_GetPooledActorHandle::GetOwnerPin() const {
  UEdGraphPin *Pin = FindPin(FK2Node_GetPooledActorHandleHelper::OwnerPinName);
  check(Pin == nullptr || Pin->Direction == EGPD_Input);
  return Pin;
}

bool UK2Node_GetPooledActorHandle::IsSpawnVarPin(UEdGraphPin *Pin) const {
  const UEdGraphPin *ParentPin = Pin->ParentPin;
  while (ParentPin) {
    if (ParentPin->PinName ==
        FK2Node_GetPooledActorHandleHelper::ActorClassTransformPinName) {
      return false;
    }
    ParentPin = ParentPin->ParentPin;
  }

  return (Super::IsSpawnVarPin(Pin) &&
          Pin->PinName !=
              FK2Node_GetPooledActorHandleHelper::ActorClassTransformPinName &&
          Pin->PinName != FK2Node_GetPooledActorHandleHelper::OwnerPinName);
}

void UK2Node_GetPooledActorHandle::PostReconstructNode() {
  // Call parent implementation
  Super::PostReconstructNode();

  // Force the result pin to stay as FPooledActorHandle struct
  if (UEdGraphPin *ResultPin = GetResultPin()) {
    ResultPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    ResultPin->PinType.PinSubCategoryObject =
        FPooledActorHandle::StaticStruct();
    ResultPin->PinType.PinSubCategory = NAME_None;
  }
}

void UK2Node_GetPooledActorHandle::PinDefaultValueChanged(UEdGraphPin *Pin) {
  // Call parent implementation
  Super::PinDefaultValueChanged(Pin);

  // If the class pin changed, force result pin type to remain
  // FPooledActorHandle
  if (Pin && Pin == GetClassPin()) {
    if (UEdGraphPin *ResultPin = GetResultPin()) {
      ResultPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
      ResultPin->PinType.PinSubCategoryObject =
          FPooledActorHandle::StaticStruct();
      ResultPin->PinType.PinSubCategory = NAME_None;
    }
  }
}

void UK2Node_GetPooledActorHandle::ReallocatePinsDuringReconstruction(
    TArray<UEdGraphPin *> &OldPins) {
  // Call parent implementation
  Super::ReallocatePinsDuringReconstruction(OldPins);

  // After reconstruction, ensure result pin is FPooledActorHandle
  if (UEdGraphPin *ResultPin = GetResultPin()) {
    ResultPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    ResultPin->PinType.PinSubCategoryObject =
        FPooledActorHandle::StaticStruct();
    ResultPin->PinType.PinSubCategory = NAME_None;
  }
}

UK2Node_CallFunction *UK2Node_GetPooledActorHandle::CreateGetSubsystemNode(
    FKismetCompilerContext &CompilerContext, UEdGraph *SourceGraph,
    UEdGraphPin *&OutWorldContextPin) {
  UK2Node_CallFunction *GetSubsystemNode =
      CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this,
                                                                  SourceGraph);
  GetSubsystemNode->FunctionReference.SetExternalMember(
      GET_FUNCTION_NAME_CHECKED(ULazyDynamicObjectPoolLibrary, GetSubsystem),
      ULazyDynamicObjectPoolLibrary::StaticClass());
  GetSubsystemNode->AllocateDefaultPins();

  return GetSubsystemNode;
}

#undef LOCTEXT_NAMESPACE
