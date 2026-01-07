// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ULazyDynamicObjectPoolSubsystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPoolClassChanged, TSubclassOf<AActor>);

/**
 * Base class for all Pool Manager editor panels.
 * Provides common functionality for subsystem access, PIE detection, and class selection.
 *
 * This eliminates repeated boilerplate code across:
 * - SObjectPoolQuickAction
 * - SPoolLeakDetector
 * - SObjectPoolSearchAction
 * - Future panels (Dashboard, Performance, Config)
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolPanelBase : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPoolPanelBase) {}
	/** Optional: Initial class to select */
	SLATE_ARGUMENT(TSubclassOf<AActor>, InitialClass)
	SLATE_END_ARGS()

	/** Constructs this widget */
	void Construct(const FArguments& InArgs);

	// ==================== CLASS SELECTION ====================

	/** Get the currently selected actor class */
	TSubclassOf<AActor> GetSelectedClass() const { return SelectedClass; }

	/** Set the selected actor class (triggers OnClassChanged) */
	virtual void SetSelectedClass(TSubclassOf<AActor> NewClass);

	/** Called when class selection changes (from external source like parent) */
	virtual void OnActorClassSelected(const UClass* Class);

	/** Get selected class for property binding */
	UClass* GetSelectedActorClass() const;

	// ==================== DELEGATE ====================

	/** Delegate broadcast when class selection changes */
	FOnPoolClassChanged OnClassChangedDelegate;

protected:
	// ==================== SUBSYSTEM ACCESS ====================

	/**
	 * Get the pool subsystem from the current PIE world.
	 * Returns nullptr if not in PIE or subsystem unavailable.
	 * This eliminates the repeated null-check boilerplate across panels.
	 */
	ULazyDynamicObjectPoolSubsystem* GetPoolSubsystem() const;

	/** Get the current PIE world (if any) */
	UWorld* GetPlayWorld() const;

	/** Check if currently in a PIE session */
	bool IsInPIE() const;

	// ==================== COMMON UI HELPERS ====================

	/** Format a duration in seconds to a human-readable string (e.g., "5.2s" or "1m 30s") */
	FText FormatDuration(float Seconds) const;

	/** Get plugin version string from the .uplugin file */
	static FString GetPluginVersion();

	/** Get plugin version with PIE status (e.g., "v1.0 • PIE Active") */
	FText GetVersionWithStatus() const;

protected:
	/** Currently selected actor class */
	TSubclassOf<AActor> SelectedClass;

	/** Cached plugin version (loaded once) */
	static FString CachedPluginVersion;
};
