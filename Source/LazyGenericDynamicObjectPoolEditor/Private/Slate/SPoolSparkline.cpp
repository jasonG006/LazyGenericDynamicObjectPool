// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolSparkline.h"
#include "Rendering/DrawElements.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPoolSparkline::Construct(const FArguments& InArgs)
{
	LineColor = InArgs._LineColor;
	FillColor = InArgs._FillColor;
	LineThickness = InArgs._LineThickness;
	MaxDataPoints = InArgs._MaxDataPoints;
}

void SPoolSparkline::AddDataPoint(float Value)
{
	DataPoints.Add(Value);

	// Keep only the max number of data points
	const int32 MaxPoints = MaxDataPoints.Get();
	while (DataPoints.Num() > MaxPoints)
	{
		DataPoints.RemoveAt(0);
	}
}

void SPoolSparkline::ClearData()
{
	DataPoints.Empty();
}

void SPoolSparkline::SetData(const TArray<float>& NewData)
{
	DataPoints = NewData;

	// Trim if necessary
	const int32 MaxPoints = MaxDataPoints.Get();
	while (DataPoints.Num() > MaxPoints)
	{
		DataPoints.RemoveAt(0);
	}
}

int32 SPoolSparkline::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 NumPoints = DataPoints.Num();
	if (NumPoints < 2)
	{
		return LayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float Width = LocalSize.X;
	const float Height = LocalSize.Y;

	// Get data range for normalization
	float MinValue, MaxValue;
	GetDataRange(MinValue, MaxValue);

	// Avoid division by zero
	const float Range = MaxValue - MinValue;
	if (Range < SMALL_NUMBER)
	{
		return LayerId;
	}

	// Build line points
	TArray<FVector2D> LinePoints;
	LinePoints.Reserve(NumPoints);

	const float XStep = Width / FMath::Max(1, NumPoints - 1);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float NormalizedValue = (DataPoints[i] - MinValue) / Range;
		const float X = i * XStep;
		const float Y = Height - (NormalizedValue * Height); // Invert Y (Slate coordinates go down)

		LinePoints.Add(FVector2D(X, Y));
	}

	// Draw fill area under the line using simpler approach
	if (FillColor.Get().A > 0.0f)
	{
		// Build points for the filled area
		TArray<FVector2D> FillPoints;
		FillPoints.Reserve(NumPoints + 2);

		// Add line points
		for (const FVector2D& Point : LinePoints)
		{
			FillPoints.Add(Point);
		}

		// Close the polygon by adding bottom corners
		FillPoints.Add(FVector2D(Width, Height));
		FillPoints.Add(FVector2D(0.0f, Height));

		// Draw filled polygon
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FillPoints,
			ESlateDrawEffect::None,
			FillColor.Get(),
			false, // Don't anti-alias the fill
			1.0f
		);
	}

	// Draw the line
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		ESlateDrawEffect::None,
		LineColor.Get(),
		true,
		LineThickness.Get()
	);

	return LayerId + 2;
}

FVector2D SPoolSparkline::ComputeDesiredSize(float) const
{
	return FVector2D(100.0f, 30.0f);
}

void SPoolSparkline::GetDataRange(float& OutMin, float& OutMax) const
{
	if (DataPoints.Num() == 0)
	{
		OutMin = 0.0f;
		OutMax = 1.0f;
		return;
	}

	OutMin = DataPoints[0];
	OutMax = DataPoints[0];

	for (float Value : DataPoints)
	{
		OutMin = FMath::Min(OutMin, Value);
		OutMax = FMath::Max(OutMax, Value);
	}

	// Ensure we have some range
	if (FMath::IsNearlyEqual(OutMin, OutMax))
	{
		OutMax = OutMin + 1.0f;
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
