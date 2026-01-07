// // Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "Core/PooledActorHandle.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loading/PrewarmTask.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "LazyDynamicObjectPoolLibrary.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogLazyDynamicObjectPool, Log, All);

class UPoolPrewarmLoadingSubsystem;

/**
 * Blueprint function library for Lazy Dynamic Object Pool operations
 */
UCLASS()
class LAZYGENERICDYNAMICOBJECTPOOL_API ULazyDynamicObjectPoolLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintPure, Category = "LazyDynamicObjectPoolLibrary",
            meta = (WorldContext = "ContextObject",
                    BlueprintInternalUseOnly = "true"))
  static ULazyDynamicObjectPoolSubsystem *
  GetSubsystem(const UObject *ContextObject);

  // ========================================
  // Pooled Actor Handle Conversion Functions
  // ========================================

  /**
   * Convert a PooledActorHandle to an Actor reference.
   * Returns nullptr if the handle is invalid or actor has been returned to
   * pool.
   *
   * This enables automatic casting in Blueprint graphs.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool",
            meta = (DisplayName = "To Actor (Pooled Handle)",
                    CompactNodeTitle = "->", BlueprintAutocast))
  static AActor *
  Conv_PooledActorHandleToActor(const FPooledActorHandle &Handle);

  /**
   * Break a PooledActorHandle into its components for debugging.
   * Useful for inspecting the handle state in Blueprints.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool", meta = (NativeBreakFunc))
  static void BreakPooledActorHandle(const FPooledActorHandle &Handle,
                                     AActor *&Actor, bool &bIsValid,
                                     bool &bIsActiveInPool);

  /**
   * Create a PooledActorHandle from an actor and subsystem.
   * This is primarily for internal use - prefer using SpawnPooledActorSafe.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool",
            meta = (WorldContext = "ContextObject"))
  static FPooledActorHandle MakePooledActorHandle(const UObject *ContextObject,
                                                  AActor *Actor);

  /**
   * Create a PooledActorHandle from an actor and subsystem directly.
   * This overload is used internally by the K2 node when the subsystem is
   * already available.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool",
            meta = (BlueprintInternalUseOnly = "true"))
  static FPooledActorHandle MakePooledActorHandleFromSubsystem(
      AActor *Actor, ULazyDynamicObjectPoolSubsystem *PoolSubsystem);

  // ========================================
  // Blueprint-Callable Handle Functions
  // ========================================

  /**
   * Get the actor from a pooled actor handle.
   * Returns nullptr if the handle is invalid or actor has been returned to
   * pool.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool",
            meta = (DisplayName = "Get Actor (Handle)"))
  static AActor *GetActorFromHandle(const FPooledActorHandle &Handle);

  /**
   * Check if a pooled actor handle is valid and points to an active actor.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool",
            meta = (DisplayName = "Is Valid (Handle)"))
  static bool IsHandleValid(const FPooledActorHandle &Handle);

  /**
   * Return the actor to the pool using the handle.
   * After calling this, the handle will be invalid.
   */
  UFUNCTION(BlueprintCallable, Category = "Object Pool",
            meta = (DisplayName = "Return To Pool (Handle)"))
  static void ReturnHandleToPool(UPARAM(ref) FPooledActorHandle &Handle);

  /**
   * Check if the handle's actor is currently active in the pool.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool",
            meta = (DisplayName = "Is Active In Pool (Handle)"))
  static bool IsHandleActiveInPool(const FPooledActorHandle &Handle);

  /**
   * Get the class of the actor referenced by the handle.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool",
            meta = (DisplayName = "Get Actor Class (Handle)"))
  static TSubclassOf<AActor>
  GetHandleActorClass(const FPooledActorHandle &Handle);

  // ========================================
  // Convenience Functions (C++ & Blueprint)
  // ========================================

  /**
   * Spawn a pooled actor with safe handle validation (convenience wrapper).
   * This is the recommended way to spawn pooled actors - equivalent to
   * UGameplayStatics::SpawnActor.
   *
   * Example C++ usage:
   *   FPooledActorHandle Handle =
   * ULazyDynamicObjectPoolLibrary::SpawnPooledActorSafe(this, EnemyClass,
   * SpawnTransform); if (Handle)
   *   {
   *       Handle->SetActorLocation(NewLocation);
   *   }
   */
  UFUNCTION(BlueprintCallable, Category = "Object Pool",
            meta = (WorldContext = "WorldContextObject"))
  static FPooledActorHandle SpawnPooledActorSafe(
      const UObject *WorldContextObject, TSubclassOf<AActor> ActorClass,
      const FTransform &SpawnTransform, AActor *Owner = nullptr);

  /**
   * Spawn a pooled actor and return raw pointer (convenience wrapper).
   * WARNING: Returns raw actor pointer. Prefer SpawnPooledActorSafe for
   * automatic validation.
   *
   * Example C++ usage:
   *   AActor* Enemy = ULazyDynamicObjectPoolLibrary::SpawnActorFromPool(this,
   * EnemyClass, SpawnTransform);
   */
  UFUNCTION(BlueprintCallable, Category = "Object Pool",
            meta = (WorldContext = "WorldContextObject"))
  static AActor *SpawnActorFromPool(const UObject *WorldContextObject,
                                    TSubclassOf<AActor> ActorClass,
                                    const FTransform &SpawnTransform,
                                    AActor *Owner = nullptr);

  /**
   * Return an actor to the pool (convenience wrapper).
   *
   * Example C++ usage:
   *   ULazyDynamicObjectPoolLibrary::ReturnActorToPool(this, Actor);
   */
  UFUNCTION(BlueprintCallable, Category = "Object Pool",
            meta = (WorldContext = "WorldContextObject"))
  static void ReturnActorToPool(const UObject *WorldContextObject,
                                AActor *Actor);

  /**
   * Create a pool for a specific actor class (convenience wrapper).
   * Pre-warms the pool with the specified number of actors.
   *
   * Example C++ usage:
   *   ULazyDynamicObjectPoolLibrary::CreatePool(this, EnemyClass, 20);
   */
  UFUNCTION(BlueprintCallable, Category = "Object Pool",
            meta = (WorldContext = "WorldContextObject"))
  static bool CreatePool(const UObject *WorldContextObject,
                         TSubclassOf<AActor> ActorClass,
                         int32 InitialSize = -1);

  /**
   * Pre-warm a pool to the target size (convenience wrapper).
   * Useful for loading screens or level initialization.
   *
   * Example C++ usage:
   *   ULazyDynamicObjectPoolLibrary::PrewarmPool(this, EnemyClass, 50);
   */
  UFUNCTION(BlueprintCallable, Category = "Object Pool",
            meta = (WorldContext = "WorldContextObject"))
  static void PrewarmPool(const UObject *WorldContextObject,
                          TSubclassOf<AActor> ActorClass, int32 TargetSize);

  // ========================================
  // Loading Screen Integration Functions
  // ========================================

  /**
   * Queue a prewarm task during level load.
   * Tasks are processed incrementally while the loading screen is visible.
   */
  UFUNCTION(BlueprintCallable, Category = "Object Pool|Loading",
            meta = (WorldContext = "WorldContextObject"))
  static void QueuePoolPrewarm(const UObject *WorldContextObject,
                               TSubclassOf<AActor> ActorClass, int32 Count,
                               int32 Priority = 0);

  /**
   * Check if the prewarm system is currently processing tasks.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool|Loading",
            meta = (WorldContext = "WorldContextObject"))
  static bool IsInPrewarmPhase(const UObject *WorldContextObject);

  /**
   * Get the current prewarm progress (0.0 - 1.0).
   * Returns 1.0 if not in prewarm phase.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool|Loading",
            meta = (WorldContext = "WorldContextObject"))
  static float GetPrewarmProgress(const UObject *WorldContextObject);

  /**
   * Get the prewarm loading subsystem for advanced usage.
   */
  UFUNCTION(BlueprintPure, Category = "Object Pool|Loading",
            meta = (WorldContext = "WorldContextObject"))
  static UPoolPrewarmLoadingSubsystem *
  GetPrewarmSubsystem(const UObject *WorldContextObject);
};
