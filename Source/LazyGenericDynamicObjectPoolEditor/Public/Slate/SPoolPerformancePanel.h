// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Slate/SPoolPanelBase.h"

// Forward declarations
class ULazyDynamicObjectPoolSubsystem;

// Data item for the performance list view
struct FPoolPerformanceItem
{
	FString ClassName;
	float AvgRetrievalTime;
	int32 GrowthOps;
	int32 ShrinkOps;
	float EfficiencyScore;

	FPoolPerformanceItem(const FString& InName, float InTime, int32 InGrowth, int32 InShrink, float InEfficiency) :
		ClassName(InName), AvgRetrievalTime(InTime), GrowthOps(InGrowth), ShrinkOps(InShrink),
		EfficiencyScore(InEfficiency)
	{
	}
};

/**
 * Performance tab panel showing detailed metrics for identifying bottlenecks.
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolPerformancePanel : public SPoolPanelBase
{
public:
	SLATE_BEGIN_ARGS(SPoolPerformancePanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	// UI Builders
	TSharedRef<SWidget> BuildMetricsGrid();
	TSharedRef<SWidget> BuildMetricCard(const FText& Label, const TAttribute<FText>& Value, const FLinearColor& Color);
	TSharedRef<SWidget> BuildDetailedStats();

	// List View
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPoolPerformanceItem> Item,
										const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshPerformanceList();

	// Data Getters
	FText GetAverageRetrievalTimeText() const;
	FText GetTotalGrowthOperationsText() const;
	FText GetTotalShrinkOperationsText() const;
	FText GetMemoryEfficiencyText() const;

	// Actions
	FReply OnResetMetricsClicked();

	// State
	double LastUpdateTime = 0.0;

	// Grid Metrics
	float CachedAvgRetrievalTime = 0.0f;
	int32 CachedGrowthOps = 0;
	int32 CachedShrinkOps = 0;
	float CachedMemoryEfficiency = 0.0f;

	// List Data
	TArray<TSharedPtr<FPoolPerformanceItem>> PerformanceItems;
	TSharedPtr<SListView<TSharedPtr<FPoolPerformanceItem>>> PerformanceListView;
};
