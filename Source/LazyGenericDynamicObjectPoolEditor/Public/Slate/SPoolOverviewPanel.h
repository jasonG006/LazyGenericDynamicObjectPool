// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Slate/SPoolPanelBase.h"

class SSearchBox;
class ITableRow;
class STableViewBase;
class SPoolSparkline;

/**
 * Pool actor item for the actor list view
 */
struct FPoolActorItem
{
	TWeakObjectPtr<AActor> Actor;
	FString				   ActorName;
	float				   ActiveTime = 0.0f;
	bool				   bIsInUse = false;
	bool				   bIsLeaked = false;
	FString				   OwnerName;

	FPoolActorItem() = default;
	FPoolActorItem(AActor* InActor, const FString& InName, float InActiveTime, bool bInUse, bool bLeaked, const FString& InOwner)
		: Actor(InActor), ActorName(InName), ActiveTime(InActiveTime), bIsInUse(bInUse), bIsLeaked(bLeaked), OwnerName(InOwner) {}
};

/**
 * Overview panel showing detailed view of a selected pool class.
 * Features: header bar, action buttons, sparkline charts, and actor list.
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolOverviewPanel : public SPoolPanelBase
{
public:
	SLATE_BEGIN_ARGS(SPoolOverviewPanel) {}
	SLATE_END_ARGS()

	void		 Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	// ==================== UI BUILDERS ====================

	/** Build the header bar with class selector and stat badges */
	TSharedRef<SWidget> BuildHeaderBar();

	/** Build action buttons row with side stats */
	TSharedRef<SWidget> BuildActionBar();

	/** Build sparkline charts section */
	TSharedRef<SWidget> BuildChartsSection();

	/** Build summary stats row */
	TSharedRef<SWidget> BuildSummaryRow();

	/** Build the actor pool list section */
	TSharedRef<SWidget> BuildActorListSection();

	/** Build a single icon button for actions */
	TSharedRef<SWidget> BuildActionButton(const FSlateBrush* Icon, const FText& Tooltip, FOnClicked OnClicked);

	/** Build a stat badge (value + label) */
	TSharedRef<SWidget> BuildStatBadge(const TAttribute<FText>& Value, const FText& Label);

	// ==================== LIST VIEW ====================

	TSharedRef<ITableRow> OnGenerateActorRow(TSharedPtr<FPoolActorItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void				  RefreshActorList();
	void				  OnActorSearchChanged(const FText& InText);

	// ==================== CLASS SELECTION ====================

	const UClass* GetSelectedActorClass() const;

public:
	/** Called when user selects a new class - public for ObjectPoolDebugger sync */
	void OnActorClassSelected(const UClass* InClass);

private:
	// ==================== DATA GETTERS ====================

	// Header stats
	FText		GetHealthStatusText() const;
	FSlateColor GetHealthStatusColor() const;
	FText		GetActorsInUseText() const;
	FText		GetActorsAvailableText() const;
	FText		GetGrowthOpsText() const;
	FText		GetCacheHitText() const;
	FSlateColor GetCacheHitColor() const;

	// Side stats
	FText GetPoolShrinkInText() const;
	FText GetLastAccessedText() const;

	// Summary stats
	FText GetTotalActorsText() const;
	FText GetMaxPoolSizeText() const;
	FText GetShrinkOperationsText() const;

	// Sparkline data
	float GetCurrentCacheHitPercent() const;
	float GetCurrentUsagePercent() const;
	float GetCurrentGrowthRate() const;

	// ==================== ACTIONS ====================

	FReply OnClearPoolClicked();
	FReply OnShrinkPoolClicked();

	// ==================== STATE ====================

	// Selected class for this overview
	TSubclassOf<AActor> OverviewClass;

	// Sparklines
	TSharedPtr<SPoolSparkline> CacheHitSparkline;
	TSharedPtr<SPoolSparkline> UsageSparkline;
	TSharedPtr<SPoolSparkline> GrowthSparkline;

	// Actor list
	TArray<TSharedPtr<FPoolActorItem>>				  ActorListItems;
	TArray<TSharedPtr<FPoolActorItem>>				  FilteredActorItems;
	TSharedPtr<SListView<TSharedPtr<FPoolActorItem>>> ActorListView;
	FString											  ActorSearchText;

	// Timing
	double LastUpdateTime = 0.0;
	double LastSparklineSampleTime = 0.0;

	// Leak threshold (actors active longer than this are flagged)
	static constexpr float LeakThresholdSeconds = 30.0f;
};
