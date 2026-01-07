// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Core/PooledActorHandle.h"
#include "DeveloperSettings/LazyDynamicObjectPoolSettings.h"
#include "Engine/World.h"
#include "HAL/PlatformFilemanager.h"
#include "Interface/PoolableActorInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Profiling/PoolProfilingStats.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "TimerManager.h"

ULazyDynamicObjectPoolSubsystem::ULazyDynamicObjectPoolSubsystem() {}

void ULazyDynamicObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Settings = GetMutableDefault<ULazyDynamicObjectPoolSettings>();
	check(Settings);

	if (Settings->bEnableAutoShrink && IsValid(GetWorld()))
	{
		NextShrinkTime = Settings->AutoShrinkInterval;
		GetWorld()->GetTimerManager().SetTimer(ShrinkTimeProgressTimerHandle, this,
											   &ULazyDynamicObjectPoolSubsystem::CalculateNextShrinkTime, 1.0f, true);
		GetWorld()->GetTimerManager().SetTimer(AutoShrinkTimerHandle, this,
											   &ULazyDynamicObjectPoolSubsystem::PerformAutoShrink,
											   Settings->AutoShrinkInterval, true);
	}

	// Set up health monitoring
	if (IsValid(GetWorld()))
	{
		GetWorld()->GetTimerManager().SetTimer(HealthCheckTimerHandle, this,
											   &ULazyDynamicObjectPoolSubsystem::PerformHealthCheck, 30.0f, true);

		// Set up performance metrics updates
		GetWorld()->GetTimerManager().SetTimer(MetricsUpdateTimerHandle, this,
											   &ULazyDynamicObjectPoolSubsystem::UpdatePerformanceMetrics, 5.0f, true);
	}

	LastPerformanceUpdate = FPlatformTime::Seconds();
}

void ULazyDynamicObjectPoolSubsystem::Deinitialize()
{
	if (IsValid(GetWorld()))
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoShrinkTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ShrinkTimeProgressTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HealthCheckTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(MetricsUpdateTimerHandle);
	}

	ClearAllPools();
	Super::Deinitialize();
}

bool ULazyDynamicObjectPoolSubsystem::CreatePool(const TSubclassOf<AActor> ActorClass, const int32 InitialSize)
{
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot create pool for null ActorClass"));
		return false;
	}

	if (!IsValid(Settings))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot create pool - Settings not initialized"));
		return false;
	}

	FScopeLock GlobalLock(&ObjectPoolsMutex);

	if (ObjectPools.Contains(ActorClass)) { return false; }

	const int32 PoolSize = (InitialSize > 0) ? InitialSize : Settings->DefaultInitialPoolSize;
	FObjectPool& NewPool = ObjectPools.Add(ActorClass, FObjectPool());

	// Pre-allocate memory for better performance
	NewPool.Reserve(PoolSize * 2); // Reserve extra space for growth

	GrowActorPool(NewPool, ActorClass, PoolSize);
	NewPool.HealthStatus = EPoolHealthStatus::Healthy;
	NewPool.LastHealthCheck = FPlatformTime::Seconds();

	LogPoolOperation(FString::Printf(TEXT("Created actor pool for %s with size %%"),
									 ActorClass ? *ActorClass->GetName() : TEXT("NULL")),
					 ActorClass);
	return true;
}

AActor* ULazyDynamicObjectPoolSubsystem::InitializeActorFromPool(TSubclassOf<AActor> ActorClass, AActor* NewOwner)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULazyDynamicObjectPoolSubsystem::InitializeActorFromPool);
	SCOPE_CYCLE_COUNTER(STAT_PoolRetrievals);

	if (!ActorClass) { return nullptr; }

	StartRetrievalTimer(ActorClass);

	FObjectPool* Pool;
	{
		FScopeLock GlobalLock(&ObjectPoolsMutex);
		Pool = ObjectPools.Find(ActorClass);
		if (!Pool)
		{
			CreatePool(ActorClass);
			Pool = ObjectPools.Find(ActorClass);
		}
	}

	if (!Pool)
	{
		EndRetrievalTimer(ActorClass);
		return nullptr;
	}

	if (FCriticalSection* Mutex = Pool->GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		// Clean up any invalid objects first
		Pool->CleanupInvalidObjects();

		if (Pool->AvailableObjects.Num() == 0)
		{
			TRACE_BOOKMARK(TEXT("Pool Cache Miss - Growing Pool"));
			Pool->PerformanceMetrics.CacheMisses++;
			INC_DWORD_STAT(STAT_PoolCacheMisses);

			const int32 GrowthAmount = IsValid(Settings)
				? FMath::Max(1, FMath::FloorToInt(Pool->GetInUseCount() * (Settings->PoolGrowthFactor - 1.0f)))
				: 1; // Fallback to growing by 1 if Settings is unavailable

			// Temporarily release lock for growing
			PoolLock.Unlock();
			GrowActorPool(*Pool, ActorClass, GrowthAmount);
		}
		else
		{
			Pool->PerformanceMetrics.CacheHits++;
			INC_DWORD_STAT(STAT_PoolCacheHits);
		}

		if (Pool->AvailableObjects.Num() == 0)
		{
			EndRetrievalTimer(ActorClass);
			return nullptr;
		}

		const TWeakObjectPtr<AActor> WeakActor = Pool->AvailableObjects.Pop();
		AActor* Actor = WeakActor.Get();

		if (!IsValid(Actor))
		{
			LogPoolOperation(FString::Printf(TEXT("Failed to initialize actor from pool for %s - Invalid actor"),
											 ActorClass ? *ActorClass->GetName() : TEXT("NULL")),
							 ActorClass);
			EndRetrievalTimer(ActorClass);
			return nullptr;
		}

		Pool->AccessCount++;
		Pool->PerformanceMetrics.TotalRetrievals++;

		// Track retrieval time for leak detection
		Pool->ActorRetrievalTimes.Add(WeakActor, FPlatformTime::Seconds());

		if (IsValid(NewOwner)) { Actor->SetOwner(NewOwner); }

		// Update analytics
		Pool->Analytics.AddUsagePoint(static_cast<float>(Pool->GetInUseCount() + 1));

		LogPoolOperation(FString::Printf(TEXT("Initialized actor from pool for %s"),
										 ActorClass ? *ActorClass->GetName() : TEXT("NULL")),
						 ActorClass);

		EndRetrievalTimer(ActorClass);
		return Actor;
	}

	return nullptr;
}

AActor* ULazyDynamicObjectPoolSubsystem::FinishInitializeActorFromPool(AActor* Actor, const FTransform& NewTransform,
																	   bool bSweep, FHitResult OutSweepHitResult,
																	   ETeleportType Teleport)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to activate invalid actor from pool"));
		return nullptr;
	}

	const TSubclassOf<AActor> ActorClass = Actor->GetClass();
	FObjectPool* Pool;
	{
		FScopeLock GlobalLock(&ObjectPoolsMutex);
		Pool = ObjectPools.Find(ActorClass);
	}

	if (!Pool)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to activate actor of type %s from non-existent pool"),
			   ActorClass ? *ActorClass->GetName() : TEXT("NULL"));
		return Actor;
	}

	if (FCriticalSection* Mutex = Pool->GetMutex())
	{
		FScopeLock PoolLock(Mutex);
		Pool->InUseObjects.Add(Actor);
	}

	Actor->SetActorTransform(NewTransform, bSweep, &OutSweepHitResult, Teleport);
	ActivateActor(Actor);

	OnActorAddedToPool.Broadcast();

	LogPoolOperation(
		FString::Printf(TEXT("Activated actor from pool for %s"), ActorClass ? *ActorClass->GetName() : TEXT("NULL")),
		ActorClass);
	return Actor;
}

void ULazyDynamicObjectPoolSubsystem::ReturnActorToPool(AActor* Actor)
{
	if (!IsValid(Actor)) { return; }

	StartReturnTimer(Actor->GetClass());
	ReturnActorToPool_ThreadSafe(Actor);
	EndReturnTimer(Actor->GetClass());
}

AActor* ULazyDynamicObjectPoolSubsystem::GetActorFromPool_ThreadSafe(const TSubclassOf<AActor>& ActorClass,
																	 AActor* NewOwner)
{
	return InitializeActorFromPool(ActorClass, NewOwner);
}

void ULazyDynamicObjectPoolSubsystem::ReturnActorToPool_ThreadSafe(AActor* Actor)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULazyDynamicObjectPoolSubsystem::ReturnActorToPool);
	SCOPE_CYCLE_COUNTER(STAT_PoolReturns);

	if (!IsValid(Actor)) { return; }

	const TSubclassOf<AActor> ActorClass = Actor->GetClass();
	FObjectPool* Pool;
	{
		FScopeLock GlobalLock(&ObjectPoolsMutex);
		Pool = ObjectPools.Find(ActorClass);
	}

	if (!Pool)
	{
		LogPoolOperation(FString::Printf(TEXT("Attempted to return actor of type %s to non-existent pool"),
										 ActorClass ? *ActorClass->GetName() : TEXT("NULL")),
						 ActorClass);
		return;
	}

	// Deactivate the actor first
	DeactivateActor(Actor);

	if (FCriticalSection* Mutex = Pool->GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		// Remove from in-use and add to available
		Pool->InUseObjects.RemoveSingle(Actor);
		Pool->AvailableObjects.Add(Actor);
		Pool->PerformanceMetrics.TotalReturns++;

		// Remove retrieval time tracking (actor is no longer in use)
		Pool->ActorRetrievalTimes.Remove(TWeakObjectPtr<AActor>(Actor));

		// Update analytics
		Pool->Analytics.AddUsagePoint(static_cast<float>(Pool->GetInUseCount()));
	}

	OnActorRemovedFromPool.Broadcast();

	LogPoolOperation(
		FString::Printf(TEXT("Returned actor to pool for %s"), ActorClass ? *ActorClass->GetName() : TEXT("NULL")),
		ActorClass);
}

int32 ULazyDynamicObjectPoolSubsystem::GetPoolSize(const TSubclassOf<AActor>& ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	return Pool ? Pool->GetTotalSize() : 0;
}

bool ULazyDynamicObjectPoolSubsystem::IsActorActiveInPool(const AActor* Actor) const
{
	if (!IsValid(Actor)) { return false; }
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(Actor->GetClass());

	if (!Pool) { return false; }

	return Pool->InUseObjects.Contains(Actor);
}

bool ULazyDynamicObjectPoolSubsystem::IsActorInactiveInPool(AActor* Actor) const
{
	if (!IsValid(Actor)) { return false; }
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(Actor->GetClass());

	if (!Pool) { return false; }

	return Pool->AvailableObjects.Contains(Actor);
}

TArray<AActor*> ULazyDynamicObjectPoolSubsystem::GetAvailableActorsInPool(TSubclassOf<AActor> ClassType) const
{
	TArray<AActor*> Result;
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	if (Pool)
	{
		if (FCriticalSection* Mutex = Pool->GetMutex())
		{
			FScopeLock PoolLock(Mutex);

			for (const TWeakObjectPtr<AActor>& WeakActor : Pool->AvailableObjects)
			{
				if (AActor* Actor = WeakActor.Get()) { Result.Add(Actor); }
			}
		}
	}

	return Result;
}

TArray<AActor*> ULazyDynamicObjectPoolSubsystem::GetInUseActorsInPool(TSubclassOf<AActor> ClassType) const
{
	TArray<AActor*> Result;
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	if (Pool)
	{
		if (FCriticalSection* Mutex = Pool->GetMutex())
		{
			FScopeLock PoolLock(Mutex);

			for (const TWeakObjectPtr<AActor>& WeakActor : Pool->InUseObjects)
			{
				if (AActor* Actor = WeakActor.Get()) { Result.Add(Actor); }
			}
		}
	}

	return Result;
}

TArray<AActor*> ULazyDynamicObjectPoolSubsystem::GetAllActorsInPool(TSubclassOf<AActor> ClassType) const
{
	TArray<AActor*> Result = GetAvailableActorsInPool(ClassType);
	TArray<AActor*> InUse = GetInUseActorsInPool(ClassType);
	Result.Append(InUse);
	return Result;
}

TArray<TSubclassOf<AActor>> ULazyDynamicObjectPoolSubsystem::GetAllPooledClasses() const
{
	TArray<TSubclassOf<AActor>> Result;
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	ObjectPools.GetKeys(Result);
	return Result;
}

int32 ULazyDynamicObjectPoolSubsystem::GetTotalActorsInAllPools() const
{
	int32 Total = 0;
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	for (const auto& Pair : ObjectPools)
	{
		Total += Pair.Value.GetTotalSize();
	}

	return Total;
}

int32 ULazyDynamicObjectPoolSubsystem::GetMaximumPoolSize() const { return Settings ? Settings->MaxPoolSize : 0; }

int32 ULazyDynamicObjectPoolSubsystem::GetPoolGrowthOperation(const TSubclassOf<AActor>& ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	return Pool ? Pool->TotalGrowthOperations : 0;
}

int32 ULazyDynamicObjectPoolSubsystem::GetPoolAccessCount(const TSubclassOf<AActor>& ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	return Pool ? Pool->AccessCount : 0;
}

FPoolPerformanceMetrics
ULazyDynamicObjectPoolSubsystem::GetPoolPerformanceMetrics(const TSubclassOf<AActor>& ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	return Pool ? Pool->PerformanceMetrics : FPoolPerformanceMetrics();
}

float ULazyDynamicObjectPoolSubsystem::GetCacheHitRate(const TSubclassOf<AActor>& ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);

	if (!Pool || Pool->PerformanceMetrics.TotalRetrievals == 0) { return 0.0f; }

	return (static_cast<float>(Pool->PerformanceMetrics.CacheHits) / Pool->PerformanceMetrics.TotalRetrievals) * 100.0f;
}

EPoolHealthStatus ULazyDynamicObjectPoolSubsystem::GetPoolHealth(const TSubclassOf<AActor>& ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	return Pool ? Pool->HealthStatus : EPoolHealthStatus::Failed;
}

FPoolAnalytics ULazyDynamicObjectPoolSubsystem::GetPoolAnalytics(const TSubclassOf<AActor>& ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);
	return Pool ? Pool->Analytics : FPoolAnalytics();
}

void ULazyDynamicObjectPoolSubsystem::ClearAllPools()
{
	TArray<TSubclassOf<AActor>> ClassesToClear;

	// First, collect all class types
	{
		FScopeLock GlobalLock(&ObjectPoolsMutex);
		ObjectPools.GetKeys(ClassesToClear);
	}

	// Clear each pool individually
	for (const TSubclassOf<AActor>& ClassType : ClassesToClear)
	{
		ClearPool(ClassType);
	}
}

// Helper method to clear a specific pool
void ULazyDynamicObjectPoolSubsystem::ClearPool(const TSubclassOf<AActor> ClassType)
{
	TArray<AActor*> ActorsToDestroy;

	{
		FScopeLock GlobalLock(&ObjectPoolsMutex);

		if (FObjectPool* Pool = ObjectPools.Find(ClassType))
		{
			if (FCriticalSection* Mutex = Pool->GetMutex())
			{
				FScopeLock PoolLock(Mutex);

				// Collect all actors
				for (const TWeakObjectPtr<AActor>& WeakActor : Pool->AvailableObjects)
				{
					if (AActor* Actor = WeakActor.Get()) { ActorsToDestroy.Add(Actor); }
				}

				for (const TWeakObjectPtr<AActor>& WeakActor : Pool->InUseObjects)
				{
					if (AActor* Actor = WeakActor.Get()) { ActorsToDestroy.Add(Actor); }
				}

				// Clear the pool arrays
				Pool->AvailableObjects.Empty();
				Pool->InUseObjects.Empty();
			}

			// Remove the pool from the map
			ObjectPools.Remove(ClassType);
		}
	}

	// Destroy actors outside of the lock
	for (AActor* Actor : ActorsToDestroy)
	{
		if (IsValid(Actor)) { Actor->Destroy(); }
	}
}

void ULazyDynamicObjectPoolSubsystem::ShrinkAllPools()
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	for (auto& Pair : ObjectPools)
	{
		ShrinkPool(Pair.Value, Pair.Key);
	}
}

void ULazyDynamicObjectPoolSubsystem::OptimizeAllPools()
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	for (auto& Pair : ObjectPools)
	{
		FObjectPool& Pool = Pair.Value;

		// Clean up invalid objects
		Pool.CleanupInvalidObjects();

		// Compact memory
		Pool.CompactMemory();

		// Update health status
		UpdatePoolHealth(Pair.Key, Pool);
	}
}

void ULazyDynamicObjectPoolSubsystem::PrewarmPool(const TSubclassOf<AActor> ClassType, const int32 TargetSize)
{
	if (!ClassType || TargetSize <= 0) { return; }

	FScopeLock GlobalLock(&ObjectPoolsMutex);
	FObjectPool* Pool = ObjectPools.Find(ClassType);

	if (!Pool)
	{
		CreatePool(ClassType, TargetSize);
		return;
	}

	const int32 CurrentSize = Pool->GetTotalSize();
	if (CurrentSize < TargetSize) { GrowActorPool(*Pool, ClassType, TargetSize - CurrentSize); }
}

float ULazyDynamicObjectPoolSubsystem::GetTotalActorsInPoolRatio()
{
	const int32 MaxSize = GetMaximumPoolSize();
	if (MaxSize <= 0) { return 0.0f; }

	const int32 TotalActors = GetTotalActorsInAllPools();
	return static_cast<float>(TotalActors) / MaxSize;
}

void ULazyDynamicObjectPoolSubsystem::CompactPoolMemory(TSubclassOf<AActor> ClassType)
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	FObjectPool* Pool = ObjectPools.Find(ClassType);

	if (Pool) { Pool->CompactMemory(); }
}

void ULazyDynamicObjectPoolSubsystem::CompactAllPoolMemory()
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	for (auto& Pair : ObjectPools)
	{
		Pair.Value.CompactMemory();
	}
}

int32 ULazyDynamicObjectPoolSubsystem::GetPoolMemoryUsage(TSubclassOf<AActor> ClassType) const
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	const FObjectPool* Pool = ObjectPools.Find(ClassType);

	if (!Pool) { return 0; }

	// Estimate memory usage (this is a rough calculation)
	const int32 TotalActors = Pool->GetTotalSize();
	constexpr int32 EstimatedActorSize = sizeof(AActor); // This is a rough estimate
	const int32 ArrayOverhead = Pool->AvailableObjects.GetAllocatedSize() + Pool->InUseObjects.GetAllocatedSize();

	return (TotalActors * EstimatedActorSize) + ArrayOverhead;
}

void ULazyDynamicObjectPoolSubsystem::ActivateActor(AActor* Actor)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULazyDynamicObjectPoolSubsystem::ActivateActor);
	SCOPE_CYCLE_COUNTER(STAT_PoolActorActivation);

	if (!IsValid(Actor)) { return; }

	// Standard activation
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);

	// Call custom interface if implemented
	if (Actor->Implements<UPoolableActorInterface>())
	{
		IPoolableActorInterface::Execute_OnActivateFromPool(Actor);
		return; // Let the interface handle component activation
	}

	// Activate and configure components
	TInlineComponentArray<UActorComponent*> Components;
	Actor->GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		if (!IsValid(Component)) { continue; }

		Component->Activate(true);

		// Restore component states for active use
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
		{
			PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			PrimComp->SetVisibility(true);
		}
	}

	if (!Actor->HasActorBegunPlay()) { Actor->DispatchBeginPlay(); }
}

void ULazyDynamicObjectPoolSubsystem::DeactivateActor(AActor* Actor)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULazyDynamicObjectPoolSubsystem::DeactivateActor);
	SCOPE_CYCLE_COUNTER(STAT_PoolActorDeactivation);

	if (!IsValid(Actor)) { return; }

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	// Call custom interface if implemented
	if (Actor->Implements<UPoolableActorInterface>())
	{
		IPoolableActorInterface::Execute_OnDeactivateToPool(Actor);
		return;
	}

	// Deactivate components
	TInlineComponentArray<UActorComponent*> Components;
	Actor->GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		if (IsValid(Component)) { Component->Deactivate(); }
	}
}

void ULazyDynamicObjectPoolSubsystem::GrowActorPool(FObjectPool& Pool, TSubclassOf<AActor> ActorClass,
													int32 GrowthAmount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULazyDynamicObjectPoolSubsystem::GrowActorPool);
	SCOPE_CYCLE_COUNTER(STAT_PoolGrowths);

	if (!ActorClass || GrowthAmount <= 0) { return; }

	const int32 CurrentSize = Pool.GetTotalSize();
	const int32 MaxGrowth = Settings && Settings->MaxPoolSize > 0
		? FMath::Min(GrowthAmount, Settings->MaxPoolSize - CurrentSize)
		: GrowthAmount;

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to grow actor pool: World is null"));
		return;
	}

	// Trace bookmark for important growth event
	TRACE_BOOKMARK(TEXT("Pool Growing: %s by %d actors (Current: %d)"),
				   ActorClass ? *ActorClass->GetName() : TEXT("NULL"), MaxGrowth, CurrentSize);

	if (FCriticalSection* Mutex = Pool.GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		// Batch spawn for better performance
		TArray<AActor*> NewActors;
		NewActors.Reserve(MaxGrowth);

		for (int32 i = 0; i < MaxGrowth; ++i)
		{
			// Step 1: Create deferred actor with proper collision handling
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.bDeferConstruction = true; // Explicitly defer construction
			SpawnParams.Name = MakeUniqueObjectName(
				World, ActorClass, FName(*FString::Printf(TEXT("%s_Pool_%d"), *ActorClass->GetName(), i)));

			AActor* NewActor = World->SpawnActorDeferred<AActor>(ActorClass, FTransform::Identity, nullptr, nullptr,
																 ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (!IsValid(NewActor))
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to spawn deferred actor during pool growth for class %s"),
					   ActorClass ? *ActorClass->GetName() : TEXT("NULL"));
				continue;
			}

			// Step 2: Configure the actor while deferred (before FinishSpawning)
			ConfigurePoolActor(NewActor, ActorClass);

			// Step 3: Finish spawning with a safe transform (far from gameplay area)
			const FTransform SafeTransform = GetSafePoolTransform();
			UGameplayStatics::FinishSpawningActor(NewActor, SafeTransform);

			if (!IsValid(NewActor))
			{
				UE_LOG(LogTemp, Warning, TEXT("Actor became invalid after FinishSpawning for class %s"),
					   ActorClass ? *ActorClass->GetName() : TEXT("NULL"));
				continue;
			}

			NewActors.Add(NewActor);
		}

		// Step 4: Process all successfully created actors
		for (AActor* NewActor : NewActors)
		{
			// Set up destruction handling
			NewActor->OnDestroyed.AddDynamic(this, &ULazyDynamicObjectPoolSubsystem::HandleActorDestroyed);

			// Immediately deactivate for pool storage
			DeactivateActor(NewActor);

			// Add to pool
			Pool.AvailableObjects.Add(NewActor);
			Pool.TotalGrowthOperations++;

			OnActorSpawn.Broadcast();
		}
	}

	LogPoolOperation(FString::Printf(TEXT("Grew actor pool for %s by %d actors"),
									 ActorClass ? *ActorClass->GetName() : TEXT("NULL"), MaxGrowth),
					 ActorClass);
}

void ULazyDynamicObjectPoolSubsystem::ConfigurePoolActor(AActor* Actor, TSubclassOf<AActor> ActorClass)
{
	if (!IsValid(Actor)) { return; }

	// Call interface setup if implemented
	if (Actor->Implements<UPoolableActorInterface>()) { IPoolableActorInterface::Execute_OnInitializeForPool(Actor); }

	// Set up any class-specific configuration
	ConfigureActorComponents(Actor);
}

// This function runs on the DEFERRED actor, BEFORE FinishSpawningActor runs.
// By disabling expensive components here, we make the actual Spawn 10x cheaper.
void ULazyDynamicObjectPoolSubsystem::ConfigureActorComponents(const AActor* Actor)
{
	if (!IsValid(Actor)) { return; }

	// 1. Disable Physics & Collision
	// This prevents the physics engine from creating expensive bodies during spawn
	TInlineComponentArray<UPrimitiveComponent*> PrimComps;
	Actor->GetComponents(PrimComps);
	for (UPrimitiveComponent* Comp : PrimComps)
	{
		if (Comp)
		{
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetGenerateOverlapEvents(false);
		}
	}

	// 2. Disable Niagara / Particle Systems
	// This prevents Niagara from compiling buffers and "Auto Activating" during spawn
	TInlineComponentArray<UFXSystemComponent*> FXComps;
	Actor->GetComponents(FXComps);
	for (UFXSystemComponent* Comp : FXComps)
	{
		if (Comp) { Comp->SetAutoActivate(false); }
	}
}

FTransform ULazyDynamicObjectPoolSubsystem::GetSafePoolTransform() const
{
	// This prevents any accidental interactions while actors are being set up
	static const FVector PoolLocation(0.0f, 0.0f, -100000.0f); // Deep underground
	return FTransform(FRotator::ZeroRotator, PoolLocation, FVector::OneVector);
}

void ULazyDynamicObjectPoolSubsystem::ShrinkPool(FObjectPool& Pool, const TSubclassOf<AActor>& ActorClass)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULazyDynamicObjectPoolSubsystem::ShrinkPool);
	SCOPE_CYCLE_COUNTER(STAT_PoolShrinks);

	if (!IsValid(Settings)) { return; }

	if (FCriticalSection* Mutex = Pool.GetMutex())
	{
		FScopeLock PoolLock(Mutex);
		const int32 TotalSize = Pool.GetTotalSize();
		const int32 TargetSize = FMath::Max(Settings->DefaultInitialPoolSize,
											FMath::CeilToInt(TotalSize * (1.0f - Settings->ShrinkThreshold)));

		const int32 NumToRemove = FMath::Max((Pool.AvailableObjects.Num() - TargetSize), 0);
		if (NumToRemove <= 0) { return; }

		// Trace bookmark for shrink event
		TRACE_BOOKMARK(TEXT("Pool Shrinking: %s removing %d actors (Total: %d)"),
					   ActorClass ? *ActorClass->GetName() : TEXT("NULL"), NumToRemove, TotalSize);

		int32 ActuallyRemoved = 0;
		for (int32 i = 0; i < NumToRemove; ++i)
		{
			if (Pool.AvailableObjects.Num() == 0) { break; }

			TWeakObjectPtr<AActor> WeakActor = Pool.AvailableObjects.Pop(EAllowShrinking::No);
			AActor* ActorToRemove = WeakActor.Get();

			if (IsValid(ActorToRemove))
			{
				ActorToRemove->Destroy();
				++ActuallyRemoved;
				OnActorDestroy.Broadcast();
			}
		}

		// Compact memory after shrinking
		Pool.CompactMemory();
		TotalShrinkOperations++;

		LogPoolOperation(FString::Printf(TEXT("Shrunk actor pool for %s by %d actors (attempted %d)"),
										 ActorClass ? *ActorClass->GetName() : TEXT("NULL"), ActuallyRemoved,
										 NumToRemove),
						 ActorClass);
	}
}

void ULazyDynamicObjectPoolSubsystem::PerformAutoShrink()
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	for (auto& Pair : ObjectPools)
	{
		ShrinkPool(Pair.Value, Pair.Key);
		LogPoolOperation(
			FString::Printf(TEXT("Auto-shrunk pool for %s"), Pair.Key ? *Pair.Key->GetName() : TEXT("NULL")), Pair.Key);
	}
}

void ULazyDynamicObjectPoolSubsystem::UpdatePoolHealth(const TSubclassOf<AActor>& ActorClass, FObjectPool& Pool) const
{
	if (FCriticalSection* Mutex = Pool.GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		const int32 TotalSize = Pool.GetTotalSize();
		const int32 InUseCount = Pool.GetInUseCount();
		const float UsageRatio = TotalSize > 0 ? static_cast<float>(InUseCount) / TotalSize : 0.0f;

		EPoolHealthStatus NewStatus = EPoolHealthStatus::Healthy;

		// Determine health status
		if (UsageRatio > 0.9f) { NewStatus = EPoolHealthStatus::Critical; }
		else if (UsageRatio > 0.75f) { NewStatus = EPoolHealthStatus::Warning; }
		else if (TotalSize == 0) { NewStatus = EPoolHealthStatus::Failed; }

		// Check for memory efficiency
		const float MemoryEfficiency = Pool.PerformanceMetrics.TotalRetrievals > 0
			? static_cast<float>(Pool.PerformanceMetrics.CacheHits) / Pool.PerformanceMetrics.TotalRetrievals
			: 1.0f;
		Pool.PerformanceMetrics.MemoryEfficiency = MemoryEfficiency;

		if (Pool.HealthStatus != NewStatus)
		{
			Pool.HealthStatus = NewStatus;
			OnPoolHealthChanged.Broadcast(ActorClass, NewStatus);
		}

		Pool.LastHealthCheck = FPlatformTime::Seconds();
	}
}

void ULazyDynamicObjectPoolSubsystem::UpdatePerformanceMetrics()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULazyDynamicObjectPoolSubsystem::UpdatePerformanceMetrics);

	const double CurrentTime = FPlatformTime::Seconds();
	const double DeltaTime = CurrentTime - LastPerformanceUpdate;

	if (DeltaTime < 1.0)
	{
		return; // Only update once per second
	}

	FScopeLock GlobalLock(&ObjectPoolsMutex);

	// Aggregate stats across all pools
	int32 TotalActors = 0;
	int32 TotalInUse = 0;
	int32 TotalAvailable = 0;
	int32 TotalCacheHits = 0;
	int32 TotalRetrievals = 0;
	int32 TotalMemory = 0;
	int32 HealthyCounts[4] = {0, 0, 0, 0}; // Healthy, Warning, Critical, Failed

	for (auto& Pair : ObjectPools)
	{
		FObjectPool& Pool = Pair.Value;
		if (FCriticalSection* Mutex = Pool.GetMutex())
		{
			FScopeLock PoolLock(Mutex);

			// Aggregate pool data
			const int32 PoolTotal = Pool.GetTotalSize();
			const int32 PoolInUse = Pool.GetInUseCount();
			const int32 PoolAvailable = Pool.GetAvailableCount();

			TotalActors += PoolTotal;
			TotalInUse += PoolInUse;
			TotalAvailable += PoolAvailable;
			TotalCacheHits += Pool.PerformanceMetrics.CacheHits;
			TotalRetrievals += Pool.PerformanceMetrics.TotalRetrievals;
			TotalMemory += GetPoolMemoryUsage(Pair.Key);

			// Count health status
			switch (Pool.HealthStatus)
			{
			case EPoolHealthStatus::Healthy:
				HealthyCounts[0]++;
				break;
			case EPoolHealthStatus::Warning:
				HealthyCounts[1]++;
				break;
			case EPoolHealthStatus::Critical:
				HealthyCounts[2]++;
				break;
			case EPoolHealthStatus::Failed:
				HealthyCounts[3]++;
				break;
			}

			// Update average retrieval and return times
			if (Pool.PerformanceMetrics.TotalRetrievals > 0)
			{
				if (const double* RetrievalTime = LastRetrievalTimes.Find(Pair.Key))
				{
					Pool.PerformanceMetrics.AverageRetrievalTime =
						(Pool.PerformanceMetrics.AverageRetrievalTime + *RetrievalTime) * 0.5f;
				}
			}

			if (Pool.PerformanceMetrics.TotalReturns > 0)
			{
				if (const double* ReturnTime = LastReturnTimes.Find(Pair.Key))
				{
					Pool.PerformanceMetrics.AverageReturnTime =
						(Pool.PerformanceMetrics.AverageReturnTime + *ReturnTime) * 0.5f;
				}
			}
		}
	}

	// Update global stats
	SET_DWORD_STAT(STAT_PoolTotalActors, TotalActors);
	SET_DWORD_STAT(STAT_PoolActorsInUse, TotalInUse);
	SET_DWORD_STAT(STAT_PoolActorsAvailable, TotalAvailable);
	SET_MEMORY_STAT(STAT_PoolMemoryUsage, TotalMemory);

	// Calculate and set cache hit rate
	if (TotalRetrievals > 0)
	{
		const float CacheHitRate = (static_cast<float>(TotalCacheHits) / TotalRetrievals) * 100.0f;
		SET_FLOAT_STAT(STAT_PoolCacheHitRate, CacheHitRate);
	}

	// Update health stats
	SET_DWORD_STAT(STAT_PoolsHealthy, HealthyCounts[0]);
	SET_DWORD_STAT(STAT_PoolsWarning, HealthyCounts[1]);
	SET_DWORD_STAT(STAT_PoolsCritical, HealthyCounts[2]);

	LastPerformanceUpdate = CurrentTime;
}

void ULazyDynamicObjectPoolSubsystem::PerformHealthCheck()
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	for (auto& Pair : ObjectPools)
	{
		UpdatePoolHealth(Pair.Key, Pair.Value);
	}
}

void ULazyDynamicObjectPoolSubsystem::CleanupInvalidObjects()
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	for (auto& Pair : ObjectPools)
	{
		Pair.Value.CleanupInvalidObjects();
	}
}

void ULazyDynamicObjectPoolSubsystem::HandleActorDestroyed(AActor* DestroyedActor)
{
	if (!IsValid(DestroyedActor)) { return; }

	const TSubclassOf<AActor> ActorClass = DestroyedActor->GetClass();
	FScopeLock GlobalLock(&ObjectPoolsMutex);

	if (FObjectPool* Pool = ObjectPools.Find(ActorClass))
	{
		if (FCriticalSection* Mutex = Pool->GetMutex())
		{
			FScopeLock PoolLock(Mutex);

			Pool->AvailableObjects.RemoveSingle(DestroyedActor);
			Pool->InUseObjects.RemoveSingle(DestroyedActor);
		}
	}
}

void ULazyDynamicObjectPoolSubsystem::LogPoolOperation(const FString& Operation,
													   const TSubclassOf<AActor>& ClassType) const
{
	if (Settings->bEnableDetailedLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Class: %s"), *Operation, *ClassType->GetName());
	}
}

void ULazyDynamicObjectPoolSubsystem::CalculateNextShrinkTime()
{
	if (!Settings || !Settings->bEnableAutoShrink)
	{
		NextShrinkTime = 0.0f;
		return;
	}

	NextShrinkTime -= 1.0f; // Decrement by 1 second (called every second)
	if (NextShrinkTime <= 0.0f) { NextShrinkTime = Settings->AutoShrinkInterval; }
}

void ULazyDynamicObjectPoolSubsystem::StartRetrievalTimer(const TSubclassOf<AActor>& ActorClass)
{
	LastRetrievalTimes.Add(ActorClass, FPlatformTime::Seconds());
}

void ULazyDynamicObjectPoolSubsystem::EndRetrievalTimer(const TSubclassOf<AActor>& ActorClass)
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	FObjectPool* Pool = ObjectPools.Find(ActorClass);
	if (!Pool) { return; }

	const double StartTime = LastRetrievalTimes.FindRef(ActorClass);
	const double EndTime = FPlatformTime::Seconds();
	const float RetrievalTime = static_cast<float>(EndTime - StartTime) * 1000.0f; // Convert to milliseconds

	if (FCriticalSection* Mutex = Pool->GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		const int32 TotalRetrievals = Pool->PerformanceMetrics.TotalRetrievals;
		Pool->PerformanceMetrics.AverageRetrievalTime =
			(Pool->PerformanceMetrics.AverageRetrievalTime * (TotalRetrievals - 1) + RetrievalTime) / TotalRetrievals;
	}
}

void ULazyDynamicObjectPoolSubsystem::StartReturnTimer(const TSubclassOf<AActor>& ActorClass)
{
	LastReturnTimes.Add(ActorClass, FPlatformTime::Seconds());
}

void ULazyDynamicObjectPoolSubsystem::EndReturnTimer(const TSubclassOf<AActor>& ActorClass)
{
	FScopeLock GlobalLock(&ObjectPoolsMutex);
	FObjectPool* Pool = ObjectPools.Find(ActorClass);
	if (!Pool) { return; }

	const double StartTime = LastReturnTimes.FindRef(ActorClass);
	const double EndTime = FPlatformTime::Seconds();
	const float ReturnTime = static_cast<float>(EndTime - StartTime) * 1000.0f; // Convert to milliseconds

	if (FCriticalSection* Mutex = Pool->GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		const int32 TotalReturns = Pool->PerformanceMetrics.TotalReturns;
		Pool->PerformanceMetrics.AverageReturnTime =
			(Pool->PerformanceMetrics.AverageReturnTime * (TotalReturns - 1) + ReturnTime) / TotalReturns;
	}
}

FPooledActorHandle ULazyDynamicObjectPoolSubsystem::GetPooledActorHandle(TSubclassOf<AActor> ActorClass,
																		 AActor* NewOwner)
{
	AActor* Actor = InitializeActorFromPool(ActorClass, NewOwner);
	return FPooledActorHandle(Actor, this);
}

FPooledActorHandle ULazyDynamicObjectPoolSubsystem::SpawnPooledActorSafe(TSubclassOf<AActor> ActorClass,
																		 const FTransform& SpawnTransform,
																		 AActor* NewOwner)
{
	AActor* Actor = InitializeActorFromPool(ActorClass, NewOwner);
	if (IsValid(Actor))
	{
		FinishInitializeActorFromPool(Actor, SpawnTransform, false, FHitResult(), ETeleportType::TeleportPhysics);
	}
	return FPooledActorHandle(Actor, this);
}

// ========================================
// Leak Detection Implementation
// ========================================

TArray<FLeakedActorInfo> ULazyDynamicObjectPoolSubsystem::ScanForLeaks(TSubclassOf<AActor> ClassType,
																	   float MinTimeInUse)
{
	TArray<FLeakedActorInfo> LeakedActors;

	if (!ClassType) { return LeakedActors; }

	FScopeLock GlobalLock(&ObjectPoolsMutex);
	FObjectPool* Pool = ObjectPools.Find(ClassType);

	if (!Pool) { return LeakedActors; }

	const double CurrentTime = FPlatformTime::Seconds();

	if (FCriticalSection* Mutex = Pool->GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		// Check all in-use actors
		for (const TWeakObjectPtr<AActor>& WeakActor : Pool->InUseObjects)
		{
			if (!WeakActor.IsValid()) { continue; }

			AActor* Actor = WeakActor.Get();
			const double* RetrievalTime = Pool->ActorRetrievalTimes.Find(WeakActor);

			if (RetrievalTime)
			{
				const float TimeInUse = static_cast<float>(CurrentTime - *RetrievalTime);

				if (TimeInUse >= MinTimeInUse) { LeakedActors.Add(FLeakedActorInfo(Actor, ClassType, TimeInUse)); }
			}
		}
	}

	return LeakedActors;
}

TArray<FLeakedActorInfo> ULazyDynamicObjectPoolSubsystem::ScanAllPoolsForLeaks(float MinTimeInUse)
{
	TArray<FLeakedActorInfo> AllLeakedActors;
	TArray<TSubclassOf<AActor>> AllClasses;

	{
		FScopeLock GlobalLock(&ObjectPoolsMutex);
		ObjectPools.GetKeys(AllClasses);
	}

	for (const TSubclassOf<AActor>& ClassType : AllClasses)
	{
		TArray<FLeakedActorInfo> ClassLeaks = ScanForLeaks(ClassType, MinTimeInUse);
		AllLeakedActors.Append(ClassLeaks);
	}

	return AllLeakedActors;
}

int32 ULazyDynamicObjectPoolSubsystem::GetTotalLeakedActorsCount(float MinTimeInUse)
{
	TArray<FLeakedActorInfo> AllLeaks = ScanAllPoolsForLeaks(MinTimeInUse);
	return AllLeaks.Num();
}

bool ULazyDynamicObjectPoolSubsystem::ForceReturnLeakedActor(AActor* Actor)
{
	if (!IsValid(Actor)) { return false; }

	const TSubclassOf<AActor> ActorClass = Actor->GetClass();

	FScopeLock GlobalLock(&ObjectPoolsMutex);
	FObjectPool* Pool = ObjectPools.Find(ActorClass);

	if (!Pool) { return false; }

	if (FCriticalSection* Mutex = Pool->GetMutex())
	{
		FScopeLock PoolLock(Mutex);

		// Check if actor is in use
		const int32 Index = Pool->InUseObjects.Find(TWeakObjectPtr<AActor>(Actor));
		if (Index != INDEX_NONE)
		{
			// Force return to pool
			ReturnActorToPool(Actor);
			return true;
		}
	}

	return false;
}

int32 ULazyDynamicObjectPoolSubsystem::ForceReturnAllLeakedActors(TSubclassOf<AActor> ClassType, float MinTimeInUse)
{
	TArray<FLeakedActorInfo> LeakedActors = ScanForLeaks(ClassType, MinTimeInUse);
	int32 ReturnedCount = 0;

	for (const FLeakedActorInfo& LeakInfo : LeakedActors)
	{
		if (LeakInfo.Actor.IsValid())
		{
			if (ForceReturnLeakedActor(LeakInfo.Actor.Get())) { ReturnedCount++; }
		}
	}

	return ReturnedCount;
}

TArray<FLeakedActorInfo> ULazyDynamicObjectPoolSubsystem::ScanForOrphanedActors(TSubclassOf<AActor> ClassType)
{
	TArray<FLeakedActorInfo> OrphanedActors;

	if (!ClassType) { return OrphanedActors; }

	UWorld* World = GetWorld();
	if (!World) { return OrphanedActors; }

	// Get the pool for this class
	FObjectPool* Pool = ObjectPools.Find(ClassType);
	if (!Pool)
	{
		// No pool exists, so all actors of this type are orphaned
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(World, ClassType, FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (Actor && Actor->IsValidLowLevel())
			{
				FLeakedActorInfo LeakInfo;
				LeakInfo.Actor = Actor;
				LeakInfo.ActorName = Actor->GetName();
				LeakInfo.ActorClass = ClassType;
				LeakInfo.TimeInUse = 0.0f;
				LeakInfo.Location = Actor->GetActorLocation();
				LeakInfo.bIsStillValid = true;
				OrphanedActors.Add(LeakInfo);
			}
		}

		return OrphanedActors;
	}

	// Build set of all tracked actors
	TSet<TWeakObjectPtr<AActor>> TrackedActors;

	{
		FScopeLock Lock(Pool->GetMutex());

		// Add all available actors
		for (const TWeakObjectPtr<AActor>& Actor : Pool->AvailableObjects)
		{
			if (Actor.IsValid()) { TrackedActors.Add(Actor); }
		}

		// Add all in-use actors
		for (const TWeakObjectPtr<AActor>& Actor : Pool->InUseObjects)
		{
			if (Actor.IsValid()) { TrackedActors.Add(Actor); }
		}
	}

	// Find all actors of this class in the world
	TArray<AActor*> AllActorsInWorld;
	UGameplayStatics::GetAllActorsOfClass(World, ClassType, AllActorsInWorld);

	// Check which ones are NOT tracked by the pool
	for (AActor* Actor : AllActorsInWorld)
	{
		if (!Actor || !Actor->IsValidLowLevel()) { continue; }

		TWeakObjectPtr<AActor> WeakActor(Actor);

		// If not tracked by pool, it's orphaned
		if (!TrackedActors.Contains(WeakActor))
		{
			FLeakedActorInfo LeakInfo;
			LeakInfo.Actor = WeakActor;
			LeakInfo.ActorName = Actor->GetName();
			LeakInfo.ActorClass = ClassType;
			LeakInfo.TimeInUse = 0.0f;
			LeakInfo.Location = Actor->GetActorLocation();
			LeakInfo.bIsStillValid = true;
			OrphanedActors.Add(LeakInfo);
		}
	}

	return OrphanedActors;
}
