// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolPerformancePanel.h"
#include "FLazyGenericDynamicObjectPoolStyle.h"
#include "PoolManagerStyles.h"
#include "Styling/AppStyle.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SPoolPerformancePanel"

// Local helper for row
class SPerformanceTableRow : public SMultiColumnTableRow<TSharedPtr<FPoolPerformanceItem>>
{
public:
	SLATE_BEGIN_ARGS(SPerformanceTableRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView,
				   TSharedPtr<FPoolPerformanceItem> InItem)
	{
		Item = InItem;
		SMultiColumnTableRow<TSharedPtr<FPoolPerformanceItem>>::Construct(FSuperRowType::FArguments(),
																		  InOwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid()) return SNullWidget::NullWidget;

		FText ColumnText;
		FLinearColor Color = FPoolManagerStyles::GetTextPrimary();
		const FSlateFontInfo RowFont = FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall);
		const FSlateFontInfo BoldFont = FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeSmall);

		// Common Padding
		const FMargin CellPadding(12.0f, 6.0f);

		if (ColumnName == "PoolClass")
		{
			return SNew(SBox).Padding(CellPadding)[SNew(STextBlock)
													   .Text(FText::FromString(Item->ClassName))
													   .Font(BoldFont)
													   .ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())];
		}
		else if (ColumnName == "AvgRetrieval")
		{
			ColumnText = FText::AsNumber(Item->AvgRetrievalTime);
			if (Item->AvgRetrievalTime > 0.5f) Color = FPoolManagerStyles::GetWarningColor();
			if (Item->AvgRetrievalTime > 1.0f) Color = FPoolManagerStyles::GetCriticalColor();
		}
		else if (ColumnName == "GrowthOps")
		{
			ColumnText = FText::AsNumber(Item->GrowthOps);
			if (Item->GrowthOps > 10) Color = FPoolManagerStyles::GetWarningColor();
		}
		else if (ColumnName == "ShrinkOps") { ColumnText = FText::AsNumber(Item->ShrinkOps); }
		else if (ColumnName == "Efficiency")
		{
			ColumnText = FText::AsNumber(Item->EfficiencyScore); // Mock score
			if (Item->EfficiencyScore < 0.5f) Color = FPoolManagerStyles::GetWarningColor();
		}

		return SNew(SBox).Padding(CellPadding)[SNew(STextBlock).Text(ColumnText).Font(RowFont).ColorAndOpacity(Color)];
	}

private:
	TSharedPtr<FPoolPerformanceItem> Item;
};

void SPoolPerformancePanel::Construct(const FArguments& InArgs)
{
	SPoolPanelBase::Construct(SPoolPanelBase::FArguments());

	ChildSlot
		[SNew(SScrollBox) +
		 SScrollBox::Slot().Padding(FPoolManagerStyles::PaddingMedium)
			 [SNew(SVerticalBox)

			  // 1. Performance Metrics (Top Grid)
			  + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f,
														  FPoolManagerStyles::PaddingMedium)[BuildMetricsGrid()]

			  // 2. Detailed Breakdown Title
			  +
			  SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingSmall)
				  [SNew(SBorder)
					   .BorderImage(FPoolManagerStyles::GetWhiteBrush())
					   .BorderBackgroundColor(FPoolManagerStyles::GetHeaderBackground())
					   .Padding(FPoolManagerStyles::PaddingNone)
						   [SNew(SVerticalBox) +
							SVerticalBox::Slot().AutoHeight().Padding(FMargin(FPoolManagerStyles::PaddingMedium))
								[SNew(SHorizontalBox) +
								 SHorizontalBox::Slot()
									 .AutoWidth()
									 .VAlign(VAlign_Center)
									 .Padding(0.0f, 0.0f, 8.0f,
											  0.0f)[SNew(SImage)
														.Image(FAppStyle::GetBrush("Icons.Layout")) // Placeholder icon
														.ColorAndOpacity(FPoolManagerStyles::GetAccentPurple())
														.DesiredSizeOverride(FVector2D(16.0f, 16.0f))] +
								 SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
									 [SNew(STextBlock)
										  .Text(LOCTEXT("DetailedStatsTitle", "DETAILED BREAKDOWN by POOL CLASS"))
										  .Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
										  .ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())]]
							// Separator
							+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(
								  1.0f)[SNew(SImage)
											.Image(FPoolManagerStyles::GetWhiteBrush())
											.ColorAndOpacity(FPoolManagerStyles::GetBorderColor())]]]]

			  // 3. Detailed Data Table
			  + SVerticalBox::Slot().FillHeight(1.0f)[BuildDetailedStats()]]];
}

void SPoolPerformancePanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPoolPanelBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (InCurrentTime - LastUpdateTime < 1.0) return;
	LastUpdateTime = InCurrentTime;
	RefreshPerformanceList();
}

TSharedRef<SWidget> SPoolPerformancePanel::BuildMetricsGrid()
{
	// Matching Global Stats Layout from Dashboard
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetContainerBackground())
		.Padding(FPoolManagerStyles::PaddingNone)
			[SNew(SVerticalBox)

			 // Header
			 +
			 SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingMedium)
				 [SNew(SBorder)
					  .BorderImage(FPoolManagerStyles::GetWhiteBrush())
					  .BorderBackgroundColor(FPoolManagerStyles::GetHeaderBackground())
					  .Padding(FPoolManagerStyles::PaddingNone)
						  [SNew(SVerticalBox) +
						   SVerticalBox::Slot().AutoHeight().Padding(FMargin(FPoolManagerStyles::PaddingMedium))
							   [SNew(SHorizontalBox) +
								SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(0.0f, 0.0f, 8.0f,
											 0.0f)[SNew(SImage)
													   .Image(FAppStyle::GetBrush("Icons.Layout"))
													   .ColorAndOpacity(FPoolManagerStyles::GetAccentBlue())
													   .DesiredSizeOverride(FVector2D(16.0f, 16.0f))] +
								SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
									[SNew(STextBlock)
										 .Text(LOCTEXT("PerfMetricsTitle", "PERFORMANCE METRICS"))
										 .Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
										 .ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())]] +
						   SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(
							   1.0f)[SNew(SImage)
										 .Image(FPoolManagerStyles::GetWhiteBrush())
										 .ColorAndOpacity(FPoolManagerStyles::GetBorderColor())]]]]

			 // Grid Content
			 + SVerticalBox::Slot().AutoHeight()
				   [SNew(SHorizontalBox)

					// Avg Retrieval
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						BuildMetricCard(
						  LOCTEXT("AvgRetrieval", "AVG RETRIEVAL (ms)"),
						  TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
							  this, &SPoolPerformancePanel::GetAverageRetrievalTimeText)),
						  FPoolManagerStyles::GetAccentBlue())
						  ]

					// Growth Ops
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 12.0f, 0.0f)[BuildMetricCard(
						  LOCTEXT("GrowthOps", "GROWTH OPS"),
						  TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
							  this, &SPoolPerformancePanel::GetTotalGrowthOperationsText)),
						  FPoolManagerStyles::GetAccentPurple())]

					// Shrink Ops
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 12.0f, 0.0f)[BuildMetricCard(
						  LOCTEXT("ShrinkOps", "SHRINK OPS"),
						  TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
							  this, &SPoolPerformancePanel::GetTotalShrinkOperationsText)),
						  FPoolManagerStyles::GetAccentOrange())]

					// Efficiency
					+ SHorizontalBox::Slot().FillWidth(
						  1.0f)[BuildMetricCard(LOCTEXT("MemEfficiency", "EFFICIENCY"),
												TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
													this, &SPoolPerformancePanel::GetMemoryEfficiencyText)),
												FPoolManagerStyles::GetHealthyColor())]]];
}

TSharedRef<SWidget> SPoolPerformancePanel::BuildMetricCard(const FText& Label, const TAttribute<FText>& Value,
														   const FLinearColor& Color)
{
	// Matching BuildStatCardWidget from Dashboard
	return SNew(SBorder)
		.BorderImage(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush("PoolManager.RoundedBorder"))
		.BorderBackgroundColor(FPoolManagerStyles::GetBorderColor())
		.Padding(FMargin(
			1.0f))[SNew(SBorder)
					   .BorderImage(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush("PoolManager.CardGradient"))
					   .BorderBackgroundColor(FLinearColor::White)
					   .Padding(FPoolManagerStyles::PaddingLarge)
					   .VAlign(VAlign_Center)
						   [SNew(SVerticalBox)

							// Value
							+ SVerticalBox::Slot()
								  .AutoHeight()
								  .HAlign(HAlign_Center)
								  .Padding(0.0f, 0.0f, 0.0f, 4.0f)[SNew(STextBlock)
																	   .Text(Value)
																	   .Font(FPoolManagerStyles::GetFont("Bold", 20))
																	   .ColorAndOpacity(Color)]

							// Label
							+ SVerticalBox::Slot().HAlign(HAlign_Center)
								  [SNew(STextBlock)
									   .Text(Label)
									   .Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
									   .ColorAndOpacity(FPoolManagerStyles::GetTextMuted())]]];
}

TSharedRef<SWidget> SPoolPerformancePanel::BuildDetailedStats()
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetContainerBackground())
		.Padding(FMargin(1.0f)) // Thin border
			[SNew(SListView<TSharedPtr<FPoolPerformanceItem>>)
				 .ListItemsSource(&PerformanceItems)
				 .OnGenerateRow(this, &SPoolPerformancePanel::OnGenerateRow)
				 .HeaderRow(
					 SNew(SHeaderRow).Visibility(EVisibility::Visible) +
					 SHeaderRow::Column("PoolClass").DefaultLabel(LOCTEXT("ColClass", "Pool Class")).FillWidth(0.4f) +
					 SHeaderRow::Column("AvgRetrieval")
						 .DefaultLabel(LOCTEXT("ColRetrieval", "Avg Retrieval (ms)"))
						 .FillWidth(0.15f) +
					 SHeaderRow::Column("GrowthOps").DefaultLabel(LOCTEXT("ColGrowth", "Growth Ops")).FillWidth(0.15f) +
					 SHeaderRow::Column("ShrinkOps").DefaultLabel(LOCTEXT("ColShrink", "Shrink Ops")).FillWidth(0.15f) +
					 SHeaderRow::Column("Efficiency")
						 .DefaultLabel(LOCTEXT("ColEfficiency", "Efficiency"))
						 .FillWidth(0.15f))];
}

TSharedRef<ITableRow> SPoolPerformancePanel::OnGenerateRow(TSharedPtr<FPoolPerformanceItem> Item,
														   const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SPerformanceTableRow, OwnerTable, Item);
}

void SPoolPerformancePanel::RefreshPerformanceList()
{
	PerformanceItems.Empty();

	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		const TArray<TSubclassOf<AActor>>& PooledClasses = Subsystem->GetAllPooledClasses();

		float TotalAvgTime = 0.0f;
		int32 TotalGrowth = 0;
		int32 TotalShrink = 0;

		for (const TSubclassOf<AActor>& PoolClass : PooledClasses)
		{
			if (!PoolClass) continue;

			// Mock or real data retrieval
			float AvgTime = 0.02f; // Mock
			int32 Growth = 5; // Mock
			int32 Shrink = 2; // Mock
			float Eff = 0.95f; // Mock

			// In a real scenario, we'd pull this from FPoolPerformanceMetrics via Subsystem
			// e.g. AvgTime = Subsystem->GetPerformanceMetrics(PoolClass).AverageRetrievalTime;

			TotalAvgTime += AvgTime;
			TotalGrowth += Growth;
			TotalShrink += Shrink;

			PerformanceItems.Add(MakeShared<FPoolPerformanceItem>(PoolClass->GetName(), AvgTime, Growth, Shrink, Eff));
		}

		CachedAvgRetrievalTime = PooledClasses.Num() > 0 ? TotalAvgTime / PooledClasses.Num() : 0.0f;
		CachedGrowthOps = TotalGrowth;
		CachedShrinkOps = TotalShrink;
	}
	else
	{
		// Mock Data for Editor Preview
		PerformanceItems.Add(MakeShared<FPoolPerformanceItem>("BP_Enemy_Slime", 0.05f, 12, 4, 0.88f));
		PerformanceItems.Add(MakeShared<FPoolPerformanceItem>("BP_Projectile", 0.01f, 55, 10, 0.99f));
		PerformanceItems.Add(MakeShared<FPoolPerformanceItem>("BP_VFX_Impact", 0.02f, 8, 0, 1.0f));

		CachedAvgRetrievalTime = 0.03f;
		CachedGrowthOps = 75;
		CachedShrinkOps = 14;
	}

	if (PerformanceListView.IsValid()) { PerformanceListView->RequestListRefresh(); }
}

FText SPoolPerformancePanel::GetAverageRetrievalTimeText() const
{
	if (!IsInPIE()) return LOCTEXT("NA", "--");
	return FText::AsNumber(CachedAvgRetrievalTime);
}

FText SPoolPerformancePanel::GetTotalGrowthOperationsText() const
{
	if (!IsInPIE()) return LOCTEXT("NA", "--");
	return FText::AsNumber(CachedGrowthOps);
}

FText SPoolPerformancePanel::GetTotalShrinkOperationsText() const
{
	if (!IsInPIE()) return LOCTEXT("NA", "--");
	return FText::AsNumber(CachedShrinkOps);
}

FText SPoolPerformancePanel::GetMemoryEfficiencyText() const
{
	// Placeholder
	return LOCTEXT("EfficiencyPlaceholder", "94%");
}

FReply SPoolPerformancePanel::OnResetMetricsClicked()
{
	// TODO: Reset stats in subsystem
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
