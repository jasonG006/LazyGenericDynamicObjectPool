// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * A reusable section header widget with icon, title, separator, and optional action slot.
 *
 * Usage:
 *   SNew(SSectionHeader)
 *       .Title(LOCTEXT("AllPoolsTitle", "ALL POOLS"))
 *       .Icon(FAppStyle::GetBrush("Icons.Layout"))
 *       .IconColor(FPoolManagerStyles::GetAccentOrange())
 *       .ActionContent()
 *       [
 *           SNew(SButton)...
 *       ]
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SSectionHeader : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSectionHeader) : _IconColor(FLinearColor::White), _ShowSeparator(true) {}
	/** Section title text */
	SLATE_ARGUMENT(FText, Title)

	/** Icon brush (from FAppStyle or custom) */
	SLATE_ARGUMENT(const FSlateBrush*, Icon)

	/** Icon tint color */
	SLATE_ARGUMENT(FLinearColor, IconColor)

	/** Whether to show bottom separator line */
	SLATE_ARGUMENT(bool, ShowSeparator)

	/** Optional action content (e.g., a button) on the right side */
	SLATE_NAMED_SLOT(FArguments, ActionContent)
	SLATE_END_ARGS()

	/** Constructs this widget */
	void Construct(const FArguments& InArgs);
};
