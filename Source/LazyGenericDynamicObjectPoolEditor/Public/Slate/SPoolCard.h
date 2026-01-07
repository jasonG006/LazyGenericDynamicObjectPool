// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ULazyDynamicObjectPoolSubsystem;

/**
 * A self-contained pool card widget that displays live pool statistics.
 * Uses TAttribute bindings for automatic data updates without rebuilding the widget tree.
 *
 * Usage:
 *   SNew(SPoolCard)
 *       .PoolClass(MyActorClass)
 *       .Subsystem(GetPoolSubsystem())
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPoolCard) : _CardWidth(260.0f), _CardHeight(130.0f) {}
	/** The actor class this pool manages */
	SLATE_ARGUMENT(TSubclassOf<AActor>, PoolClass)

	/** Weak reference to the pool subsystem */
	SLATE_ARGUMENT(TWeakObjectPtr<ULazyDynamicObjectPoolSubsystem>, Subsystem)

	/** Optional: Override default card width (default: 260px) */
	SLATE_ARGUMENT(float, CardWidth)

	/** Optional: Override default card height (default: 130px) */
	SLATE_ARGUMENT(float, CardHeight)
	SLATE_END_ARGS()

	/** Constructs this widget */
	void Construct(const FArguments& InArgs);

	// ==================== LIVE DATA GETTERS (for TAttribute bindings) ====================

	/** Get cleaned class name (removes _C suffix) */
	FText GetClassName() const;

	/** Get usage text (e.g., "25/100") */
	FText GetUsageText() const;

	/** Get usage percentage (0.0-1.0) wrapped in TOptional for SProgressBar */
	TOptional<float> GetUsagePercent() const;

	/** Get cache hit text (e.g., "85%") */
	FText GetCacheHitText() const;

	/** Get cache hit color based on percentage */
	FSlateColor GetCacheHitColor() const;

	/** Get memory usage text (e.g., "1.5MB") */
	FText GetMemoryText() const;

	/** Get status color based on usage */
	FSlateColor GetStatusColor() const;

private:
	/** The actor class this card represents */
	TSubclassOf<AActor> PoolClass;

	/** Weak reference to the pool subsystem (avoids holding subsystem alive) */
	TWeakObjectPtr<ULazyDynamicObjectPoolSubsystem> CachedSubsystem;

	/** Card dimensions */
	float CardWidth = 260.0f;
	float CardHeight = 130.0f;

	// ==================== INTERNAL HELPERS ====================

	/** Safely get subsystem (returns nullptr if invalid) */
	ULazyDynamicObjectPoolSubsystem* GetSubsystem() const;

	/** Calculate usage percentage */
	float CalculateUsagePercent() const;

	/** Calculate cache hit rate */
	float CalculateCacheHitRate() const;
};
