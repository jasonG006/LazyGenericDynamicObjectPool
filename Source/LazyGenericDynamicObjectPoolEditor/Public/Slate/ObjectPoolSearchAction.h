// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/SCompoundWidget.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"

class ULazyDynamicObjectPoolSubsystem;

struct FPooledActor
{
    TWeakObjectPtr<AActor> Actor;
    FString CachedName;
    FString CachedClassName;
    bool bIsValid;
    
    FPooledActor(AActor* InActor) 
        : Actor(InActor)
        , bIsValid(IsValid(InActor))
    {
        if (bIsValid)
        {
            CachedName = InActor->GetName();
            CachedClassName = InActor->GetClass()->GetName();
        }
    }
    
    void UpdateValidity()
    {
        bIsValid = Actor.IsValid();
    }
};

/**
 * Optimized Object Pool Search Widget with performance improvements
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SObjectPoolSearchAction : public SCompoundWidget
{
    friend class FPoolDataCollector;
    
public:
    SLATE_BEGIN_ARGS(SObjectPoolSearchAction)
    {}
    SLATE_END_ARGS()
    
    /** Constructs this widget with InArgs */
    void Construct(const FArguments& InArgs);
    virtual ~SObjectPoolSearchAction() override;
    
    // Called by background collector
    void OnBackgroundDataReady();

private:
    // UI Components
    TSharedPtr<SListView<TSharedPtr<FPooledActor>>> ActorListView;
    TSharedPtr<SSearchBox> SearchBox;
    
    // Data Management
    TArray<TSharedPtr<FPooledActor>> AllActors;
    TArray<TSharedPtr<FPooledActor>> FilteredActors;
    TMap<TWeakObjectPtr<AActor>, TSharedPtr<FPooledActor>> ActorLookup;
    
    // Caching
    FString CurrentSearchText;
    FText CachedStatusText;
    FText CachedActorTypeText;
    double LastUpdateTime;
    double LastStatusUpdateTime;
    
    // Performance Tracking
    int32 CachedActorCount;
    FString CachedActorType;
    
    // Delegates
    FDelegateHandle OnActorSpawnHandle;
    FDelegateHandle OnActorDestroyHandle;
    FDelegateHandle OnPreGarbageCollectHandle;
    
    // Background Processing
    TUniquePtr<FPoolDataCollector> DataCollector;
    FTimerHandle RefreshTimerHandle;
    FTimerHandle StatusUpdateTimerHandle;
    
    // Settings
    static constexpr float REFRESH_INTERVAL = 0.5f;
    static constexpr float STATUS_UPDATE_INTERVAL = 1.0f;
    static constexpr double CACHE_VALIDITY_TIME = 1.0;
    
    // Core Functions
    void InitializeWidget();
    void ShutdownWidget();
    
    // Data Management
    void RefreshActorList();
    void RefreshActorListImmediate();
    void UpdateFilteredList();
    void CleanupInvalidActors();
    
    // Event Handlers
    void OnActorSpawned();
    void OnActorDestroyed();
    void OnPreGarbageCollect();
    void OnTimerRefresh();
    void OnTimerStatusUpdate();
    
    // UI Generation
    TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FPooledActor> Item, const TSharedRef<STableViewBase>& OwnerTable);
    static TSharedPtr<SWidget> ConstructClassHyperlink(const TSharedPtr<FPooledActor>& TreeItem);
    void OnSearchTextChanged(const FText& InSearchText);
    void OnSearchTextCommitted(const FText& InSearchText, ETextCommit::Type CommitType);
    
    // Cached Getters
    FText GetFilterActorStatusText();
    FText GetActorTypeText();
    
    // Utility Functions
    void RegisterPoolDelegates();
    void UnregisterPoolDelegates();
    ULazyDynamicObjectPoolSubsystem* GetPoolSubsystem() const;
    bool ShouldUpdateCache() const;
    void InvalidateCache();
    
    // Filtering
    bool PassesSearchFilter(const TSharedPtr<FPooledActor>& Actor, const FString& FilterText) const;
    void ApplySearchFilter(const FString& FilterText);
};

// Background task for pool data collection
class FPoolDataCollector : public FRunnable
{
public:
    FPoolDataCollector(TWeakPtr<SObjectPoolSearchAction> InOwner);
    virtual ~FPoolDataCollector() override;
    
    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;
    
    void RequestUpdate();
    bool HasPendingData() const { return bHasPendingData.Load(); }
    TArray<TSharedPtr<FPooledActor>> GetCollectedData();
    
private:
    TWeakPtr<SObjectPoolSearchAction> Owner;
    FRunnableThread* Thread;
    FEvent* UpdateRequestEvent;
    TAtomic<bool> bStopRequested;
    TAtomic<bool> bHasPendingData;
    FCriticalSection DataLock;
    TArray<TSharedPtr<FPooledActor>> CollectedActors;
    
    void CollectPoolData();
};
