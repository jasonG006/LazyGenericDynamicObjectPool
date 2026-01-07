// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/ObjectPoolDebugger.h"
#include "Editor.h"
#include "PoolManagerStyles.h"
#include "Slate/ObjectPoolDebugger.h"
#include "Slate/ObjectPoolSearchAction.h"
#include "Slate/SObjectPoolQuickAction.h"
#include "Slate/SPoolConfigPanel.h"
#include "Slate/SPoolDashboardPanel.h"
#include "Slate/SPoolLeaksPanel.h"
#include "Slate/SPoolManagerHeader.h"
#include "Slate/SPoolOverviewPanel.h"
#include "Slate/SPoolPerformancePanel.h"
#include "Slate/SPoolTabButton.h"
#include "SlateOptMacros.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
#define LOCTEXT_NAMESPACE "SObjectPoolDebugger"

void SObjectPoolDebugger::Construct(const FArguments& InArgs)
{
	// Initialize tab definitions
	Tabs.Empty();
	Tabs.Add(FPoolManagerTab(LOCTEXT("DashboardTab", "Dashboard"), FAppStyle::GetBrush("Icons.Blueprints"), 0));
	Tabs.Add(FPoolManagerTab(LOCTEXT("OverviewTab", "Overview"), FAppStyle::GetBrush("Icons.Details"), 1));
	Tabs.Add(FPoolManagerTab(LOCTEXT("PerformanceTab", "Performance"), FAppStyle::GetBrush("Icons.Visibility"), 2));
	Tabs.Add(FPoolManagerTab(LOCTEXT("LeaksTab", "Leaks"), FAppStyle::GetBrush("Icons.Warning"), 3));
	Tabs.Add(FPoolManagerTab(LOCTEXT("ConfigTab", "Config"), FAppStyle::GetBrush("Icons.Settings"), 4));

	// UI is always accessible - individual panels handle PIE state gracefully
	ChildSlot[SNew(SVerticalBox)

		// ==================== HEADER ====================
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(Header, SPoolManagerHeader)
		]

		// ==================== TAB BAR ====================
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildTabBar()
		]

		// ==================== TAB CONTENT ====================
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SAssignNew(TabContentSwitcher, SWidgetSwitcher)
			.WidgetIndex_Lambda([this]() { return ActiveTabIndex; })

			// Tab 0: Dashboard
			+ SWidgetSwitcher::Slot()
			[
				BuildDashboardTab()
			]

			// Tab 1: Overview
			+ SWidgetSwitcher::Slot()
			[
				BuildOverviewTab()
			]

			// Tab 2: Performance
			+ SWidgetSwitcher::Slot()
			[
				BuildPerformanceTab()
			]

			// Tab 3: Leaks
			+ SWidgetSwitcher::Slot()
			[
				BuildLeaksTab()
			]

			// Tab 4: Config
			+ SWidgetSwitcher::Slot()
			[
				BuildConfigTab()
			]
		]];

	// Bind selection delegates
	if (DashboardPanel.IsValid())
	{
		DashboardPanel->OnClassChangedDelegate.AddSP(this, &SObjectPoolDebugger::OnClassSelectionChanged);
	}
	if (OverviewPanel.IsValid())
	{
		OverviewPanel->OnClassChangedDelegate.AddSP(this, &SObjectPoolDebugger::OnClassSelectionChanged);
	}
	if (PerformancePanel.IsValid())
	{
		PerformancePanel->OnClassChangedDelegate.AddSP(this, &SObjectPoolDebugger::OnClassSelectionChanged);
	}
	if (LeaksPanel.IsValid())
	{
		LeaksPanel->OnClassChangedDelegate.AddSP(this, &SObjectPoolDebugger::OnClassSelectionChanged);
	}
	if (ConfigPanel.IsValid())
	{
		ConfigPanel->OnClassChangedDelegate.AddSP(this, &SObjectPoolDebugger::OnClassSelectionChanged);
	}
}

// ==================== TAB MANAGEMENT ====================

FReply SObjectPoolDebugger::OnTabClicked(int32 TabIndex)
{
	ActiveTabIndex = TabIndex;
	return FReply::Handled();
}

bool SObjectPoolDebugger::IsTabActive(int32 TabIndex) const { return ActiveTabIndex == TabIndex; }

int32 SObjectPoolDebugger::GetLeakCount() const
{
	// TODO: Get actual leak count from SPoolLeakDetector or subsystem
	return CachedLeakCount;
}

void SObjectPoolDebugger::OnClassSelectionChanged(TSubclassOf<AActor> NewClass)
{
	if (DashboardPanel.IsValid()) { DashboardPanel->OnActorClassSelected(NewClass); }
	if (OverviewPanel.IsValid()) { OverviewPanel->OnActorClassSelected(NewClass); }
	if (PerformancePanel.IsValid()) { PerformancePanel->OnActorClassSelected(NewClass); }
	if (LeaksPanel.IsValid()) { LeaksPanel->OnActorClassSelected(NewClass); }
	if (ConfigPanel.IsValid()) { ConfigPanel->OnActorClassSelected(NewClass); }
}

// ==================== TAB BAR BUILDER ====================

TSharedRef<SWidget> SObjectPoolDebugger::BuildTabBar()
{
	const TSharedRef<SHorizontalBox> TabBar = SNew(SHorizontalBox);

	for (const FPoolManagerTab& Tab : Tabs)
	{
		TabBar->AddSlot()
		      .AutoWidth()
		      .Padding(Tab.Index == 0 ? 0.0f : 2.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SPoolTabButton)
			.TabIndex(Tab.Index)
			.Label(Tab.Label)
			.Icon(Tab.Icon)
			.BadgeCount_Lambda
			(
				[this, TabIndex = Tab.Index]() -> int32
				{
					// Show badge only on Leaks tab
					if (TabIndex == 3) { return GetLeakCount(); }
					return 0;
				}
			)
			.IsActive_Lambda([this, TabIndex = Tab.Index]() -> bool { return IsTabActive(TabIndex); })
			.OnTabClicked_Lambda([this, TabIndex = Tab.Index]() -> FReply { return OnTabClicked(TabIndex); })
		];
	}

	return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FPoolManagerStyles::GetWhiteBrush())
				.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
				.Padding(FMargin(FPoolManagerStyles::PaddingNone))
				[
					TabBar
				]
			]

			// Bottom Separator Line
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SImage)
					.Image(FPoolManagerStyles::GetWhiteBrush())
					.ColorAndOpacity(FPoolManagerStyles::GetBorderColor())
				]
			];
}

// ==================== TAB CONTENT BUILDERS ====================

TSharedRef<SWidget> SObjectPoolDebugger::BuildDashboardTab() { return SAssignNew(DashboardPanel, SPoolDashboardPanel); }

TSharedRef<SWidget> SObjectPoolDebugger::BuildOverviewTab() { return SAssignNew(OverviewPanel, SPoolOverviewPanel); }

TSharedRef<SWidget> SObjectPoolDebugger::BuildPerformanceTab()
{
	return SAssignNew(PerformancePanel, SPoolPerformancePanel);
}

TSharedRef<SWidget> SObjectPoolDebugger::BuildLeaksTab() { return SAssignNew(LeaksPanel, SPoolLeaksPanel); }

TSharedRef<SWidget> SObjectPoolDebugger::BuildConfigTab() { return SAssignNew(ConfigPanel, SPoolConfigPanel); }

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
