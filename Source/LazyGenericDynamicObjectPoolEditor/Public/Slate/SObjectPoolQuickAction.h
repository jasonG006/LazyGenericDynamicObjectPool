// // Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SObjectPoolQuickAction : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SObjectPoolQuickAction)
	{} // constructor

	SLATE_ATTRIBUTE(const TSubclassOf<AActor>*, SelectedActorClass)

	SLATE_END_ARGS()

	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	float TopLevelContentPadding = 12.0f;
	float MidLevelContentPadding = 12.0f;
	float BottomLevelContentPadding = 12.0f;

	FSlateColor ButtonColor = FSlateColor(FColor::Transparent);

	void OnActorClassSelected(const UClass* Class);
	UClass* GetSelectedActorClass();
	FText GetNextPoolShrinkTime() const;
	FText GetLastAccessTime() const;
	FText GetTotalActorText() const;
	FText GetMaxPoolSizeText() const;
	FText GetShrinkOperationsText() const;
	FText GetActorsOfClassInUse() const;
	FText GetActorsOfClassInPool() const;
	FText GetPoolGrowthOperation() const;
	FSlateColor GetButtonColor() const { return ButtonColor; };
	void SetButtonHoverColor();
	void ResetButtonColor();
	TOptional<float> GetTotalPoolSizeRatio() const;

	// Health status visualization
	FText GetHealthStatusText() const;
	FSlateColor GetHealthStatusColor() const;
	const FSlateBrush* GetHealthStatusIcon() const;
	EVisibility GetHealthWarningVisibility() const;
	FText GetHealthWarningText() const;

	// Cache hit rate
	FText GetCacheHitRateText() const;
	FSlateColor GetCacheHitRateColor() const;

	// Leak detection
	FText GetLeakCountText() const;
	FSlateColor GetLeakBadgeColor() const;
	EVisibility GetLeakBadgeVisibility() const;
	FReply OnScanForLeaksClicked();
	FReply OnForceReturnLeaksClicked();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	FReply OnClearAllPoolClicked();
	FReply OnShrinkAllPoolClicked();
	FReply OnPrewarmPoolClicked();
	FReply OnOptimizePoolClicked();

	/** Update sparkline data (called by ticker) */
	void UpdateSparklineData();

private:
	TSubclassOf<AActor> SelectedClass;

	// Sparkline widgets for performance visualization
	TSharedPtr<class SPoolSparkline> CacheHitRateSparkline;
	TSharedPtr<class SPoolSparkline> PoolUsageSparkline;
	TSharedPtr<class SPoolSparkline> GrowthOpsSparkline;

	// Ticker for updating sparklines
	double LastSparklineUpdate = 0.0;

	// Leak detection
	int32 CachedLeakCount = 0;
	double LastLeakScanTime = 0.0;
};
