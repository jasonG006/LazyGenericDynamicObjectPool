// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"

/**
 * Profiling statistics for Object Pool system
 *
 * Usage in Unreal Insights:
 * 1. Launch game with -trace=cpu,frame,stats
 * 2. Open Unreal Insights
 * 3. Look for "ObjectPool" stat group
 *
 * Console commands:
 * - stat ObjectPool        : Show stats on screen
 * - stat startfile         : Start recording stats
 * - stat stopfile          : Stop recording stats
 */

// Define stat group
DECLARE_STATS_GROUP(TEXT("ObjectPool"), STATGROUP_ObjectPool, STATCAT_Advanced);

// ========================================
// Operation Counters (Per Frame)
// ========================================

/** Number of actors retrieved from pool this frame */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Pool Retrievals"), STAT_PoolRetrievals, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of actors returned to pool this frame */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Pool Returns"), STAT_PoolReturns, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of pool growth operations this frame */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Pool Growths"), STAT_PoolGrowths, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of pool shrink operations this frame */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Pool Shrinks"), STAT_PoolShrinks, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Time spent activating pooled actors this frame */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Actor Activation"), STAT_PoolActorActivation, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Time spent deactivating pooled actors this frame */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Actor Deactivation"), STAT_PoolActorDeactivation, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

// ========================================
// Memory & Size Counters
// ========================================

/** Total number of actors in all pools */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Total Pooled Actors"), STAT_PoolTotalActors, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of actors currently in use */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Actors In Use"), STAT_PoolActorsInUse, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of actors available in pool */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Actors Available"), STAT_PoolActorsAvailable, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Estimated memory usage of all pools (bytes) */
DECLARE_MEMORY_STAT_EXTERN(TEXT("Pool Memory"), STAT_PoolMemoryUsage, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

// ========================================
// Performance Metrics
// ========================================

/** Average cache hit rate (0-100%) */
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("Cache Hit Rate %"), STAT_PoolCacheHitRate, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of cache misses this frame */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Cache Misses"), STAT_PoolCacheMisses, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of cache hits this frame */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Cache Hits"), STAT_PoolCacheHits, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

// ========================================
// Health Metrics
// ========================================

/** Number of pools in critical state */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Pools Critical"), STAT_PoolsCritical, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of pools in warning state */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Pools Warning"), STAT_PoolsWarning, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);

/** Number of healthy pools */
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Pools Healthy"), STAT_PoolsHealthy, STATGROUP_ObjectPool, LAZYGENERICDYNAMICOBJECTPOOL_API);
