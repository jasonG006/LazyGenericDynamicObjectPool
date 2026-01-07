// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Loading/PrewarmTask.h"
#include "PoolPrewarmDataAsset.generated.h"

/**
 * Data asset for per-level pool prewarm configuration.
 * Reference this from level blueprints or world settings for level-specific prewarming.
 */
UCLASS(BlueprintType)
class LAZYGENERICDYNAMICOBJECTPOOL_API UPoolPrewarmDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** List of actor classes to prewarm when this config is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm Configuration")
	TArray<FPoolPrewarmConfig> PrewarmConfigs;

	/**
	 * If true, this config replaces global settings entirely.
	 * If false, this config is merged with global settings (this config takes priority for duplicates).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm Configuration")
	bool bOverrideGlobalConfig = false;

	/** Optional override for frame budget (ms). 0 = use global setting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm Configuration",
			  meta = (ClampMin = "0", UIMin = "0"))
	float FrameBudgetOverride = 0.0f;

	/** Get all valid prewarm configs sorted by priority */
	UFUNCTION(BlueprintCallable, Category = "Prewarm Configuration")
	TArray<FPoolPrewarmConfig> GetSortedConfigs() const
	{
		TArray<FPoolPrewarmConfig> ValidConfigs;
		for (const FPoolPrewarmConfig& Config : PrewarmConfigs)
		{
			if (Config.IsValid()) { ValidConfigs.Add(Config); }
		}

		// Sort by priority (higher first)
		ValidConfigs.Sort([](const FPoolPrewarmConfig& A, const FPoolPrewarmConfig& B)
						  { return A.Priority > B.Priority; });

		return ValidConfigs;
	}
};
