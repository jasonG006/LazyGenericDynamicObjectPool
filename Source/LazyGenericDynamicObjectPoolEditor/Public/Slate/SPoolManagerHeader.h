// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Premium header widget for the Pool Manager UI.
 *
 * Displays:
 * - Logo icon with gradient background
 * - "OBJECT POOL MANAGER" title
 * - Version string (from plugin descriptor) + PIE status
 * - Help button (opens info dialog)
 *
 * Visual reference: Matches React prototype header design
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolManagerHeader : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPoolManagerHeader) {}
	SLATE_END_ARGS()

	/** Constructs this widget */
	void Construct(const FArguments& InArgs);

	/** Updates PIE status dynamically */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** Get version text with PIE status */
	FText GetVersionText() const;

	/** Get PIE indicator color (green = active, gray = inactive) */
	FSlateColor GetPIEIndicatorColor() const;

	/** Handle help button click */
	FReply OnHelpButtonClicked();

	/** Get plugin version from descriptor */
	static FString GetPluginVersion();

	/** Check if in PIE */
	bool IsInPIE() const;

private:
	/** Cached plugin version */
	static FString CachedVersion;

	/** Track PIE state for status updates */
	bool bWasInPIE = false;
};
