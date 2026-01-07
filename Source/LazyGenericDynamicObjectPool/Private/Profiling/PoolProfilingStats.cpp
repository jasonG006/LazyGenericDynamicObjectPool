// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Profiling/PoolProfilingStats.h"

// ========================================
// Operation Counters (Per Frame)
// ========================================

DEFINE_STAT(STAT_PoolRetrievals);
DEFINE_STAT(STAT_PoolReturns);
DEFINE_STAT(STAT_PoolGrowths);
DEFINE_STAT(STAT_PoolShrinks);
DEFINE_STAT(STAT_PoolActorActivation);
DEFINE_STAT(STAT_PoolActorDeactivation);

// ========================================
// Memory & Size Counters
// ========================================

DEFINE_STAT(STAT_PoolTotalActors);
DEFINE_STAT(STAT_PoolActorsInUse);
DEFINE_STAT(STAT_PoolActorsAvailable);
DEFINE_STAT(STAT_PoolMemoryUsage);

// ========================================
// Performance Metrics
// ========================================

DEFINE_STAT(STAT_PoolCacheHitRate);
DEFINE_STAT(STAT_PoolCacheMisses);
DEFINE_STAT(STAT_PoolCacheHits);

// ========================================
// Health Metrics
// ========================================

DEFINE_STAT(STAT_PoolsCritical);
DEFINE_STAT(STAT_PoolsWarning);
DEFINE_STAT(STAT_PoolsHealthy);
