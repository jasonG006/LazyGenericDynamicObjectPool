// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Subsystems/PoolPrewarmLoadingSubsystem.h"
#include "DeveloperSettings/LazyDynamicObjectPoolSettings.h"
#include "Engine/World.h"
#include "Loading/PoolPrewarmDataAsset.h"
#include "ProfilingDebugging/CountersTrace.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogPoolPrewarm);
UE_TRACE_CHANNEL_DEFINE(ObjectPoolChannel);

UPoolPrewarmLoadingSubsystem::UPoolPrewarmLoadingSubsystem() {}

void UPoolPrewarmLoadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Get settings reference
	Settings = GetDefault<ULazyDynamicObjectPoolSettings>();

	UE_LOG(LogPoolPrewarm, Log, TEXT("PoolPrewarmLoadingSubsystem initialized. Loading integration: %s"),
		   Settings && Settings->bEnableLoadingIntegration ? TEXT("Enabled") : TEXT("Disabled"));
}

void UPoolPrewarmLoadingSubsystem::Deinitialize()
{
	// Clean up timer if active
	if (UWorld* World = GetWorld()) { World->GetTimerManager().ClearTimer(ProcessingTimerHandle); }

	TaskQueue.Empty();
	bIsProcessing = false;

	Super::Deinitialize();
}

bool UPoolPrewarmLoadingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create in game worlds, not editor preview worlds
	if (const UWorld* World = Cast<UWorld>(Outer)) { return World->IsGameWorld(); }
	return false;
}

void UPoolPrewarmLoadingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!IsValid(Settings) || !Settings->bEnableLoadingIntegration)
	{
		UE_LOG(LogPoolPrewarm, Verbose, TEXT("Skipping prewarm - loading integration disabled"));
		return;
	}

	// Load config and queue tasks
	LoadConfigurationAndQueueTasks();

	// Start processing if we have tasks
	if (TaskQueue.Num() > 0) { StartProcessing(); }
	else
	{
		UE_LOG(LogPoolPrewarm, Log, TEXT("No prewarm tasks configured, skipping prewarm phase"));
	}
}

void UPoolPrewarmLoadingSubsystem::LoadConfigurationAndQueueTasks()
{
	if (!IsValid(Settings)) { return; }

	// Check Level Filtering
	if (const UWorld* World = GetWorld())
	{
		const FSoftObjectPath CurrentMap(World);
		// Note: We strip the PIE prefix if necessary, but SoftObjectPath often handles package paths well.
		// For robustness, checking package name string contains:
		const FString CurrentMapName = World->GetPackage()->GetName();

		bool bShouldPrewarm = Settings->bPrewarmAllLevelsByDefault;

		if (Settings->bPrewarmAllLevelsByDefault)
		{
			// EXCLUSION CHECK
			for (const FSoftObjectPath& Excluded : Settings->ExcludedLevels)
			{
				// Check using string contains to avoid strict path matching issues with PIE (UEDPIE_0_...)
				if (CurrentMapName.Contains(Excluded.GetAssetName()))
				{
					bShouldPrewarm = false;
					UE_LOG(LogPoolPrewarm, Log, TEXT("Level '%s' is EXCLUDED from prewarming by config."),
						   *CurrentMapName);
					break;
				}
			}
		}
		else
		{
			// INCLUSION CHECK
			bShouldPrewarm = false; // Default to false
			for (const FSoftObjectPath& Included : Settings->IncludedLevels)
			{
				if (CurrentMapName.Contains(Included.GetAssetName()))
				{
					bShouldPrewarm = true;
					UE_LOG(LogPoolPrewarm, Log, TEXT("Level '%s' is INCLUDED in prewarming by config."),
						   *CurrentMapName);
					break;
				}
			}
		}

		if (!bShouldPrewarm)
		{
			UE_LOG(LogPoolPrewarm, Verbose, TEXT("Skipping prewarm for level '%s' (Config Disabled)"), *CurrentMapName);
			return;
		}
	}

	// Queue global prewarm configs
	for (const FPoolPrewarmConfig& Config : Settings->GlobalPrewarmConfigs)
	{
		if (Config.IsValid()) { QueuePrewarmTask(Config.ActorClass, Config.PrewarmCount, Config.Priority); }
	}

	UE_LOG(LogPoolPrewarm, Log, TEXT("Queued %d prewarm tasks from global configuration"), TaskQueue.Num());
}

void UPoolPrewarmLoadingSubsystem::QueuePrewarmTask(TSubclassOf<AActor> ActorClass, int32 Count, int32 Priority)
{
	if (!ActorClass || Count <= 0) { return; }

	FPrewarmTask Task;
	Task.ActorClass = ActorClass;
	Task.RemainingCount = Count;
	Task.TotalCount = Count;
	Task.Priority = Priority;

	TaskQueue.Add(Task);
	TotalActorsToSpawn += Count;

	if (IsValid(Settings) && Settings->bLogPrewarmProgress)
	{
		UE_LOG(LogPoolPrewarm, Log, TEXT("Queued prewarm: %s x%d (Priority: %d)"), *ActorClass->GetName(), Count,
			   Priority);
	}
}

void UPoolPrewarmLoadingSubsystem::QueuePrewarmTasks(const TArray<FPoolPrewarmConfig>& Configs)
{
	for (const FPoolPrewarmConfig& Config : Configs)
	{
		if (Config.IsValid()) { QueuePrewarmTask(Config.ActorClass, Config.PrewarmCount, Config.Priority); }
	}
}

void UPoolPrewarmLoadingSubsystem::QueuePrewarmFromDataAsset(UPoolPrewarmDataAsset* DataAsset)
{
	if (!DataAsset) { return; }

	// If data asset overrides global config, clear existing tasks
	if (DataAsset->bOverrideGlobalConfig)
	{
		TaskQueue.Empty();
		TotalActorsToSpawn = 0;
		ActorsSpawned = 0;
	}

	QueuePrewarmTasks(DataAsset->PrewarmConfigs);

	UE_LOG(LogPoolPrewarm, Log, TEXT("Queued %d prewarm tasks from data asset (Override: %s)"),
		   DataAsset->PrewarmConfigs.Num(), DataAsset->bOverrideGlobalConfig ? TEXT("Yes") : TEXT("No"));
}

float UPoolPrewarmLoadingSubsystem::GetProgress() const
{
	if (TotalActorsToSpawn <= 0) { return bHasCompletedPrewarm ? 1.0f : 0.0f; }
	return static_cast<float>(ActorsSpawned) / TotalActorsToSpawn;
}

void UPoolPrewarmLoadingSubsystem::CancelAllTasks()
{
	TaskQueue.Empty();
	bIsProcessing = false;

	if (UWorld* World = GetWorld()) { World->GetTimerManager().ClearTimer(ProcessingTimerHandle); }

	UE_LOG(LogPoolPrewarm, Log, TEXT("All prewarm tasks cancelled"));
}

void UPoolPrewarmLoadingSubsystem::StartPrewarmManually()
{
	if (!IsValid(Settings) || !Settings->bEnableLoadingIntegration)
	{
		UE_LOG(LogPoolPrewarm, Warning, TEXT("StartPrewarmManually called but loading integration is disabled"));
		return;
	}

	UE_LOG(LogPoolPrewarm, Log, TEXT("Manual Prewarm Triggered"));
	bPendingStart = false; // logic for deferred start if any

	StartProcessing();
}

void UPoolPrewarmLoadingSubsystem::StartProcessing()
{
	if (bIsProcessing || TaskQueue.Num() == 0) { return; }

	bIsProcessing = true;

	// Sort by priority
	SortTaskQueue();

	UE_LOG(LogPoolPrewarm, Log, TEXT("Starting prewarm processing: %d tasks, %d total actors"), TaskQueue.Num(),
		   TotalActorsToSpawn);

	// Start tick-based processing
	if (UWorld* World = GetWorld())
	{
		// [FIX] Use SetTimerForNextTick to guarantee per-frame execution without "Catch-Up" spikes.
		// This replaces the old 0.001f looping timer.
		World->GetTimerManager().SetTimerForNextTick(this, &UPoolPrewarmLoadingSubsystem::OnTick);

		// Trace Bookmark for verifying start time in Insights
		TRACE_BOOKMARK(TEXT("Pool Prewarm Start"));
	}
}

void UPoolPrewarmLoadingSubsystem::OnTick()
{
	if (!bIsProcessing || TaskQueue.Num() == 0)
	{
		OnAllTasksComplete();
		return;
	}

	ProcessTaskQueue();

	// Broadcast progress
	const float Progress = GetProgress();
	OnPrewarmProgress.Broadcast(Progress);

	if (IsValid(Settings) && Settings->bLogPrewarmProgress)
	{
		UE_LOG(LogPoolPrewarm, Verbose, TEXT("Prewarm progress: %.1f%% (%d/%d actors)"), Progress * 100.0f,
			   ActorsSpawned, TotalActorsToSpawn);
	}

	// [FIX] Recursively schedule the next tick IF there are still tasks.
	// This ensures we stop ticking exactly when finished.
	if (bIsProcessing && TaskQueue.Num() > 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(this, &UPoolPrewarmLoadingSubsystem::OnTick);
		}
	}
}

void UPoolPrewarmLoadingSubsystem::ProcessTaskQueue()
{
	ULazyDynamicObjectPoolSubsystem* PoolSubsystem = GetPoolSubsystem();
	if (!PoolSubsystem)
	{
		UE_LOG(LogPoolPrewarm, Warning, TEXT("Cannot process prewarm - pool subsystem not available"));
		OnAllTasksComplete();
		return;
	}

	const double StartTime = FPlatformTime::Seconds();
	const double FrameBudgetSeconds = IsValid(Settings) ? Settings->PrewarmFrameBudgetMs / 1000.0 : 0.008;
	const bool bHasFrameBudget = FrameBudgetSeconds > 0.0;

	// Trace the budget logic specifically
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UPoolPrewarmLoadingSubsystem::ProcessTaskQueue, ObjectPoolChannel);

	// Process tasks within frame budget
	while (TaskQueue.Num() > 0)
	{
		FPrewarmTask& CurrentTask = TaskQueue[0];

		// Get current pool size and calculate cumulative target
		const int32 CurrentPoolSize = PoolSubsystem->GetPoolSize(CurrentTask.ActorClass);
		const int32 SpawnedForThisTask = CurrentTask.TotalCount - CurrentTask.RemainingCount;
		const int32 TargetPoolSize = CurrentPoolSize + 1;

		// Prewarm to the new target size (adds 1 actor)
		PoolSubsystem->PrewarmPool(CurrentTask.ActorClass, TargetPoolSize);
		CurrentTask.RemainingCount--;
		ActorsSpawned++;

		// Check if this task is complete
		if (CurrentTask.IsComplete())
		{
			OnClassPrewarmComplete.Broadcast(CurrentTask.ActorClass, CurrentTask.TotalCount);

			if (IsValid(Settings) && Settings->bLogPrewarmProgress)
			{
				UE_LOG(LogPoolPrewarm, Log, TEXT("Completed prewarm: %s x%d"), *CurrentTask.ActorClass->GetName(),
					   CurrentTask.TotalCount);
			}

			TaskQueue.RemoveAt(0);
		}

		// Check frame budget
		if (bHasFrameBudget)
		{
			const double ElapsedTime = FPlatformTime::Seconds() - StartTime;
			if (ElapsedTime >= FrameBudgetSeconds) { break; }
		}
	}

	// Check if all tasks complete
	if (TaskQueue.Num() == 0) { OnAllTasksComplete(); }
}

void UPoolPrewarmLoadingSubsystem::OnAllTasksComplete()
{
	bIsProcessing = false;
	bHasCompletedPrewarm = true;

	// Clear timer
	if (UWorld* World = GetWorld()) { World->GetTimerManager().ClearTimer(ProcessingTimerHandle); }

	UE_LOG(LogPoolPrewarm, Log, TEXT("Prewarm complete! Spawned %d actors"), ActorsSpawned);

	// Trace Marker
	TRACE_BOOKMARK(TEXT("Pool Prewarm Complete"));

	// Broadcast completion
	OnPrewarmProgress.Broadcast(1.0f);
	OnPrewarmComplete.Broadcast();
	OnPrewarmPhaseComplete.Broadcast();
}

ULazyDynamicObjectPoolSubsystem* UPoolPrewarmLoadingSubsystem::GetPoolSubsystem() const
{
	if (const UWorld* World = GetWorld()) { return World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>(); }
	return nullptr;
}

void UPoolPrewarmLoadingSubsystem::SortTaskQueue() { TaskQueue.Sort(FPrewarmTaskPrioritySort()); }
