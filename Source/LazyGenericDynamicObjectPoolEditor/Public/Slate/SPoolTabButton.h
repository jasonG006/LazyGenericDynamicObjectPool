// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Reusable tab button widget for the Pool Manager UI.
 * Supports: icon, text label, badge count, active/inactive state.
 *
 * Usage:
 * SNew(SPoolTabButton)
 *     .TabIndex(0)
 *     .Label(LOCTEXT("DashboardTab", "Dashboard"))
 *     .Icon(FAppStyle::GetBrush("Icons.Details"))
 *     .BadgeCount(0)  // 0 = no badge
 *     .IsActive(this, &MyWidget::IsTabActive, 0)
 *     .OnTabClicked(this, &MyWidget::OnTabClicked)
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolTabButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPoolTabButton) : _TabIndex(0), _BadgeCount(0), _IsActive(false) {}

	/** Unique index for this tab */
	SLATE_ARGUMENT(int32, TabIndex)

	/** Text label for the tab */
	SLATE_ATTRIBUTE(FText, Label)

	/** Optional icon brush */
	SLATE_ARGUMENT(const FSlateBrush*, Icon)

	/** Badge count (0 = no badge shown) */
	SLATE_ATTRIBUTE(int32, BadgeCount)

	/** Whether this tab is currently active */
	SLATE_ATTRIBUTE(bool, IsActive)

	/** Callback when tab is clicked */
	SLATE_EVENT(FOnClicked, OnTabClicked)

	SLATE_END_ARGS()

	/** Constructs this widget */
	void Construct(const FArguments& InArgs);

private:
	/** Handle button click */
	FReply HandleButtonClicked();

	/** Get text color based on active state */
	FSlateColor GetTextColor() const;

	/** Get icon color based on active state */
	FSlateColor GetIconColor() const;

	/** Get underline visibility based on active state */
	EVisibility GetUnderlineVisibility() const;

	/** Get badge visibility based on count */
	EVisibility GetBadgeVisibility() const;

	/** Get badge text */
	FText GetBadgeText() const;

	/** Get background color based on active/hover state */
	FSlateColor GetBackgroundColor() const;

private:
	int32 TabIndex;
	TAttribute<FText> Label;
	const FSlateBrush* IconBrush;
	TAttribute<int32> BadgeCount;
	TAttribute<bool> IsActive;
	FOnClicked OnTabClicked;

	/** Track hover state for styling */
	bool bIsHovered = false;
};
