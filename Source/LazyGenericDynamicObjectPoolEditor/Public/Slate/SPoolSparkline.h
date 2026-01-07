// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

/**
 * A simple sparkline widget for displaying mini performance graphs
 * Shows historical data as a compact line chart
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolSparkline : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SPoolSparkline)
		: _LineColor(FLinearColor::White)
		, _FillColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.2f))
		, _LineThickness(1.5f)
		, _MaxDataPoints(60)
	{}

	/** Color of the line */
	SLATE_ATTRIBUTE(FLinearColor, LineColor)

	/** Color of the fill area under the line */
	SLATE_ATTRIBUTE(FLinearColor, FillColor)

	/** Thickness of the line */
	SLATE_ATTRIBUTE(float, LineThickness)

	/** Maximum number of data points to display */
	SLATE_ATTRIBUTE(int32, MaxDataPoints)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Add a new data point to the sparkline */
	void AddDataPoint(float Value);

	/** Clear all data points */
	void ClearData();

	/** Set all data points at once */
	void SetData(const TArray<float>& NewData);

	// SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	/** Historical data points */
	TArray<float> DataPoints;

	/** Attributes */
	TAttribute<FLinearColor> LineColor;
	TAttribute<FLinearColor> FillColor;
	TAttribute<float> LineThickness;
	TAttribute<int32> MaxDataPoints;

	/** Get min/max values from data for normalization */
	void GetDataRange(float& OutMin, float& OutMax) const;
};
