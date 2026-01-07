// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolOverviewPanel.h"
#include "Slate/SPoolSparkline.h"
#include "Editor.h"
#include "FLazyGenericDynamicObjectPoolStyle.h"
#include "PoolManagerStyles.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "SPoolOverviewPanel"

void SPoolOverviewPanel::Construct(const FArguments& InArgs)
{
	SPoolPanelBase::Construct(SPoolPanelBase::FArguments());

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FColor::Transparent)
		.Padding(FPoolManagerStyles::PaddingMedium)
		[
			SNew(SVerticalBox)

			// Header Bar (Class Selector + Stats)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingMedium)
			[
				BuildHeaderBar()
			]

			// Action Bar + Side Stats
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingMedium)
			[
				BuildActionBar()
			]

			// Sparkline Charts
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingMedium)
			[
				BuildChartsSection()
			]

			// Summary Stats Row
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingMedium)
			[
				BuildSummaryRow()
			]

			// Actor Pool List (fills remaining space)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				BuildActorListSection()
			]
		]
	];
}

void SPoolOverviewPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPoolPanelBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Sample sparkline data every 0.5s
	if (InCurrentTime - LastSparklineSampleTime >= 0.5)
	{
		LastSparklineSampleTime = InCurrentTime;

		if (IsInPIE() && OverviewClass)
		{
			if (CacheHitSparkline.IsValid()) { CacheHitSparkline->AddDataPoint(GetCurrentCacheHitPercent()); }
			if (UsageSparkline.IsValid()) { UsageSparkline->AddDataPoint(GetCurrentUsagePercent()); }
			if (GrowthSparkline.IsValid()) { GrowthSparkline->AddDataPoint(GetCurrentGrowthRate()); }
		}
	}

	// Refresh actor list every 1s
	if (InCurrentTime - LastUpdateTime >= 1.0)
	{
		LastUpdateTime = InCurrentTime;
		RefreshActorList();
	}
}

// ==================== UI BUILDERS ====================

TSharedRef<SWidget> SPoolOverviewPanel::BuildHeaderBar()
{
	return SNew(SHorizontalBox)

			// Class Selector (fills remaining space on left)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingLarge, 0.0f)
			[
				SNew(SClassPropertyEntryBox)
				.MetaClass(AActor::StaticClass())
				.SelectedClass(this, &SPoolOverviewPanel::GetSelectedActorClass)
				.OnSetClass(this, &SPoolOverviewPanel::OnActorClassSelected)
			]

			// Health Badge (same card style as stat badges)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingSmall, 0.0f)
			[
				SNew(SBox)
				.MinDesiredHeight(32.0f)
				[
					SNew(SBorder)
					.BorderImage(FPoolManagerStyles::GetWhiteBrush())
					.BorderBackgroundColor_Raw(this, &SPoolOverviewPanel::GetHealthStatusColor)
					.Padding(FMargin(8.0f, 4.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 2.0f)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Check"))
							.ColorAndOpacity(FLinearColor::White)
							.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text_Raw(this, &SPoolOverviewPanel::GetHealthStatusText)
							.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeTiny))
							.ColorAndOpacity(FLinearColor::White)
						]
					]
				]
			]

			// Stat Badges (on the right)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingSmall, 0.0f)
			[
				BuildStatBadge(
					TAttribute<FText>::CreateSP(this, &SPoolOverviewPanel::GetActorsInUseText),
					LOCTEXT("InUseLabel", "ACTOR IN USE"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingSmall, 0.0f)
			[
				BuildStatBadge(
					TAttribute<FText>::CreateSP(this, &SPoolOverviewPanel::GetActorsAvailableText),
					LOCTEXT("AvailableLabel", "AVAILABLE ACTORS"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingSmall, 0.0f)
			[
				BuildStatBadge(
					TAttribute<FText>::CreateSP(this, &SPoolOverviewPanel::GetGrowthOpsText),
					LOCTEXT("GrowthLabel", "GROWTH OPERATION"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				BuildStatBadge(
					TAttribute<FText>::CreateSP(this, &SPoolOverviewPanel::GetCacheHitText),
					LOCTEXT("CacheHitLabel", "CACHE HIT RATE"))
			];
}

TSharedRef<SWidget> SPoolOverviewPanel::BuildActionBar()
{
	return SNew(SHorizontalBox)

			// Action Buttons (Clear Pool, Shrink Pool)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)

				// Clear Pool Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(8.0f, 4.0f))
					.OnClicked(this, &SPoolOverviewPanel::OnClearPoolClicked)
					.IsEnabled_Lambda([this]() { return IsInPIE() && OverviewClass != nullptr; })
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 2.0f)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Delete"))
							.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
							.DesiredSizeOverride(FVector2D(20.0f, 20.0f))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ClearPoolBtn", "Clear Pool"))
							.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeTiny))
							.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
						]
					]
				]

				// Shrink Pool Button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(FMargin(8.0f, 4.0f))
					.OnClicked(this, &SPoolOverviewPanel::OnShrinkPoolClicked)
					.IsEnabled_Lambda([this]() { return IsInPIE() && OverviewClass != nullptr; })
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 2.0f)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Transform"))
							.ColorAndOpacity(FPoolManagerStyles::GetAccentOrange())
							.DesiredSizeOverride(FVector2D(20.0f, 20.0f))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ShrinkPoolBtn", "Shrink Pool"))
							.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeTiny))
							.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
						]
					]
				]
			]

			// Spacer
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			// Side Stats (Pool Shrink In, Last Accessed)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				// Pool Shrink In
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingLarge, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Raw(this, &SPoolOverviewPanel::GetPoolShrinkInText)
						.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeBody))
						.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ShrinkInLabel", "Pool Shrink In"))
						.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeTiny))
						.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
					]
				]

				// Last Accessed
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Raw(this, &SPoolOverviewPanel::GetLastAccessedText)
						.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeBody))
						.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("LastAccessLabel", "Last Accessed"))
						.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeTiny))
						.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
					]
				]
			];
}

TSharedRef<SWidget> SPoolOverviewPanel::BuildChartsSection()
{
	return SNew(SHorizontalBox)

			// Cache Hit % Chart
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingSmall, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CacheHitChartLabel", "Cache Hit %"))
					.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
					.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(40.0f)
					[
						SAssignNew(CacheHitSparkline, SPoolSparkline)
						.LineColor(FPoolManagerStyles::GetAccentBlue())
						.FillColor(FLinearColor(0.23f, 0.51f, 0.96f, 0.2f))
						.LineThickness(1.5f)
						.MaxDataPoints(60)
					]
				]
			]

			// Pool Usage % Chart
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingSmall, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("UsageChartLabel", "Pool Usage %"))
					.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
					.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(40.0f)
					[
						SAssignNew(UsageSparkline, SPoolSparkline)
						.LineColor(FPoolManagerStyles::GetAccentOrange())
						.FillColor(FLinearColor(0.98f, 0.45f, 0.09f, 0.2f))
						.LineThickness(1.5f)
						.MaxDataPoints(60)
					]
				]
			]

			// Growth Rate Chart
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GrowthChartLabel", "Growth Rate"))
					.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
					.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(40.0f)
					[
						SAssignNew(GrowthSparkline, SPoolSparkline)
						.LineColor(FPoolManagerStyles::GetAccentYellow())
						.FillColor(FLinearColor(0.92f, 0.70f, 0.03f, 0.2f))
						.LineThickness(1.5f)
						.MaxDataPoints(60)
					]
				]
			];
}

TSharedRef<SWidget> SPoolOverviewPanel::BuildSummaryRow()
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
		.Padding(FMargin(FPoolManagerStyles::PaddingSmall))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Raw(this, &SPoolOverviewPanel::GetTotalActorsText)
				.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
				.ColorAndOpacity(FPoolManagerStyles::GetAccentBlue())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FPoolManagerStyles::PaddingMedium, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString("|"))
				.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Raw(this, &SPoolOverviewPanel::GetMaxPoolSizeText)
				.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
				.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FPoolManagerStyles::PaddingMedium, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString("|"))
				.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Raw(this, &SPoolOverviewPanel::GetShrinkOperationsText)
				.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
				.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
			]
		];
}

TSharedRef<SWidget> SPoolOverviewPanel::BuildActorListSection()
{
	return SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingSmall)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ActorPoolTitle", "Actor Pool"))
					.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
					.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::Format(
							LOCTEXT("ActorCountFmt", "{0} actors"), FText::AsNumber(FilteredActorItems.Num()));
					})
					.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
					.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
				]
			]

			// Search Box
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingSmall)
			[
				SNew(SSearchBox)
				.HintText(LOCTEXT("SearchActors", "Search actors..."))
				.OnTextChanged(this, &SPoolOverviewPanel::OnActorSearchChanged)
			]

			// List View
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FPoolManagerStyles::GetWhiteBrush())
				.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
				.Padding(0.0f)
				[
					SAssignNew(ActorListView, SListView<TSharedPtr<FPoolActorItem>>)
					.ListItemsSource(&FilteredActorItems)
					.OnGenerateRow(this, &SPoolOverviewPanel::OnGenerateActorRow)
					.SelectionMode(ESelectionMode::Single)
				]
			];
}

TSharedRef<SWidget> SPoolOverviewPanel::BuildActionButton(const FSlateBrush* Icon, const FText& Tooltip,
                                                          FOnClicked OnClicked)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(6.0f))
		.ToolTipText(Tooltip)
		.OnClicked(OnClicked)
		.IsEnabled_Lambda([this]() { return IsInPIE() && OverviewClass != nullptr; })
		[
			SNew(SImage)
			.Image(Icon)
			.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
			.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
		];
}

TSharedRef<SWidget> SPoolOverviewPanel::BuildStatBadge(const TAttribute<FText>& Value, const FText& Label)
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
		.Padding(FMargin(8.0f, 4.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(Value)
				.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeBody))
				.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeTiny))
				.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
			]
		];
}

// ==================== LIST VIEW ====================

TSharedRef<ITableRow> SPoolOverviewPanel::OnGenerateActorRow(TSharedPtr<FPoolActorItem> Item,
                                                             const TSharedRef<STableViewBase>& OwnerTable)
{
	// Determine status color
	FLinearColor StatusColor = FPoolManagerStyles::GetAccentGreen(); // Available
	if (Item->bIsLeaked)
	{
		StatusColor = FPoolManagerStyles::GetCriticalColor(); // Red
	}
	else if (Item->bIsInUse)
	{
		StatusColor = FPoolManagerStyles::GetAccentOrange(); // In use
	}

	return SNew(STableRow<TSharedPtr<FPoolActorItem>>, OwnerTable)
		.Padding(FMargin(8.0f, 4.0f))
		[
			SNew(SHorizontalBox)

			// Status Dot
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(8.0f)
				.HeightOverride(8.0f)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.FilledCircle"))
					.ColorAndOpacity(StatusColor)
				]
			]

			// Actor Name
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->ActorName))
				.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeBody))
				.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
			]

			// LEAKED badge
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FPoolManagerStyles::GetWhiteBrush())
				.BorderBackgroundColor(FPoolManagerStyles::GetCriticalColor())
				.Padding(FMargin(4.0f, 2.0f))
				.Visibility(Item->bIsLeaked ? EVisibility::Visible : EVisibility::Collapsed)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LeakedBadge", "LEAKED"))
					.Font(FPoolManagerStyles::GetFont("Bold", 7))
					.ColorAndOpacity(FLinearColor::White)
				]
			]

			// State text
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(
					Item->bIsInUse
						? FString::Printf(TEXT("Active: %.1fs"), Item->ActiveTime)
						: TEXT("In Pool")))
				.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
				.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
			]

			// Owner
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("Owner: %s"), *Item->OwnerName)))
				.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeTiny))
				.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
				.Visibility(Item->bIsInUse ? EVisibility::Visible : EVisibility::Collapsed)
			]
		];
}

void SPoolOverviewPanel::RefreshActorList()
{
	ActorListItems.Empty();

	if (!IsInPIE() || !OverviewClass)
	{
		FilteredActorItems.Empty();
		if (ActorListView.IsValid()) { ActorListView->RequestListRefresh(); }
		return;
	}

	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return; }

	// Get all actors in pool for the selected class
	TArray<AActor*> AllActors = Subsystem->GetAllActorsInPool(OverviewClass);
	TArray<AActor*> InUseActors = Subsystem->GetInUseActorsInPool(OverviewClass);

	for (AActor* Actor : AllActors)
	{
		if (!IsValid(Actor)) { continue; }

		bool bInUse = InUseActors.Contains(Actor);
		float ActiveTime = 0.0f; // Would come from subsystem if tracked
		bool bLeaked = bInUse && ActiveTime > LeakThresholdSeconds;
		FString OwnerName = Actor->GetOwner() ? Actor->GetOwner()->GetName() : TEXT("None");

		// Clean up the name
		FString ActorName = Actor->GetName();
		ActorName.RemoveFromEnd(TEXT("_C"));

		ActorListItems.Add(MakeShared<FPoolActorItem>(Actor, ActorName, ActiveTime, bInUse, bLeaked, OwnerName));
	}

	OnActorSearchChanged(FText::FromString(ActorSearchText));
}

void SPoolOverviewPanel::OnActorSearchChanged(const FText& InText)
{
	ActorSearchText = InText.ToString();
	FilteredActorItems.Empty();

	for (const TSharedPtr<FPoolActorItem>& Item : ActorListItems)
	{
		if (ActorSearchText.IsEmpty() || Item->ActorName.Contains(ActorSearchText)) { FilteredActorItems.Add(Item); }
	}

	if (ActorListView.IsValid()) { ActorListView->RequestListRefresh(); }
}

// ==================== CLASS SELECTION ====================

const UClass* SPoolOverviewPanel::GetSelectedActorClass() const { return OverviewClass.Get(); }

void SPoolOverviewPanel::OnActorClassSelected(const UClass* InClass)
{
	OverviewClass = TSubclassOf<AActor>(const_cast<UClass*>(InClass));

	// Clear sparkline history when class changes
	if (CacheHitSparkline.IsValid()) { CacheHitSparkline->ClearData(); }
	if (UsageSparkline.IsValid()) { UsageSparkline->ClearData(); }
	if (GrowthSparkline.IsValid()) { GrowthSparkline->ClearData(); }

	RefreshActorList();
}

// ==================== DATA GETTERS ====================

FText SPoolOverviewPanel::GetHealthStatusText() const
{
	if (!IsInPIE() || !OverviewClass) { return LOCTEXT("NoClass", "N/A"); }

	float UsagePercent = GetCurrentUsagePercent();
	if (UsagePercent > 0.9f) { return LOCTEXT("StatusCritical", "CRITICAL"); }
	if (UsagePercent > 0.75f) { return LOCTEXT("StatusWarning", "WARNING"); }
	return LOCTEXT("StatusHealthy", "HEALTHY");
}

FSlateColor SPoolOverviewPanel::GetHealthStatusColor() const
{
	if (!IsInPIE() || !OverviewClass) { return FPoolManagerStyles::GetTextMuted(); }

	float UsagePercent = GetCurrentUsagePercent();
	if (UsagePercent > 0.9f) { return FPoolManagerStyles::GetCriticalColor(); }
	if (UsagePercent > 0.75f) { return FPoolManagerStyles::GetWarningColor(); }
	return FPoolManagerStyles::GetAccentGreen();
}

FText SPoolOverviewPanel::GetActorsInUseText() const
{
	if (!IsInPIE() || !OverviewClass) { return FText::AsNumber(0); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return FText::AsNumber(0); }
	return FText::AsNumber(Subsystem->GetInUseActorsInPool(OverviewClass).Num());
}

FText SPoolOverviewPanel::GetActorsAvailableText() const
{
	if (!IsInPIE() || !OverviewClass) { return FText::AsNumber(0); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return FText::AsNumber(0); }
	return FText::AsNumber(Subsystem->GetAvailableActorsInPool(OverviewClass).Num());
}

FText SPoolOverviewPanel::GetGrowthOpsText() const
{
	if (!IsInPIE() || !OverviewClass) { return FText::AsNumber(0); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return FText::AsNumber(0); }
	return FText::AsNumber(Subsystem->GetPoolGrowthOperation(OverviewClass));
}

FText SPoolOverviewPanel::GetCacheHitText() const
{
	if (!IsInPIE() || !OverviewClass) { return LOCTEXT("NaPercent", "--"); }
	return FText::Format(
		LOCTEXT("CacheHitFmt", "{0}%"), FText::AsNumber(FMath::RoundToInt(GetCurrentCacheHitPercent())));
}

FSlateColor SPoolOverviewPanel::GetCacheHitColor() const
{
	float CacheHit = GetCurrentCacheHitPercent();
	return FPoolManagerStyles::GetCacheHitColor(CacheHit);
}

FText SPoolOverviewPanel::GetPoolShrinkInText() const
{
	if (!IsInPIE()) { return LOCTEXT("DisabledTime", "--"); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return LOCTEXT("DisabledTime", "--"); }

	float Time = Subsystem->GetNextAutoShrinkTime();
	if (Time <= 0.0f) { return LOCTEXT("DisabledTime", "--"); }
	return FText::AsNumber(FMath::RoundToInt(Time));
}

FText SPoolOverviewPanel::GetLastAccessedText() const
{
	if (!IsInPIE() || !OverviewClass) { return LOCTEXT("NaTimes", "--"); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return LOCTEXT("NaTimes", "--"); }

	int32 AccessCount = Subsystem->GetPoolAccessCount(OverviewClass);
	return FText::Format(LOCTEXT("AccessCountFmt", "{0} times"), FText::AsNumber(AccessCount));
}

FText SPoolOverviewPanel::GetTotalActorsText() const
{
	if (!IsInPIE() || !OverviewClass) { return LOCTEXT("NoActors", "0 Total Actor"); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return LOCTEXT("NoActors", "0 Total Actor"); }

	int32 Total = Subsystem->GetPoolSize(OverviewClass);
	FString ClassName = OverviewClass->GetName();
	ClassName.RemoveFromEnd(TEXT("_C"));

	return FText::Format(LOCTEXT("TotalActorFmt", "{0} Total Actor Of Class {1}"),
	                     FText::AsNumber(Total), FText::FromString(ClassName));
}

FText SPoolOverviewPanel::GetMaxPoolSizeText() const
{
	if (!IsInPIE()) { return LOCTEXT("MaxPool", "0 Max Pool Size"); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return LOCTEXT("MaxPool", "0 Max Pool Size"); }

	return FText::Format(LOCTEXT("MaxPoolFmt", "{0} Max Pool Size"),
	                     FText::AsNumber(Subsystem->GetMaximumPoolSize()));
}

FText SPoolOverviewPanel::GetShrinkOperationsText() const
{
	if (!IsInPIE()) { return LOCTEXT("ShrinkOps", "0 Shrink Operations Completed"); }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return LOCTEXT("ShrinkOps", "0 Shrink Operations Completed"); }

	return FText::Format(LOCTEXT("ShrinkOpsFmt", "{0} Shrink Operations Completed"),
	                     FText::AsNumber(Subsystem->GetTotalShrinkOperations()));
}

float SPoolOverviewPanel::GetCurrentCacheHitPercent() const
{
	if (!IsInPIE() || !OverviewClass) { return 0.0f; }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return 0.0f; }
	return Subsystem->GetCacheHitRate(OverviewClass);
}

float SPoolOverviewPanel::GetCurrentUsagePercent() const
{
	if (!IsInPIE() || !OverviewClass) { return 0.0f; }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return 0.0f; }

	int32 InUse = Subsystem->GetInUseActorsInPool(OverviewClass).Num();
	int32 Total = Subsystem->GetPoolSize(OverviewClass);
	return Total > 0 ? static_cast<float>(InUse) / static_cast<float>(Total) * 100.0f : 0.0f;
}

float SPoolOverviewPanel::GetCurrentGrowthRate() const
{
	if (!IsInPIE() || !OverviewClass) { return 0.0f; }
	ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem();
	if (!Subsystem) { return 0.0f; }

	// Return growth operations as a simple rate (would ideally be ops/min)
	return static_cast<float>(Subsystem->GetPoolGrowthOperation(OverviewClass));
}

// ==================== ACTIONS ====================

FReply SPoolOverviewPanel::OnClearPoolClicked()
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		if (OverviewClass)
		{
			Subsystem->ClearPool(OverviewClass);
			RefreshActorList();
		}
	}
	return FReply::Handled();
}

FReply SPoolOverviewPanel::OnShrinkPoolClicked()
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		if (OverviewClass)
		{
			// Use ShrinkAllPools since ShrinkPool is private
			Subsystem->ShrinkAllPools();
			RefreshActorList();
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
