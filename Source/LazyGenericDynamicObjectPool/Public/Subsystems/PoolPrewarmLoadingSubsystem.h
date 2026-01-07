// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Loading/PrewarmTask.h"
#include "Subsystems/WorldSubsystem.h"
#include "PoolPrewarmLoadingSubsystem.generated.h"


class ULazyDynamicObjectPoolSubsystem;
class ULazyDynamicObjectPoolSettings;
class UPoolPrewarmDataAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogPoolPrewarm, Log, All);

UE_TRACE_CHANNEL_EXTERN(ObjectPoolChannel);

/**
 * World subsystem responsible for managing pool prewarming during level load.
 * Integrates with AsyncLoadingScreen to prewarm pools while the loading screen is visible.
 */
UCLASS()
class LAZYGENERICDYNAMICOBJECTPOOL_API UPoolPrewarmLoadingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPoolPrewarmLoadingSubsystem();

	// ========================================
	// USubsystem Interface
	// ========================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// ========================================
	// Blueprint Events
	// ========================================

	/** Broadcast when prewarm progress updates. Progress is 0.0 to 1.0 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPrewarmProgress, float, Progress);

	UPROPERTY(BlueprintAssignable, Category = "Object Pool|Loading")
	FOnPrewarmProgress OnPrewarmProgress;

	/** Broadcast when all prewarm tasks complete */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPrewarmComplete);

	UPROPERTY(BlueprintAssignable, Category = "Object Pool|Loading")
	FOnPrewarmComplete OnPrewarmComplete;

	/** Broadcast when a single class prewarm completes */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClassPrewarmComplete, TSubclassOf<AActor>, ActorClass, int32,
	                                             Count);

	UPROPERTY(BlueprintAssignable, Category = "Object Pool|Loading")
	FOnClassPrewarmComplete OnClassPrewarmComplete;

	/** Broadcast when the entire prewarm phase is complete (all tasks done) */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPrewarmPhaseComplete);

	UPROPERTY(BlueprintAssignable, Category = "Object Pool|Loading")
	FOnPrewarmPhaseComplete OnPrewarmPhaseComplete;

	// ========================================
	// Public API
	// ========================================

	/** Queue a prewarm task for an actor class */
	UFUNCTION(BlueprintCallable, Category = "Object Pool|Loading")
	void QueuePrewarmTask(TSubclassOf<AActor> ActorClass, int32 Count, int32 Priority = 0);

	/** Queue multiple prewarm tasks from config array */
	UFUNCTION(BlueprintCallable, Category = "Object Pool|Loading")
	void QueuePrewarmTasks(const TArray<FPoolPrewarmConfig>& Configs);

	/**
	 * Queue prewarm task from a data asset manually.
	 * @param DataAsset The asset defining what to prewarm
	 */
	UFUNCTION(BlueprintCallable, Category = "Object Pool|Loading")
	void QueuePrewarmFromDataAsset(UPoolPrewarmDataAsset* DataAsset);

	/**
	 * Manually trigger the start of prewarming.
	 * Required when bWaitForManualStart is enabled.
	 * Call this when the loading screen is fully visible.
	 */
	UFUNCTION(BlueprintCallable, Category = "Object Pool|Loading")
	void StartPrewarmManually();

	/** Check if currently in prewarm phase (includes pending start) */
	UFUNCTION(BlueprintPure, Category = "Object Pool|Loading")
	bool IsPrewarming() const { return bIsProcessing || bPendingStart; }

	/** Get current prewarm progress (0.0 - 1.0) */
	UFUNCTION(BlueprintPure, Category = "Object Pool|Loading")
	float GetProgress() const;

	/** Get number of remaining prewarm tasks */
	UFUNCTION(BlueprintPure, Category = "Object Pool|Loading")
	int32 GetRemainingTaskCount() const { return TaskQueue.Num(); }

	/** Cancel all pending prewarm tasks */
	UFUNCTION(BlueprintCallable, Category = "Object Pool|Loading")
	void CancelAllTasks();

	/** Force start prewarm processing (usually called automatically on world begin play) */
	UFUNCTION(BlueprintCallable, Category = "Object Pool|Loading")
	void StartProcessing();

protected:
	// ========================================
	// World Lifecycle
	// ========================================

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	// ========================================
	// Internal Methods
	// ========================================

	/** Load configuration from settings and queue tasks */
	void LoadConfigurationAndQueueTasks();

	/** Process tasks within frame budget */
	void ProcessTaskQueue();

	/** Tick function for incremental processing */
	void OnTick();

	/** Called when all prewarm tasks complete */
	void OnAllTasksComplete();

	/** Get the pool subsystem for the current world */
	ULazyDynamicObjectPoolSubsystem* GetPoolSubsystem() const;

	/** Sort task queue by priority */
	void SortTaskQueue();

	// ========================================
	// State
	// ========================================

	/** Queue of pending prewarm tasks */
	TArray<FPrewarmTask> TaskQueue;

	/** Settings reference */
	UPROPERTY()
	TObjectPtr<const ULazyDynamicObjectPoolSettings> Settings;

	/** Timer handle for tick processing */
	FTimerHandle ProcessingTimerHandle;

	/** Timer handle for deferred start */
	FTimerHandle DeferredStartTimerHandle;

	/** Is currently processing prewarm queue */
	bool bIsProcessing = false;

	/** Is waiting to start (deferred start) */
	bool bPendingStart = false;

	/** Remaining frames to wait before starting */
	int32 PendingStartFramesRemaining = 0;

	/** Has completed at least one full prewarm cycle */
	bool bHasCompletedPrewarm = false;

	/** Total actors to spawn (for progress calculation) */
	int32 TotalActorsToSpawn = 0;

	/** Actors spawned so far (for progress calculation) */
	int32 ActorsSpawned = 0;
};
