// // Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "FunctionLibrary/LazyDynamicObjectPoolLibrary.h"
#include "Subsystems/PoolPrewarmLoadingSubsystem.h"

DEFINE_LOG_CATEGORY(LogLazyDynamicObjectPool);

ULazyDynamicObjectPoolSubsystem *
ULazyDynamicObjectPoolLibrary::GetSubsystem(const UObject *ContextObject) {
  if (!IsValid(ContextObject)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("GetSubsystem: ContextObject is not valid"));
    return nullptr;
  }

  const UWorld *World = GEngine->GetWorldFromContextObject(
      ContextObject, EGetWorldErrorMode::LogAndReturnNull);
  if (!IsValid(World)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("GetSubsystem: World is not valid"));
    return nullptr;
  }

  ULazyDynamicObjectPoolSubsystem *Subsystem =
      World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
  return Subsystem;
}

AActor *ULazyDynamicObjectPoolLibrary::Conv_PooledActorHandleToActor(
    const FPooledActorHandle &Handle) {
  return Handle.GetActor();
}

void ULazyDynamicObjectPoolLibrary::BreakPooledActorHandle(
    const FPooledActorHandle &Handle, AActor *&Actor, bool &bIsValid,
    bool &bIsActiveInPool) {
  Actor = Handle.GetActor();
  bIsValid = Handle.IsValid();
  bIsActiveInPool = Handle.IsActiveInPool();
}

FPooledActorHandle ULazyDynamicObjectPoolLibrary::MakePooledActorHandle(
    const UObject *ContextObject, AActor *Actor) {
  if (!IsValid(Actor))
    return FPooledActorHandle();

  ULazyDynamicObjectPoolSubsystem *Subsystem = GetSubsystem(ContextObject);
  if (!IsValid(Subsystem))
    return FPooledActorHandle();

  return FPooledActorHandle(Actor, Subsystem);
}

FPooledActorHandle
ULazyDynamicObjectPoolLibrary::MakePooledActorHandleFromSubsystem(
    AActor *Actor, ULazyDynamicObjectPoolSubsystem *PoolSubsystem) {
  if (!IsValid(Actor) || !IsValid(PoolSubsystem))
    return FPooledActorHandle();

  return FPooledActorHandle(Actor, PoolSubsystem);
}

AActor *ULazyDynamicObjectPoolLibrary::GetActorFromHandle(
    const FPooledActorHandle &Handle) {
  return Handle.GetActor();
}

bool ULazyDynamicObjectPoolLibrary::IsHandleValid(
    const FPooledActorHandle &Handle) {
  return Handle.IsValid();
}

void ULazyDynamicObjectPoolLibrary::ReturnHandleToPool(
    FPooledActorHandle &Handle) {
  Handle.ReturnToPool();
}

bool ULazyDynamicObjectPoolLibrary::IsHandleActiveInPool(
    const FPooledActorHandle &Handle) {
  return Handle.IsActiveInPool();
}

TSubclassOf<AActor> ULazyDynamicObjectPoolLibrary::GetHandleActorClass(
    const FPooledActorHandle &Handle) {
  return Handle.GetActorClass();
}

FPooledActorHandle ULazyDynamicObjectPoolLibrary::SpawnPooledActorSafe(
    const UObject *WorldContextObject, TSubclassOf<AActor> ActorClass,
    const FTransform &SpawnTransform, AActor *Owner) {
  ULazyDynamicObjectPoolSubsystem *Subsystem = GetSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("SpawnPooledActorSafe: Failed to get subsystem"));
    return FPooledActorHandle();
  }

  return Subsystem->SpawnPooledActorSafe(ActorClass, SpawnTransform, Owner);
}

AActor *ULazyDynamicObjectPoolLibrary::SpawnActorFromPool(
    const UObject *WorldContextObject, TSubclassOf<AActor> ActorClass,
    const FTransform &SpawnTransform, AActor *Owner) {
  ULazyDynamicObjectPoolSubsystem *Subsystem = GetSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("SpawnActorFromPool: Failed to get subsystem"));
    return nullptr;
  }

  AActor *Actor = Subsystem->InitializeActorFromPool(ActorClass, Owner);
  if (IsValid(Actor)) {
    Subsystem->FinishInitializeActorFromPool(Actor, SpawnTransform, false,
                                             FHitResult(),
                                             ETeleportType::TeleportPhysics);
  }

  return Actor;
}

void ULazyDynamicObjectPoolLibrary::ReturnActorToPool(
    const UObject *WorldContextObject, AActor *Actor) {
  if (!IsValid(Actor)) {
    return;
  }

  ULazyDynamicObjectPoolSubsystem *Subsystem = GetSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("ReturnActorToPool: Failed to get subsystem"));
    return;
  }

  Subsystem->ReturnActorToPool(Actor);
}

bool ULazyDynamicObjectPoolLibrary::CreatePool(
    const UObject *WorldContextObject, TSubclassOf<AActor> ActorClass,
    int32 InitialSize) {
  ULazyDynamicObjectPoolSubsystem *Subsystem = GetSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("CreatePool: Failed to get subsystem"));
    return false;
  }

  return Subsystem->CreatePool(ActorClass, InitialSize);
}

void ULazyDynamicObjectPoolLibrary::PrewarmPool(
    const UObject *WorldContextObject, TSubclassOf<AActor> ActorClass,
    int32 TargetSize) {
  ULazyDynamicObjectPoolSubsystem *Subsystem = GetSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("PrewarmPool: Failed to get subsystem"));
    return;
  }

  Subsystem->PrewarmPool(ActorClass, TargetSize);
}

// ========================================
// Loading Screen Integration Functions
// ========================================

UPoolPrewarmLoadingSubsystem *
ULazyDynamicObjectPoolLibrary::GetPrewarmSubsystem(
    const UObject *WorldContextObject) {
  if (!IsValid(WorldContextObject)) {
    return nullptr;
  }

  const UWorld *World = GEngine->GetWorldFromContextObject(
      WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
  if (!IsValid(World)) {
    return nullptr;
  }

  return World->GetSubsystem<UPoolPrewarmLoadingSubsystem>();
}

void ULazyDynamicObjectPoolLibrary::QueuePoolPrewarm(
    const UObject *WorldContextObject, TSubclassOf<AActor> ActorClass,
    int32 Count, int32 Priority) {
  UPoolPrewarmLoadingSubsystem *Subsystem =
      GetPrewarmSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    UE_LOG(LogLazyDynamicObjectPool, Warning,
           TEXT("QueuePoolPrewarm: Failed to get prewarm subsystem"));
    return;
  }

  Subsystem->QueuePrewarmTask(ActorClass, Count, Priority);
}

bool ULazyDynamicObjectPoolLibrary::IsInPrewarmPhase(
    const UObject *WorldContextObject) {
  const UPoolPrewarmLoadingSubsystem *Subsystem =
      GetPrewarmSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    return false;
  }

  return Subsystem->IsPrewarming();
}

float ULazyDynamicObjectPoolLibrary::GetPrewarmProgress(
    const UObject *WorldContextObject) {
  const UPoolPrewarmLoadingSubsystem *Subsystem =
      GetPrewarmSubsystem(WorldContextObject);
  if (!IsValid(Subsystem)) {
    return 1.0f; // Return 1.0 if no subsystem (not in prewarm phase)
  }

  return Subsystem->GetProgress();
}
