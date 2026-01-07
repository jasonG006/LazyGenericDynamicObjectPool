// // Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Loading/PrewarmTask.h"
#include "LazyDynamicObjectPoolSettings.generated.h"


/**
 * @class ULazyDynamicObjectPoolSettings
 * @brief Configuration settings for the Lazy Dynamic Object Pool system.
 *
 * This class defines the configurable parameters for the Lazy Dynamic Object Pool plugin.
 * It allows developers to fine-tune the behavior of the object pool system through the
 * Project Settings in the Unreal Editor.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Lazy Dynamic Object Pool"))
class LAZYGENERICDYNAMICOBJECTPOOL_API ULazyDynamicObjectPoolSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** @brief Default constructor */
	ULazyDynamicObjectPoolSettings(const FObjectInitializer& ObjectInitializer);

	/**
	 * @brief The default initial size for new object pools.
	 * @note This setting determines how many objects are pre-instantiated when a new pool is created.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Pool Configuration", meta = (ClampMin = "0", UIMin = "0"))
	int32 DefaultInitialPoolSize = 10;

	/**
	 * @brief The maximum size limit for object pools.
	 * @note Pools will not grow beyond this size. Set to 0 for unlimited growth.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Pool Configuration", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxPoolSize = 1000;

	/**
	 * @brief The growth factor for pools when they need to expand.
	 * @note When a pool needs more objects, it will grow by this percentage of its current size.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Pool Configuration",
		meta = (ClampMin = "1.0", UIMin = "1.0", ClampMax = "2.0", UIMax = "2.0"))
	float PoolGrowthFactor = 1.5f;

	/**
	 * @brief Whether to enable automatic pool shrinking.
	 * @note If true, pools will periodically remove excess unused objects.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Pool Optimization")
	bool bEnableAutoShrink = true;

	/**
	 * @brief The interval in seconds between automatic pool shrink operations.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Pool Optimization",
		meta = (EditCondition = "bEnableAutoShrink", ClampMin = "1.0", UIMin = "1.0"))
	float AutoShrinkInterval = 60.0f;

	/**
	 * @brief The threshold of unused objects (as a percentage of pool size) that triggers shrinking.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Pool Optimization",
		meta = (EditCondition = "bEnableAutoShrink", ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0",
			UIMax = "1.0"))
	float ShrinkThreshold = 0.25f;

	// ========================================
	// Loading Integration Settings
	// ========================================

	/**
	 * @brief Enable integration with loading screen for pool prewarming.
	 * @note When enabled, pools will be prewarmed during level load while the loading screen is visible.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Loading Integration")
	bool bEnableLoadingIntegration = true;

	/**
	 * @brief Global list of actor classes to prewarm during level transitions.
	 * @note These are applied to all levels unless overridden by a per-level data asset.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Loading Integration",
		meta = (EditCondition = "bEnableLoadingIntegration"))
	TArray<FPoolPrewarmConfig> GlobalPrewarmConfigs;

	/**
	 * @brief Maximum time in milliseconds to spend per frame on prewarming.
	 * @note Lower values = smoother loading animations but longer total load time.
	 *       Set to 0 for no limit (spawn all in one frame).
	 */
	UPROPERTY(config, EditAnywhere, Category = "Loading Integration",
		meta = (EditCondition = "bEnableLoadingIntegration", ClampMin = "0.0", UIMin = "0.0", ClampMax = "50.0",
			UIMax = "50.0"))
	float PrewarmFrameBudgetMs = 8.0f;

	/**
	 * @brief Whether to wait for a manual start signal instead of auto-starting.
	 * @note Enable this to manually trigger prewarming from your GameMode/GameInstance code.
	 *       This is the most robust way to ensure the loading screen is visible before spawning starts.
	 *       Calls to StartProcessing() will be ignored until OnManualStart() is called.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Loading Integration",
		meta = (EditCondition = "bEnableLoadingIntegration"))
	bool bWaitForManualStart = false;

	UPROPERTY(config, EditAnywhere, Category = "Loading Integration",
		meta = (EditCondition = "bEnableLoadingIntegration && bWaitForManualStart", ClampMin = "0", UIMin = "0",
			ClampMax = "10", UIMax = "10"))
	int32 PrewarmStartDelayFrames = 2;

	/**
	 * @brief Whether to attempt prewarming on all levels by default.
	 * @note If true, use ExcludedLevels to prevent prewarming on specific maps.
	 *       If false, use IncludedLevels to explicitly enable prewarming on specific maps.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Loading Integration|Level Filtering",
		meta = (EditCondition = "bEnableLoadingIntegration"))
	bool bPrewarmAllLevelsByDefault = true;

	/**
	 * @brief List of levels to EXCLUDE from prewarming (if bPrewarmAllLevelsByDefault is true).
	 * @note Useful for Menus, Splash Screens, or lightweight levels.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Loading Integration|Level Filtering",
		meta = (EditCondition = "bEnableLoadingIntegration && bPrewarmAllLevelsByDefault",
			AllowedClasses = "/Script/Engine.World"))
	TArray<FSoftObjectPath> ExcludedLevels;

	/**
	 * @brief List of levels to INCLUDE in prewarming (if bPrewarmAllLevelsByDefault is false).
	 * @note Only these levels will trigger prewarming.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Loading Integration|Level Filtering",
		meta = (EditCondition = "bEnableLoadingIntegration && !bPrewarmAllLevelsByDefault",
			AllowedClasses = "/Script/Engine.World"))
	TArray<FSoftObjectPath> IncludedLevels;

	// ========================================
	// Debugging Settings
	// ========================================

	/**
	 * @brief Whether to log detailed information about pool operations.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Debugging")
	bool bEnableDetailedLogging = false;

	/**
	 * @brief Whether to log prewarm progress during level load.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Debugging", meta = (EditCondition = "bEnableLoadingIntegration"))
	bool bLogPrewarmProgress = false;
};
