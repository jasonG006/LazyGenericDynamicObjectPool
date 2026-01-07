// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/ObjectPoolSearchAction.h"

#include "ActorTreeItem.h"
#include "Editor.h"
#include "SceneOutlinerHelpers.h"
#include "SlateOptMacros.h"
#include "Engine/StaticMeshActor.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "Widgets/Layout/SScrollBox.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/RunnableThread.h"
#include "Async/TaskGraphInterfaces.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "FLazyGenericDynamicObjectPoolEditorModule"

//////////////////////////////////////////////////////////////////////////
// SObjectPoolSearchAction Implementation

void SObjectPoolSearchAction::Construct(const FArguments& InArgs)
{
    LastUpdateTime = 0.0;
    LastStatusUpdateTime = 0.0;
    CachedActorCount = 0;
    CachedActorType = TEXT("N/A");
    
    const FLinearColor DarkBackgroundColor = FLinearColor(FColor(190, 190, 190));
    
    ChildSlot
    [
        SNew(SBorder)
        .Padding(10)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
        .BorderBackgroundColor(DarkBackgroundColor)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .Padding(5)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ActorPoolLabel", "Actor Pool"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                ]
                + SHorizontalBox::Slot()
                .Padding(5)
                [
                    SNew(STextBlock)
                    .Text(this, reinterpret_cast<TAttribute<FText>::FGetter::TConstMethodPtr<SObjectPoolSearchAction>>(&
                              SObjectPoolSearchAction::GetActorTypeText))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(5)
            [
                SAssignNew(SearchBox, SSearchBox)
                .HintText(LOCTEXT("SearchBoxHint", "Search actors..."))
                .OnTextChanged(this, &SObjectPoolSearchAction::OnSearchTextChanged)
                .OnTextCommitted(this, &SObjectPoolSearchAction::OnSearchTextCommitted)
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.f)
            [
                SNew(SBox)
                .HAlign(HAlign_Fill)
                .MaxDesiredHeight(300)
                [
                    SNew(SScrollBox)
                    .Orientation(Orient_Vertical)
                    + SScrollBox::Slot()
                    [
                        SAssignNew(ActorListView, SListView<TSharedPtr<FPooledActor>>)
                        .ItemHeight(25)
                        .ListItemsSource(&FilteredActors)
                        .OnGenerateRow(this, &SObjectPoolSearchAction::OnGenerateRowForList)
                        .SelectionMode(ESelectionMode::Single)
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(this, reinterpret_cast<TAttribute<FText>::FGetter::TConstMethodPtr<SObjectPoolSearchAction>>(&
                          SObjectPoolSearchAction::GetFilterActorStatusText))
            ]
        ]
    ];
    
    InitializeWidget();
}

SObjectPoolSearchAction::~SObjectPoolSearchAction()
{
    ShutdownWidget();
}

void SObjectPoolSearchAction::InitializeWidget()
{
    // Initialize background data collector
    DataCollector = MakeUnique<FPoolDataCollector>(SharedThis(this));
    
    // Register for garbage collection notifications
    OnPreGarbageCollectHandle = FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddSP(
        this, &SObjectPoolSearchAction::OnPreGarbageCollect);
    
    RegisterPoolDelegates();
    
    // Set up timers for periodic updates
    if (GEditor)
    {
        FTimerManager& TimerManager = GEditor->GetTimerManager().Get();
        
        TimerManager.SetTimer(RefreshTimerHandle, 
            [this]() { OnTimerRefresh(); }, 
            REFRESH_INTERVAL, true);
            
        TimerManager.SetTimer(StatusUpdateTimerHandle, 
            [this]() { OnTimerStatusUpdate(); }, 
            STATUS_UPDATE_INTERVAL, true);
    }
    
    // Initial data load
    RefreshActorListImmediate();
}

void SObjectPoolSearchAction::ShutdownWidget()
{
    // Clean up timers
    if (GEditor)
    {
        FTimerManager& TimerManager = GEditor->GetTimerManager().Get();
        TimerManager.ClearTimer(RefreshTimerHandle);
        TimerManager.ClearTimer(StatusUpdateTimerHandle);
    }
    
    // Stop background collector
    DataCollector.Reset();
    
    UnregisterPoolDelegates();
    
    if (OnPreGarbageCollectHandle.IsValid())
    {
        FCoreUObjectDelegates::GetPreGarbageCollectDelegate().Remove(OnPreGarbageCollectHandle);
        OnPreGarbageCollectHandle.Reset();
    }
}

void SObjectPoolSearchAction::OnBackgroundDataReady()
{
    if (!DataCollector || !DataCollector->HasPendingData()) return;
        
    // Get collected data from background thread
    AllActors = DataCollector->GetCollectedData();
    
    // Rebuild lookup table
    ActorLookup.Empty(AllActors.Num());
    for (const auto& PooledActor : AllActors)
    {
        if (PooledActor->bIsValid)
        {
            ActorLookup.Add(PooledActor->Actor, PooledActor);
        }
    }
    
    UpdateFilteredList();
    InvalidateCache();
}

void SObjectPoolSearchAction::RegisterPoolDelegates()
{
    UnregisterPoolDelegates(); // Ensure we don't double-register
    
    const UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsValid(World))
        return;
    
    ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
    if (!Subsystem)
        return;
        
    OnActorSpawnHandle = Subsystem->OnActorSpawn.AddSP(this, &SObjectPoolSearchAction::OnActorSpawned);
    OnActorDestroyHandle = Subsystem->OnActorDestroy.AddSP(this, &SObjectPoolSearchAction::OnActorDestroyed);
}

void SObjectPoolSearchAction::UnregisterPoolDelegates()
{
    if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
    {
        if (OnActorSpawnHandle.IsValid())
        {
            Subsystem->OnActorSpawn.Remove(OnActorSpawnHandle);
            OnActorSpawnHandle.Reset();
        }
        
        if (OnActorDestroyHandle.IsValid())
        {
            Subsystem->OnActorDestroy.Remove(OnActorDestroyHandle);
            OnActorDestroyHandle.Reset();
        }
    }
}

void SObjectPoolSearchAction::RefreshActorList()
{
    // Request background update instead of blocking main thread
    if (DataCollector)
    {
        DataCollector->RequestUpdate();
    }
}

void SObjectPoolSearchAction::RefreshActorListImmediate()
{
    const double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastUpdateTime < CACHE_VALIDITY_TIME)
    {
        return; // Too soon, use cached data
    }
    
    AllActors.Empty();
    ActorLookup.Empty();
    
    const UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsValid(World))
    {
        UpdateFilteredList();
        return;
    }

    ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
    if (!Subsystem)
    {
        UpdateFilteredList();
        return;
    }

    TArray<TSubclassOf<AActor>> PooledClasses = Subsystem->GetAllPooledClasses();
    AllActors.Reserve(PooledClasses.Num() * 10); // Rough estimate
    
    for (const TSubclassOf<AActor> Class : PooledClasses)
    {
        TArray<AActor*> Actors = Subsystem->GetAllActorsInPool(Class);
        for (AActor* Actor : Actors)
        {
            if (IsValid(Actor))
            {
                TSharedPtr<FPooledActor> PooledActor = MakeShared<FPooledActor>(Actor);
                AllActors.Add(PooledActor);
                ActorLookup.Add(Actor, PooledActor);
            }
        }
    }

    LastUpdateTime = CurrentTime;
    UpdateFilteredList();
}

void SObjectPoolSearchAction::UpdateFilteredList()
{
    const FString FilterText = SearchBox.IsValid() ? SearchBox->GetText().ToString() : FString();
    ApplySearchFilter(FilterText);
    
    if (ActorListView.IsValid())
    {
        ActorListView->RequestListRefresh();
    }
}

void SObjectPoolSearchAction::CleanupInvalidActors()
{
    // Update validity status
    for (const auto& PooledActor : AllActors)
    {
        PooledActor->UpdateValidity();
    }
    
    // Remove invalid actors
    AllActors.RemoveAll([](const TSharedPtr<FPooledActor>& Actor) {
        return !Actor->bIsValid;
    });
    
    // Rebuild lookup
    ActorLookup.Empty(AllActors.Num());
    for (const auto& PooledActor : AllActors)
    {
        if (PooledActor->bIsValid)
        {
            ActorLookup.Add(PooledActor->Actor, PooledActor);
        }
    }
    
    UpdateFilteredList();
}

void SObjectPoolSearchAction::OnActorSpawned()
{
    // Lightweight update - just request background refresh
    RefreshActorList();
}

void SObjectPoolSearchAction::OnActorDestroyed()
{
    // Clean up invalid actors
    CleanupInvalidActors();
}

void SObjectPoolSearchAction::OnPreGarbageCollect()
{
    // Clean up before GC to prevent stale references
    CleanupInvalidActors();
}

void SObjectPoolSearchAction::OnTimerRefresh()
{
    RefreshActorList();
}

void SObjectPoolSearchAction::OnTimerStatusUpdate()
{
    InvalidateCache();
}

TSharedRef<ITableRow> SObjectPoolSearchAction::OnGenerateRowForList(
    TSharedPtr<FPooledActor> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    if (!Item.IsValid() || !Item->bIsValid)
    {
        return SNew(STableRow<TSharedPtr<FPooledActor>>, OwnerTable)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("InvalidActor", "Invalid Actor"))
            .ColorAndOpacity(FSlateColor(FLinearColor::Red))
        ];
    }

    AActor* Actor = Item->Actor.Get();
    if (!IsValid(Actor))
    {
        return SNew(STableRow<TSharedPtr<FPooledActor>>, OwnerTable)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("StaleActor", "Stale Actor"))
            .ColorAndOpacity(FSlateColor(FLinearColor::Yellow))
        ];
    }

    // Create the main horizontal box for the row
    TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

    // Add the icon
    HorizontalBox->AddSlot()
    .AutoWidth()
    .VAlign(VAlign_Center)
    .Padding(0, 0, 4, 0)
    [
        SNew(SImage)
        .Image(FAppStyle::GetBrush(Actor->IsA<AStaticMeshActor>() ? 
            "ClassIcon.StaticMeshActor" : "ClassIcon.Actor"))
        .ColorAndOpacity(FSlateColor::UseForeground())
    ];

    // Add the actor name
    HorizontalBox->AddSlot()
    .FillWidth(1.0f)
    .VAlign(VAlign_Center)
    .Padding(0, 0, 8, 0)
    [
        SNew(STextBlock)
        .Text(FText::FromString(Item->CachedName))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
        .ColorAndOpacity(FSlateColor::UseForeground())
    ];

    // Create the default text widget for class name
    TSharedRef<STextBlock> DefaultClassText = SNew(STextBlock)
        .Text(FText::FromString(Item->CachedClassName))
        .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
        .ColorAndOpacity(FSlateColor(FLinearColor::Gray));

    // Try to construct a hyperlink
    TSharedPtr<SWidget> ClassHyperlink = ConstructClassHyperlink(Item);
    
    if (ClassHyperlink.IsValid())
    {
        // If we have a hyperlink, hide the default text and show the hyperlink
        DefaultClassText->SetVisibility(EVisibility::Collapsed);
        
        HorizontalBox->AddSlot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(8, 0, 0, 0)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("NoBorder"))
            .ForegroundColor_Lambda([OwnerTable]() -> FSlateColor
            {
                // Check if the current row is selected to adjust hyperlink color
                if (OwnerTable->GetParentWidget()->HasKeyboardFocus())
                {
                    return FStyleColors::ForegroundHover;
                }
                return FSlateColor::UseStyle();
            })
            [
                ClassHyperlink.ToSharedRef()
            ]
        ];
    }
    else
    {
        // If no hyperlink, show the default class text
        HorizontalBox->AddSlot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            DefaultClassText
        ];
    }

    return SNew(STableRow<TSharedPtr<FPooledActor>>, OwnerTable)
    .Padding(FMargin(4))
    [
        HorizontalBox
    ];
}

TSharedPtr<SWidget> SObjectPoolSearchAction::ConstructClassHyperlink(const TSharedPtr<FPooledActor>& PooledActor)
{
    if (!PooledActor.IsValid() || !PooledActor->bIsValid) return nullptr;
    
    AActor* Actor = PooledActor->Actor.Get();
    if (!IsValid(Actor)) return nullptr;

    // Use the Scene Outliner helper to create the class hyperlink
    return SceneOutliner::FSceneOutlinerHelpers::GetClassHyperlink(Actor);
}

void SObjectPoolSearchAction::OnSearchTextChanged(const FText& InSearchText)
{
    const FString NewSearchText = InSearchText.ToString();
    if (CurrentSearchText != NewSearchText)
    {
        CurrentSearchText = NewSearchText;
        ApplySearchFilter(CurrentSearchText);
        
        if (ActorListView.IsValid())
        {
            ActorListView->RequestListRefresh();
        }
    }
}

void SObjectPoolSearchAction::OnSearchTextCommitted(const FText& InSearchText, ETextCommit::Type CommitType)
{
    OnSearchTextChanged(InSearchText);
}

FText SObjectPoolSearchAction::GetFilterActorStatusText()
{
    const double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastStatusUpdateTime < STATUS_UPDATE_INTERVAL)
    {
        return CachedStatusText; // Return cached value
    }
    
    const int32 ActorCount = FilteredActors.Num();
    if (CachedActorCount != ActorCount)
    {
        CachedActorCount = ActorCount;
        CachedStatusText = FText::Format(LOCTEXT("FilterTextBlock", "Number of actors: {0}"), 
            FText::AsNumber(ActorCount));
    }
    
    LastStatusUpdateTime = CurrentTime;
    return CachedStatusText;
}

FText SObjectPoolSearchAction::GetActorTypeText()
{
    if (!ShouldUpdateCache())
    {
        return CachedActorTypeText;
    }
    
    const UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsValid(World))
    {
        CachedActorTypeText = FText::FromString(TEXT("N/A"));
        return CachedActorTypeText;
    }
    
    if (AllActors.Num() > 0 && AllActors[0]->bIsValid)
    {
        const FString NewActorType = AllActors[0]->CachedClassName;
        if (CachedActorType != NewActorType)
        {
            CachedActorType = NewActorType;
            CachedActorTypeText = FText::FromString(CachedActorType);
        }
    }
    else
    {
        CachedActorTypeText = LOCTEXT("NoActorsText", "No actors in pool");
    }
    
    return CachedActorTypeText;
}

ULazyDynamicObjectPoolSubsystem* SObjectPoolSearchAction::GetPoolSubsystem() const
{
    const UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    return IsValid(World) ? World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>() : nullptr;
}

bool SObjectPoolSearchAction::ShouldUpdateCache() const
{
    const double CurrentTime = FPlatformTime::Seconds();
    return (CurrentTime - LastUpdateTime) > CACHE_VALIDITY_TIME;
}

void SObjectPoolSearchAction::InvalidateCache()
{
    LastUpdateTime = 0.0;
    LastStatusUpdateTime = 0.0;
}

bool SObjectPoolSearchAction::PassesSearchFilter(const TSharedPtr<FPooledActor>& Actor, const FString& FilterText) const
{
    if (FilterText.IsEmpty())
        return true;
        
    if (!Actor.IsValid())
        return false;
    
    // Use cached names for faster searching
    return Actor->CachedName.Contains(FilterText) || Actor->CachedClassName.Contains(FilterText);
}

void SObjectPoolSearchAction::ApplySearchFilter(const FString& FilterText)
{
    FilteredActors.Empty(AllActors.Num());
    
    if (FilterText.IsEmpty())
    {
        FilteredActors = AllActors;
    }
    else
    {
        FilteredActors.Reserve(AllActors.Num() / 2); // Rough estimate
        for (const TSharedPtr<FPooledActor>& Actor : AllActors)
        {
            if (PassesSearchFilter(Actor, FilterText))
            {
                FilteredActors.Add(Actor);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// FPoolDataCollector Implementation

FPoolDataCollector::FPoolDataCollector(TWeakPtr<SObjectPoolSearchAction> InOwner)
    : Owner(InOwner)
    , Thread(nullptr)
    , UpdateRequestEvent(nullptr)
    , bStopRequested(false)
    , bHasPendingData(false)
{
    UpdateRequestEvent = FPlatformProcess::GetSynchEventFromPool(false);
    Thread = FRunnableThread::Create(this, TEXT("PoolDataCollector"), 0, TPri_BelowNormal);
}

FPoolDataCollector::~FPoolDataCollector()
{
    if (Thread)
    {
        FPoolDataCollector::Stop();
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }
    
    if (UpdateRequestEvent)
    {
        FPlatformProcess::ReturnSynchEventToPool(UpdateRequestEvent);
        UpdateRequestEvent = nullptr;
    }
}

bool FPoolDataCollector::Init()
{
    return true;
}

uint32 FPoolDataCollector::Run()
{
    while (!bStopRequested.Load())
    {
        // Wait for update request or timeout
        if (UpdateRequestEvent->Wait(1000)) // 1 second timeout
        {
            if (!bStopRequested.Load())
            {
                CollectPoolData();
                
                // Notify UI thread asynchronously
                if (TSharedPtr<SObjectPoolSearchAction> OwnerPin = Owner.Pin())
                {
                    AsyncTask(ENamedThreads::GameThread, [WeakOwner = Owner]()
                    {
                        if (TSharedPtr<SObjectPoolSearchAction> OwnerPin = WeakOwner.Pin())
                        {
                            OwnerPin->OnBackgroundDataReady();
                        }
                    });
                }
            }
        }
    }
    
    return 0;
}

void FPoolDataCollector::Stop()
{
    bStopRequested.Store(true);
    if (UpdateRequestEvent)
    {
        UpdateRequestEvent->Trigger();
    }
}

void FPoolDataCollector::Exit()
{
    // Cleanup if needed
}

void FPoolDataCollector::RequestUpdate()
{
    if (!bStopRequested.Load() && UpdateRequestEvent)
    {
        UpdateRequestEvent->Trigger();
    }
}

TArray<TSharedPtr<FPooledActor>> FPoolDataCollector::GetCollectedData()
{
    FScopeLock Lock(&DataLock);
    bHasPendingData.Store(false);
    return MoveTemp(CollectedActors);
}

void FPoolDataCollector::CollectPoolData()
{
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        if (bStopRequested.Load()) return;
            
        const UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!IsValid(World)) return;

        ULazyDynamicObjectPoolSubsystem* Subsystem = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
        if (!Subsystem) return;

        TArray<TSharedPtr<FPooledActor>> NewActors;
        TArray<TSubclassOf<AActor>> PooledClasses = Subsystem->GetAllPooledClasses();
        NewActors.Reserve(PooledClasses.Num() * 10); // Estimate

        for (const TSubclassOf<AActor> Class : PooledClasses)
        {
            if (bStopRequested.Load()) return;
                
            TArray<AActor*> Actors = Subsystem->GetAllActorsInPool(Class);
            for (AActor* Actor : Actors)
            {
                if (IsValid(Actor))
                {
                    NewActors.Add(MakeShared<FPooledActor>(Actor));
                }
            }
        }

        // Store results
        {
            FScopeLock Lock(&DataLock);
            CollectedActors = MoveTemp(NewActors);
            bHasPendingData.Store(true);
        }

        // Notify UI thread directly since we're on the game thread
        if (TSharedPtr<SObjectPoolSearchAction> OwnerPin = Owner.Pin())
        {
            OwnerPin->OnBackgroundDataReady();
        }
    });
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE