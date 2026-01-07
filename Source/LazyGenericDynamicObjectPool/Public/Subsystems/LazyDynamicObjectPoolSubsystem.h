// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HAL/CriticalSection.h"
#include "LazyDynamicObjectPoolSubsystem.generated.h"

class ULazyDynamicObjectPoolSettings;
struct FPooledActorHandle;

DECLARE_MULTICAST_DELEGATE(FDynamicObjectPoolAction);

UENUM(BlueprintType)
enum class EPoolHealthStatus : uint8
{
    Healthy,
    Warning,
    Critical,
    Failed
};

USTRUCT(BlueprintType)
struct FPoolPerformanceMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageRetrievalTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float AverageReturnTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 CacheMisses = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 CacheHits = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    float MemoryEfficiency = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 TotalRetrievals = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Performance")
    int32 TotalReturns = 0;

    void Reset()
    {
        AverageRetrievalTime = 0.0f;
        AverageReturnTime = 0.0f;
        CacheMisses = 0;
        CacheHits = 0;
        MemoryEfficiency = 0.0f;
        TotalRetrievals = 0;
        TotalReturns = 0;
    }
};

USTRUCT(BlueprintType)
struct FPoolAnalytics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Analytics")
    TArray<float> UsageHistory;

    UPROPERTY(BlueprintReadOnly, Category = "Analytics")
    float PredictedNextUsage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Analytics")
    int32 MaxHistorySize = 100;

    void AddUsagePoint(float Usage)
    {
        UsageHistory.Add(Usage);
        if (UsageHistory.Num() > MaxHistorySize)
        {
            UsageHistory.RemoveAt(0);
        }
        UpdatePrediction();
    }

    void UpdatePrediction()
    {
        if (UsageHistory.Num() < 3) return;

        // Simple moving average prediction
        float Sum = 0.0f;
        const int32 WindowSize = FMath::Min(10, UsageHistory.Num());
        for (int32 i = UsageHistory.Num() - WindowSize; i < UsageHistory.Num(); ++i)
        {
            Sum += UsageHistory[i];
        }
        PredictedNextUsage = Sum / WindowSize;
    }

    int32 GetRecommendedPoolSize(int32 CurrentSize) const
    {
        if (PredictedNextUsage <= 0.0f) return CurrentSize;

        // Add 20% buffer to predicted usage
        return FMath::CeilToInt(PredictedNextUsage * 1.2f);
    }
};

/** Information about a potentially leaked actor */
USTRUCT(BlueprintType)
struct FLeakedActorInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Leak Detection")
    TWeakObjectPtr<AActor> Actor;

    UPROPERTY(BlueprintReadOnly, Category = "Leak Detection")
    FString ActorName;

    UPROPERTY(BlueprintReadOnly, Category = "Leak Detection")
    TSubclassOf<AActor> ActorClass;

    UPROPERTY(BlueprintReadOnly, Category = "Leak Detection")
    float TimeInUse = 0.0f; // Seconds the actor has been in use

    UPROPERTY(BlueprintReadOnly, Category = "Leak Detection")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Leak Detection")
    bool bIsStillValid = true;

    FLeakedActorInfo() = default;

    FLeakedActorInfo(AActor* InActor, const TSubclassOf<AActor> InClass, const float InTime)
        : Actor(InActor)
        , ActorName(InActor ? InActor->GetName() : TEXT("Invalid"))
        , ActorClass(InClass)
        , TimeInUse(InTime)
        , Location(InActor ? InActor->GetActorLocation() : FVector::ZeroVector)
        , bIsStillValid(InActor != nullptr)
    {}
};

USTRUCT(BlueprintType)
struct FObjectPool
{
    GENERATED_BODY()
    
    // Use weak pointers for safer memory management
    TArray<TWeakObjectPtr<AActor>> AvailableObjects;
    TArray<TWeakObjectPtr<AActor>> InUseObjects;
        
    int32 AccessCount = 0;
    int32 TotalGrowthOperations = 0;
        
    // Performance metrics
    FPoolPerformanceMetrics PerformanceMetrics;
    FPoolAnalytics Analytics;
        
    // Health monitoring
    EPoolHealthStatus HealthStatus = EPoolHealthStatus::Healthy;
    double LastHealthCheck = 0.0;

    // Leak detection - tracks when actors were retrieved
    TMap<TWeakObjectPtr<AActor>, double> ActorRetrievalTimes;

    // Memory optimization
    int32 ReservedSize = 0;

    // Default constructor
    FObjectPool()
    {
        PoolMutex = MakeShareable(new FCriticalSection());
    }

    // Copy constructor
    FObjectPool(const FObjectPool& Other)
        : AvailableObjects(Other.AvailableObjects)
        , InUseObjects(Other.InUseObjects)
        , AccessCount(Other.AccessCount)
        , TotalGrowthOperations(Other.TotalGrowthOperations)
        , PerformanceMetrics(Other.PerformanceMetrics)
        , Analytics(Other.Analytics)
        , HealthStatus(Other.HealthStatus)
        , LastHealthCheck(Other.LastHealthCheck)
        , ActorRetrievalTimes(Other.ActorRetrievalTimes)
        , ReservedSize(Other.ReservedSize)
    {
        PoolMutex = MakeShareable(new FCriticalSection());
    }

    // Assignment operator
    FObjectPool& operator=(const FObjectPool& Other)
    {
        if (this != &Other)
        {
            AvailableObjects = Other.AvailableObjects;
            InUseObjects = Other.InUseObjects;
            AccessCount = Other.AccessCount;
            TotalGrowthOperations = Other.TotalGrowthOperations;
            PerformanceMetrics = Other.PerformanceMetrics;
            Analytics = Other.Analytics;
            HealthStatus = Other.HealthStatus;
            LastHealthCheck = Other.LastHealthCheck;
            ActorRetrievalTimes = Other.ActorRetrievalTimes;
            ReservedSize = Other.ReservedSize;

            // Create new mutex for this instance
            if (!PoolMutex.IsValid())
            {
                PoolMutex = MakeShareable(new FCriticalSection());
            }
        }
        return *this;
    }

    // Move constructor
    FObjectPool(FObjectPool&& Other) noexcept
        : AvailableObjects(MoveTemp(Other.AvailableObjects))
        , InUseObjects(MoveTemp(Other.InUseObjects))
        , AccessCount(Other.AccessCount)
        , TotalGrowthOperations(Other.TotalGrowthOperations)
        , PerformanceMetrics(MoveTemp(Other.PerformanceMetrics))
        , Analytics(MoveTemp(Other.Analytics))
        , HealthStatus(Other.HealthStatus)
        , LastHealthCheck(Other.LastHealthCheck)
        , ActorRetrievalTimes(MoveTemp(Other.ActorRetrievalTimes))
        , ReservedSize(Other.ReservedSize)
        , PoolMutex(MoveTemp(Other.PoolMutex))
    {
        Other.AccessCount = 0;
        Other.TotalGrowthOperations = 0;
        Other.HealthStatus = EPoolHealthStatus::Healthy;
        Other.LastHealthCheck = 0.0;
        Other.ReservedSize = 0;
    }

    // Move assignment operator
    FObjectPool& operator=(FObjectPool&& Other) noexcept
    {
        if (this != &Other)
        {
            AvailableObjects = MoveTemp(Other.AvailableObjects);
            InUseObjects = MoveTemp(Other.InUseObjects);
            AccessCount = Other.AccessCount;
            TotalGrowthOperations = Other.TotalGrowthOperations;
            PerformanceMetrics = MoveTemp(Other.PerformanceMetrics);
            Analytics = MoveTemp(Other.Analytics);
            HealthStatus = Other.HealthStatus;
            LastHealthCheck = Other.LastHealthCheck;
            ActorRetrievalTimes = MoveTemp(Other.ActorRetrievalTimes);
            ReservedSize = Other.ReservedSize;
            PoolMutex = MoveTemp(Other.PoolMutex);

            Other.AccessCount = 0;
            Other.TotalGrowthOperations = 0;
            Other.HealthStatus = EPoolHealthStatus::Healthy;
            Other.LastHealthCheck = 0.0;
            Other.ReservedSize = 0;
        }
        return *this;
    }

private:
    // Thread safety - using shared pointer to make it copyable
    TSharedPtr<FCriticalSection> PoolMutex;

public:
    void Reserve(int32 Size)
    {
        if (PoolMutex.IsValid())
        {
            FScopeLock Lock(PoolMutex.Get());
            AvailableObjects.Reserve(Size);
            InUseObjects.Reserve(Size);
            ReservedSize = Size;
        }
    }

    void CompactMemory()
    {
        if (PoolMutex.IsValid())
        {
            FScopeLock Lock(PoolMutex.Get());
            AvailableObjects.Shrink();
            InUseObjects.Shrink();
        }
    }

    int32 GetTotalSize() const
    {
        if (PoolMutex.IsValid())
        {
            FScopeLock Lock(PoolMutex.Get());
            return AvailableObjects.Num() + InUseObjects.Num();
        }
        return 0;
    }

    int32 GetAvailableCount() const
    {
        if (PoolMutex.IsValid())
        {
            FScopeLock Lock(PoolMutex.Get());
            return AvailableObjects.Num();
        }
        return 0;
    }

    int32 GetInUseCount() const
    {
        if (PoolMutex.IsValid())
        {
            FScopeLock Lock(PoolMutex.Get());
            return InUseObjects.Num();
        }
        return 0;
    }

    // Clean up invalid weak pointers
    void CleanupInvalidObjects()
    {
        if (PoolMutex.IsValid())
        {
            FScopeLock Lock(PoolMutex.Get());
                
            AvailableObjects.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr)
            {
                return !Ptr.IsValid();
            });
                
            InUseObjects.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr)
            {
                return !Ptr.IsValid();
            });
        }
    }

    // Thread-safe access to mutex for external operations
    FCriticalSection* GetMutex() const
    {
        return PoolMutex.IsValid() ? PoolMutex.Get() : nullptr;
    }
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPoolHealthChanged, TSubclassOf<AActor>, EPoolHealthStatus);

UCLASS()
class LAZYGENERICDYNAMICOBJECTPOOL_API ULazyDynamicObjectPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    FDynamicObjectPoolAction OnActorAddedToPool;
    FDynamicObjectPoolAction OnActorRemovedFromPool;
    FDynamicObjectPoolAction OnActorSpawn;
    FDynamicObjectPoolAction OnActorDestroy;
    FOnPoolHealthChanged OnPoolHealthChanged;

private:

    // Thread-safe pool storage
    TMap<TSubclassOf<AActor>, FObjectPool> ObjectPools;
    mutable FCriticalSection ObjectPoolsMutex;

    UPROPERTY()
    ULazyDynamicObjectPoolSettings* Settings = nullptr;

    FTimerHandle AutoShrinkTimerHandle;
    FTimerHandle ShrinkTimeProgressTimerHandle;
    FTimerHandle HealthCheckTimerHandle;
    FTimerHandle MetricsUpdateTimerHandle;
    
    float NextShrinkTime = 0;
    int32 TotalShrinkOperations = 0;

    // Performance tracking
    double LastPerformanceUpdate = 0.0;
    TMap<TSubclassOf<AActor>, double> LastRetrievalTimes;
    TMap<TSubclassOf<AActor>, double> LastReturnTimes;

public:
    ULazyDynamicObjectPoolSubsystem();
 
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Core pool operations with thread safety
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    bool CreatePool(TSubclassOf<AActor> ClassType, int32 InitialSize = -1);

    UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (BlueprintInternalUseOnly = "true"))
    AActor* InitializeActorFromPool(TSubclassOf<AActor> ActorClass, AActor* NewOwner);

    UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (BlueprintInternalUseOnly = "true"))
    AActor* FinishInitializeActorFromPool(AActor* Actor, const FTransform& NewTransform, bool bSweep = false,
    FHitResult OutSweepHitResult = FHitResult(), ETeleportType Teleport = ETeleportType::None);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ReturnActorToPool(AActor* Actor);

    // Thread-safe variants
    AActor* GetActorFromPool_ThreadSafe(const TSubclassOf<AActor>& ActorClass, AActor* NewOwner = nullptr);
    void ReturnActorToPool_ThreadSafe(AActor* Actor);

    // ========================================
    // Safe Handle-Based API (Recommended)
    // ========================================

    /**
     * Get an actor from the pool wrapped in a safe handle.
     * The handle automatically validates the actor hasn't been returned to the pool.
     * This is the recommended way to use pooled actors as it prevents accidental use of inactive actors.
     *
     * @param ActorClass The class of actor to retrieve from the pool
     * @param NewOwner Optional owner for the spawned actor
     * @return A handle that validates the actor on each access
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (BlueprintInternalUseOnly = "true"))
    FPooledActorHandle GetPooledActorHandle(TSubclassOf<AActor> ActorClass, AActor* NewOwner = nullptr);

    /**
     * Spawn a pooled actor with a safe handle at the specified transform.
     * This is equivalent to spawning an actor but uses the object pool for better performance.
     * The returned handle automatically validates the actor hasn't been returned to the pool.
     *
     * @param ActorClass The class of actor to spawn
     * @param SpawnTransform The transform to spawn the actor at
     * @param NewOwner Optional owner for the spawned actor
     * @return A handle that validates the actor on each access
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (BlueprintInternalUseOnly = "true"))
    FPooledActorHandle SpawnPooledActorSafe(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, AActor* NewOwner = nullptr);

    // Pool information (thread-safe)
    UFUNCTION(BlueprintPure, Category = "Object Pool")
    int32 GetPoolSize(const TSubclassOf<AActor>& ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    float GetNextAutoShrinkTime() const { return NextShrinkTime; }
    
    UFUNCTION(BlueprintPure, Category = "Object Pool")
    bool IsActorActiveInPool(const AActor* Actor) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    bool IsActorInactiveInPool(AActor* Actor) const;

    // Enhanced helper functions
    UFUNCTION(BlueprintPure, Category = "Object Pool")
    TArray<AActor*> GetAvailableActorsInPool(TSubclassOf<AActor> ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    TArray<AActor*> GetInUseActorsInPool(TSubclassOf<AActor> ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    TArray<AActor*> GetAllActorsInPool(TSubclassOf<AActor> ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    TArray<TSubclassOf<AActor>> GetAllPooledClasses() const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    int32 GetTotalActorsInAllPools() const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    int32 GetMaximumPoolSize() const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    int32 GetTotalShrinkOperations() const { return TotalShrinkOperations; }

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    int32 GetPoolGrowthOperation(const TSubclassOf<AActor>& ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    int32 GetPoolAccessCount(const TSubclassOf<AActor>& ClassType) const;

    // Performance and health monitoring
    UFUNCTION(BlueprintPure, Category = "Object Pool")
    FPoolPerformanceMetrics GetPoolPerformanceMetrics(const TSubclassOf<AActor>& ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    float GetCacheHitRate(const TSubclassOf<AActor>& ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    EPoolHealthStatus GetPoolHealth(const TSubclassOf<AActor>& ClassType) const;

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    FPoolAnalytics GetPoolAnalytics(const TSubclassOf<AActor>& ClassType) const;

    // Pool management
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ClearAllPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ClearPool(TSubclassOf<AActor> ClassType);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ShrinkAllPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void OptimizeAllPools();

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void PrewarmPool(TSubclassOf<AActor> ClassType, int32 TargetSize);

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    float GetTotalActorsInPoolRatio();

    // Leak detection
    UFUNCTION(BlueprintCallable, Category = "Object Pool|Leak Detection")
    TArray<FLeakedActorInfo> ScanForLeaks(TSubclassOf<AActor> ClassType, float MinTimeInUse = 30.0f);

    UFUNCTION(BlueprintCallable, Category = "Object Pool|Leak Detection")
    TArray<FLeakedActorInfo> ScanAllPoolsForLeaks(float MinTimeInUse = 30.0f);

    UFUNCTION(BlueprintCallable, Category = "Object Pool|Leak Detection")
    int32 GetTotalLeakedActorsCount(float MinTimeInUse = 30.0f);

    UFUNCTION(BlueprintCallable, Category = "Object Pool|Leak Detection")
    bool ForceReturnLeakedActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Object Pool|Leak Detection")
    int32 ForceReturnAllLeakedActors(TSubclassOf<AActor> ClassType, float MinTimeInUse = 30.0f);

    /**
     * Find orphaned actors - actors that exist in the world but are NOT tracked by the pool
     * These are actors that were spawned outside the pool or lost their tracking
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool|Leak Detection")
    TArray<FLeakedActorInfo> ScanForOrphanedActors(TSubclassOf<AActor> ClassType);

    // Memory management
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void CompactPoolMemory(TSubclassOf<AActor> ClassType);

    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void CompactAllPoolMemory();

    UFUNCTION(BlueprintPure, Category = "Object Pool")
    int32 GetPoolMemoryUsage(TSubclassOf<AActor> ClassType) const;

private:
    // Core functionality
    void ActivateActor(AActor* Actor);
    void DeactivateActor(AActor* Actor);
    void GrowActorPool(FObjectPool& Pool, TSubclassOf<AActor> ActorClass, int32 GrowthAmount);
    void ConfigurePoolActor(AActor* Actor, TSubclassOf<AActor> ActorClass);
    void ConfigureActorComponents(const AActor* Actor);
    FTransform GetSafePoolTransform() const;
    void ShrinkPool(FObjectPool& Pool, const TSubclassOf<AActor>& ActorClass);
    void PerformAutoShrink();
    
    // Health and performance monitoring
    void UpdatePoolHealth(const TSubclassOf<AActor>& ActorClass, FObjectPool& Pool) const;
    void UpdatePerformanceMetrics();
    void PerformHealthCheck();
    
    // Memory management
    void CleanupInvalidObjects();
    
    UFUNCTION()
    void HandleActorDestroyed(AActor* DestroyedActor);
    
    // Utility functions
    void LogPoolOperation(const FString& Operation, const TSubclassOf<AActor>& ClassType) const;
    void CalculateNextShrinkTime();
    
    // Performance tracking helpers
    void StartRetrievalTimer(const TSubclassOf<AActor>& ActorClass);
    void EndRetrievalTimer(const TSubclassOf<AActor>& ActorClass);
    void StartReturnTimer(const TSubclassOf<AActor>& ActorClass);
    void EndReturnTimer(const TSubclassOf<AActor>& ActorClass);
};
