// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Slate/SPoolPanelBase.h"

class ULazyDynamicObjectPoolSubsystem;

struct FPoolActivityItem
{
	FText		 Message;
	FDateTime	 Timestamp;
	FLinearColor Color;

	FPoolActivityItem(const FText& InMessage, const FLinearColor& InColor) : Message(InMessage), Timestamp(FDateTime::Now()), Color(InColor)
	{
	}
};

/**
 * Dashboard tab panel showing global pool statistics.
 *
 * Displays:
 * - Global Stats: Active Pools, Total Actors, In Use, Avg Cache Hit, Memory, Potential Leaks
 * - Pool Health Grid: All pools with usage bars and health indicators
 * - Quick Actions: Prewarm All, Shrink Unused, Scan Leaks
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolDashboardPanel : public SPoolPanelBase
{
public:
	SLATE_BEGIN_ARGS(SPoolDashboardPanel) {}
	SLATE_END_ARGS()

	/** Constructs this widget */
	void Construct(const FArguments& InArgs);

	/** Tick for updating stats */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	// ==================== UI BUILDERS ====================

	/** Build the global stats row */
	TSharedRef<SWidget> BuildGlobalStatsSection();

	/** Build the main split-view content */
	TSharedRef<SWidget> BuildMainContent();

	/** Build the "All Pools" grid section */
	TSharedRef<SWidget> BuildAllPoolsSection();

	/** Build the activity feed sidebar */

	/** Build a single stat card with static text */
	TSharedRef<SWidget> BuildStatCard(const FText& Value, const FText& Label,
		const FLinearColor& ValueColor = FLinearColor::White);

	/** Build a single stat card with dynamic (attribute-bound) text */
	TSharedRef<SWidget> BuildStatCardWidget(const TAttribute<FText>& Value, const FText& Label,
		const FLinearColor& ValueColor = FLinearColor::White);

	/** Build the pool health grid (Legacy/Refactored into BuildAllPoolsSection) */
	TSharedRef<SWidget> BuildPoolHealthGrid();

	/** Build the quick actions panel */

	/** Build "No PIE" info banner */
	TSharedRef<SWidget> BuildNoPIEBanner();

	// ==================== DATA GETTERS ====================

	/** Get total number of active pools */
	FText GetActivePoolsText() const;

	/** Get total actors across all pools */
	FText GetTotalActorsText() const;

	/** Get total actors in use */
	FText GetActorsInUseText() const;

	/** Get average cache hit rate */
	FText GetAvgCacheHitText() const;

	/** Get cache hit color based on value */
	FSlateColor GetAvgCacheHitColor() const;

	/** Get estimated memory usage */
	FText GetMemoryUsageText() const;

	/** Get potential leak count */
	FText GetLeakCountText() const;

	/** Check if we should show the "No PIE" banner */
	EVisibility GetNoPIEBannerVisibility() const;

	/** Check if we should show the stats content */
	EVisibility GetStatsContentVisibility() const;

	// ==================== ACTIONS ====================

	FReply OnPrewarmAllClicked();
	FReply OnShrinkUnusedClicked();
	FReply OnScanLeaksClicked();

	/** Add an entry to the activity log */
	/** Add an entry to the activity log */
	void LogActivity(const FText& Message, const FLinearColor& Color);

private:
	/** Refresh the 'All Pools' grid content */
	void RefreshPoolList();

private:
	/** Last update time for throttling */
	double LastUpdateTime = 0.0;

	/** Cached stat values (updated periodically) */
	int32 CachedActivePoolCount = 0;
	int32 CachedTotalActors = 0;
	int32 CachedActorsInUse = 0;
	float CachedAvgCacheHit = 0.0f;
	float CachedMemoryMB = 0.0f;
	int32 CachedLeakCount = 0;

	/** Local activity log for prototype */
	/** Local activity log for prototype */
	TArray<FPoolActivityItem> ActivityLog;

	/** Dynamic content box for All Pools grid */
	TSharedPtr<SBox> AllPoolsContentBox;

	/** Cached PIE state to trigger refreshes */
	bool  bCachedIsPIE = false;
	int32 LastPoolCount = -1;
};
