// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "PrewarmTask.generated.h"

/**
 * Configuration for prewarming a specific actor class during level load.
 * Used in both global settings and per-level data assets.
 */
USTRUCT(BlueprintType)
struct LAZYGENERICDYNAMICOBJECTPOOL_API FPoolPrewarmConfig
{
	GENERATED_BODY()

	/** The actor class to prewarm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm")
	TSubclassOf<AActor> ActorClass;

	/** Number of actors to prewarm for this class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm", meta = (ClampMin = "1", UIMin = "1"))
	int32 PrewarmCount = 10;

	/**
	 * Priority for processing order. Higher values are processed first.
	 * Use for dependency ordering (e.g., enemies before projectiles).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm")
	int32 Priority = 0;

	/** Optional description for designer reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prewarm")
	FString Description;

	FPoolPrewarmConfig() = default;

	FPoolPrewarmConfig(TSubclassOf<AActor> InClass, int32 InCount, int32 InPriority = 0) :
		ActorClass(InClass), PrewarmCount(InCount), Priority(InPriority)
	{
	}

	bool IsValid() const { return ActorClass != nullptr && PrewarmCount > 0; }
};

/**
 * Internal task representation for the prewarm queue.
 * Tracks progress through a single prewarm operation.
 */
USTRUCT()
struct FPrewarmTask
{
	GENERATED_BODY()

	/** Class being prewarmed */
	TSubclassOf<AActor> ActorClass;

	/** Actors remaining to spawn */
	int32 RemainingCount = 0;

	/** Original total for progress calculation */
	int32 TotalCount = 0;

	/** Processing priority */
	int32 Priority = 0;

	/** Optional custom task delegate (for extensibility) */
	TFunction<void()> CustomTask;

	FPrewarmTask() = default;

	FPrewarmTask(const FPoolPrewarmConfig& Config) :
		ActorClass(Config.ActorClass), RemainingCount(Config.PrewarmCount), TotalCount(Config.PrewarmCount),
		Priority(Config.Priority)
	{
	}

	bool IsComplete() const { return RemainingCount <= 0; }

	float GetProgress() const
	{
		return TotalCount > 0 ? 1.0f - (static_cast<float>(RemainingCount) / TotalCount) : 1.0f;
	}
};

/**
 * Comparison functor for priority queue ordering.
 * Higher priority tasks are processed first.
 */
struct FPrewarmTaskPrioritySort
{
	bool operator()(const FPrewarmTask& A, const FPrewarmTask& B) const { return A.Priority > B.Priority; }
};
