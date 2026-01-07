// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPoolManagerHeader;
class SPoolTabButton;
class SPoolDashboardPanel;
class SPoolOverviewPanel;
class SPoolPerformancePanel;
class SPoolLeaksPanel;
class SPoolConfigPanel;
class SWidgetSwitcher;

/**
 * Tab definition for the Pool Manager UI
 */
struct FPoolManagerTab
{
	FText Label;
	const FSlateBrush* Icon;
	int32 Index;

	FPoolManagerTab(const FText& InLabel, const FSlateBrush* InIcon, int32 InIndex) :
		Label(InLabel), Icon(InIcon), Index(InIndex)
	{
	}
};

/**
 * Main object pool debugger window with tabbed interface.
 *
 * Architecture:
 * - SPoolManagerHeader: Logo, title, version, actions
 * - Tab Bar: Uses SPoolTabButton for each tab
 * - Content Area: SWidgetSwitcher with tab content panels
 *
 * Tabs:
 * 0 - Dashboard (global overview)
 * 1 - Overview (class-specific quick actions)
 * 2 - Performance (graphs & timeline)
 * 3 - Leaks (leak detection)
 * 4 - Config (settings)
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SObjectPoolDebugger : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SObjectPoolDebugger) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:
	// ==================== TAB MANAGEMENT ====================

	/** Handle tab button clicked */
	FReply OnTabClicked(int32 TabIndex);

	/** Check if a tab is currently active */
	bool IsTabActive(int32 TabIndex) const;

	/** Get leak count for badge display */
	int32 GetLeakCount() const;

	// ==================== CLASS SYNC ====================

	/** Sync the selected class between all child widgets */
	void OnClassSelectionChanged(TSubclassOf<AActor> NewClass);

	// ==================== TAB CONTENT BUILDERS ====================

	/** Build the tab bar widget */
	TSharedRef<SWidget> BuildTabBar();

	/** Build Dashboard tab content (placeholder for now) */
	TSharedRef<SWidget> BuildDashboardTab();

	/** Build Overview tab content */
	TSharedRef<SWidget> BuildOverviewTab();

	/** Build Performance tab content (placeholder for now) */
	TSharedRef<SWidget> BuildPerformanceTab();

	/** Build Leaks tab content */
	TSharedRef<SWidget> BuildLeaksTab();

	/** Build Config tab content (placeholder for now) */
	TSharedRef<SWidget> BuildConfigTab();

private:
	// ==================== WIDGET REFERENCES ====================

	/** Header widget */
	TSharedPtr<SPoolManagerHeader> Header;

	/** Widget switcher for tab content */
	TSharedPtr<SWidgetSwitcher> TabContentSwitcher;

	/** Child panel references (for class sync) */
	TSharedPtr<SPoolDashboardPanel> DashboardPanel;
	TSharedPtr<SPoolOverviewPanel> OverviewPanel;
	TSharedPtr<SPoolPerformancePanel> PerformancePanel;
	TSharedPtr<SPoolLeaksPanel> LeaksPanel;
	TSharedPtr<SPoolConfigPanel> ConfigPanel;

	// ==================== STATE ====================

	/** Current active tab index */
	int32 ActiveTabIndex = 0;

	/** Tab definitions */
	TArray<FPoolManagerTab> Tabs;

	/** Cached leak count for badge */
	mutable int32 CachedLeakCount = 0;
};
