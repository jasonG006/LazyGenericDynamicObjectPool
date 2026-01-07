// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolDashboardPanel.h"
#include "Editor.h"
#include "FLazyGenericDynamicObjectPoolStyle.h"
#include "PoolManagerStyles.h"
#include "Slate/SPoolCard.h"
#include "Slate/SSectionHeader.h"
#include "Styling/AppStyle.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPoolDashboardPanel"

void SPoolDashboardPanel::Construct(const FArguments& InArgs)
{
	SPoolPanelBase::Construct(SPoolPanelBase::FArguments());

	// Initialize dummy activity log
	LogActivity(LOCTEXT("InitLog1", "Dashboard initialized"), FPoolManagerStyles::GetAccentBlue());

	ChildSlot
		[SNew(SScrollBox)

			// Global Log

			// Global Statistics Section
			+ SScrollBox::Slot().Padding(FPoolManagerStyles::PaddingMedium)
				[BuildGlobalStatsSection()]

			// Main Content (Split View: Grid + Sidebar)
			+ SScrollBox::Slot().Padding(FPoolManagerStyles::PaddingMedium)
				[BuildMainContent()]];
}

void SPoolDashboardPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPoolPanelBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Throttle updates to every 0.5 seconds
	if (InCurrentTime - LastUpdateTime < 0.5)
	{
		return;
	}
	LastUpdateTime = InCurrentTime;

	// Check for PIE state change
	const bool bCurrentPIE = IsInPIE();
	if (bCurrentPIE != bCachedIsPIE)
	{
		bCachedIsPIE = bCurrentPIE;
		RefreshPoolList();
	}

	// Update cached stats from subsystem
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		const TArray<TSubclassOf<AActor>>& PooledClasses = Subsystem->GetAllPooledClasses();

		// Only rebuild pool grid when pool count changes
		// SPoolCard uses TAttribute bindings so individual card data updates automatically
		const bool bPoolCountChanged = PooledClasses.Num() != CachedActivePoolCount;
		if (bPoolCountChanged)
		{
			RefreshPoolList();
		}

		CachedActivePoolCount = PooledClasses.Num();
		CachedTotalActors = Subsystem->GetTotalActorsInAllPools();

		// Calculate actors in use and average cache hit by iterating all pools
		int32 TotalInUse = 0;
		float TotalCacheHit = 0.0f;
		for (const TSubclassOf<AActor>& PoolClass : PooledClasses)
		{
			TotalInUse += Subsystem->GetInUseActorsInPool(PoolClass).Num();
			TotalCacheHit += Subsystem->GetCacheHitRate(PoolClass);
		}

		CachedActorsInUse = TotalInUse;
		CachedAvgCacheHit = PooledClasses.Num() > 0 ? TotalCacheHit / PooledClasses.Num() : 0.0f;

		// Memory estimation (rough: ~1KB per actor placeholder)
		CachedMemoryMB = CachedTotalActors * 0.001f;

		// Leak count - leave as 0, actual leak detection requires explicit scan
		CachedLeakCount = 0;
	}
}

// ==================== UI BUILDERS ====================

TSharedRef<SWidget> SPoolDashboardPanel::BuildMainContent()
{
	return SNew(SBox).MinDesiredHeight(500.0f).Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium,
		0.0f)
		[BuildAllPoolsSection()];
}

TSharedRef<SWidget> SPoolDashboardPanel::BuildAllPoolsSection()
{
	// Title (Matching Global Stats Style)
	const TSharedRef<SVerticalBox> Container = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingSmall)
			  [SNew(SBorder)
					  .BorderImage(FPoolManagerStyles::GetWhiteBrush())
					  .BorderBackgroundColor(FPoolManagerStyles::GetHeaderBackground())
					  .Padding(FPoolManagerStyles::PaddingNone)
						  [SNew(SVerticalBox)

							  // Header Row
							  + SVerticalBox::Slot().AutoHeight().Padding(FMargin(FPoolManagerStyles::PaddingMedium))
								  [SNew(SHorizontalBox)
									  + SHorizontalBox::Slot()
										  .AutoWidth()
										  .VAlign(VAlign_Center)
										  .Padding(0.0f, 0.0f, 8.0f,
											  0.0f)
											  [SNew(SImage)
													  .Image(FAppStyle::GetBrush("Icons.Layout"))
													  .ColorAndOpacity(FPoolManagerStyles::GetAccentOrange())
													  .DesiredSizeOverride(FVector2D(16.0f, 16.0f))]
									  + SHorizontalBox::Slot().FillWidth(
										  1.0f)
										  [SNew(STextBlock)
												  .Text(LOCTEXT("AllPoolsTitle", "ALL POOLS"))
												  .Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
												  .ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())]]

							  // Separator
							  + SVerticalBox::Slot().AutoHeight()
								  [SNew(SBox).HeightOverride(
									  1.0f)
										  [SNew(SImage)
												  .Image(FPoolManagerStyles::GetWhiteBrush())
												  .ColorAndOpacity(FPoolManagerStyles::GetBorderColor())]]]];

	// Assign the dynamic content box
	Container->AddSlot().FillHeight(1.0f)[SAssignNew(AllPoolsContentBox, SBox)];

	// Initial population
	RefreshPoolList();

	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetContainerBackground())
		.Padding(FPoolManagerStyles::PaddingNone)
			[Container];
}

void SPoolDashboardPanel::RefreshPoolList()
{
	if (!AllPoolsContentBox.IsValid())
	{
		return;
	}

	TSharedPtr<SWidget> Content;

	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		const TArray<TSubclassOf<AActor>>& PooledClasses = Subsystem->GetAllPooledClasses();

		if (PooledClasses.Num() > 0)
		{
			// Responsiveness: SWrapBox
			const TSharedRef<SWrapBox> Grid =
				SNew(SWrapBox)
					.Orientation(Orient_Horizontal)
					.HAlign(HAlign_Left) // Align the grid itself to the left
					.InnerSlotPadding(FVector2D(FPoolManagerStyles::PaddingSmall,
						FPoolManagerStyles::PaddingSmall));

			// Use available width - Correct API for modern engine versions
			Grid->SetUseAllottedSize(true);

			for (const TSubclassOf<AActor>& PoolClass : PooledClasses)
			{
				if (PoolClass)
				{
					Grid->AddSlot()
						.Padding(FPoolManagerStyles::PaddingSmall)
						.VAlign(VAlign_Top)[
							// Use SPoolCard with TAttribute bindings for live data updates
							SNew(SPoolCard).PoolClass(PoolClass).Subsystem(GetPoolSubsystem())];
				}
			}
			Content = Grid;
		}
		else
		{
			// Empty State
			Content = SNew(SBox)
						  .HAlign(HAlign_Center)
						  .VAlign(VAlign_Center)
						  .Padding(FPoolManagerStyles::PaddingLarge)
							  [SNew(STextBlock)
									  .Text(LOCTEXT("NoActivePools", "No active pools found in current session."))
									  .Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeBody))
									  .ColorAndOpacity(FPoolManagerStyles::GetTextMuted())];
		}
	}
	else
	{
		// Not in PIE State
		Content = SNew(SBox)
					  .HAlign(HAlign_Center)
					  .VAlign(VAlign_Center)
					  .Padding(FPoolManagerStyles::PaddingLarge)
						  [SNew(STextBlock)
								  .Text(LOCTEXT("PoolsRequirePIE", "Start Play-In-Editor (PIE) to view active pools."))
								  .Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeBody))
								  .ColorAndOpacity(FPoolManagerStyles::GetTextMuted())];
	}

	AllPoolsContentBox->SetContent(Content.ToSharedRef());
}

void SPoolDashboardPanel::LogActivity(const FText& Message, const FLinearColor& Color)
{
	ActivityLog.Insert(FPoolActivityItem(Message, Color), 0);
	// Keep max 10
	if (ActivityLog.Num() > 10)
	{
		ActivityLog.RemoveAt(10, ActivityLog.Num() - 10);
	}
}

// ==================== UI BUILDERS ====================

TSharedRef<SWidget> SPoolDashboardPanel::BuildGlobalStatsSection()
{
	// 6-Card Grid matching React "stats-grid"
	// Row 1: Active Pools, Total Actors, In Use
	// Row 2: Avg Cache Hit, Memory Usage, Potential Leaks

	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetContainerBackground())
		.Padding(FPoolManagerStyles::PaddingNone)
			[SNew(SVerticalBox)

				// Section Header
				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FPoolManagerStyles::PaddingNone)
						[SNew(SBorder)
								.BorderImage(FPoolManagerStyles::GetWhiteBrush())
								.BorderBackgroundColor(FPoolManagerStyles::GetHeaderBackground())
								.Padding(FPoolManagerStyles::PaddingNone)
									[SNew(SVerticalBox)

										// Header Row (Icon + Text)
										+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(FMargin(FPoolManagerStyles::PaddingMedium))
												[SNew(SHorizontalBox)

													+ SHorizontalBox::Slot()
														.AutoWidth()
														.VAlign(VAlign_Center)
														.Padding(FMargin(0, 0, FPoolManagerStyles::PaddingSmall, 0))
															[SNew(SImage)
																	.Image(FAppStyle::GetBrush("Icons.Layout"))
																	.ColorAndOpacity(FPoolManagerStyles::GetAccentBlue())
																	.DesiredSizeOverride(FVector2D(16.0f, 16.0f))]

													+ SHorizontalBox::Slot()
														.FillWidth(1.0f)
														.VAlign(VAlign_Center)
															[SNew(STextBlock)
																	.Text(LOCTEXT("GlobalStatsTitle", "GLOBAL STATISTICS"))
																	.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
																	.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())]]

										// Bottom Separator
										+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(0.0f, 0.0f, 0.0f, 0.0f)
												[SNew(SBox)
														.HeightOverride(1.0f)
															[SNew(SImage)
																	.Image(FPoolManagerStyles::GetWhiteBrush())
																	.ColorAndOpacity(FPoolManagerStyles::GetBorderColor())]]]]

				// Stats Grid
				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FPoolManagerStyles::PaddingMedium)
						[SNew(SVerticalBox)

							// Row 1
							+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 0.0f, 0.0f, 12.0f)
									[SNew(SHorizontalBox)

										// Active Pools
										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.Padding(0.0f, 0.0f, 12.0f, 0.0f)
												[BuildStatCardWidget(
													TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
														this, &SPoolDashboardPanel::GetActivePoolsText)),
													LOCTEXT("ActivePoolsLabel", "ACTIVE POOLS"), FPoolManagerStyles::GetTextPrimary())]

										// Total Actors
										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.Padding(0.0f, 0.0f, 12.0f, 0.0f)
												[BuildStatCardWidget(
													TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
														this, &SPoolDashboardPanel::GetTotalActorsText)),
													LOCTEXT("TotalActorsLabel", "TOTAL ACTORS"), FPoolManagerStyles::GetTextPrimary())]

										// In Use
										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
												[BuildStatCardWidget(
													TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
														this, &SPoolDashboardPanel::GetActorsInUseText)),
													LOCTEXT("ActorsInUseLabel", "IN USE"), FPoolManagerStyles::GetTextPrimary())]]

							// Row 2
							+ SVerticalBox::Slot().AutoHeight()
								[SNew(SHorizontalBox)

									// Avg Cache Hit (Green)
									+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 12.0f, 0.0f)
										[BuildStatCardWidget(
											TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
												this, &SPoolDashboardPanel::GetAvgCacheHitText)),
											LOCTEXT("AvgCacheHitLabel", "AVG CACHE HIT"), FPoolManagerStyles::GetAccentGreen())]

									// Memory Usage (Blue)
									+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 12.0f, 0.0f)
										[BuildStatCardWidget(
											TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(
												this, &SPoolDashboardPanel::GetMemoryUsageText)),
											LOCTEXT("MemoryUsageLabel", "MEMORY USAGE"), FPoolManagerStyles::GetAccentBlue())]

									// Potential Leaks (Orange)
									+ SHorizontalBox::Slot().FillWidth(1.0f)
										[BuildStatCardWidget(
											TAttribute<FText>::Create(
												TAttribute<FText>::FGetter::CreateSP(this, &SPoolDashboardPanel::GetLeakCountText)),
											LOCTEXT("LeakCountLabel", "POTENTIAL LEAKS"), FPoolManagerStyles::GetAccentOrange())]]]];
}

TSharedRef<SWidget> SPoolDashboardPanel::BuildStatCard(const FText& Value, const FText& Label,
	const FLinearColor& ValueColor)
{
	// This overload is for static text - kept for compatibility
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetBorderColor())
		.Padding(FMargin(
			1.0f))
			[SNew(SBorder)
					.BorderImage(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush("PoolManager.CardGradient"))
					.BorderBackgroundColor(FLinearColor::White)
					.Padding(FPoolManagerStyles::PaddingLarge)
						[SNew(SVerticalBox)

							+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								.Padding(0.0f, 0.0f, 0.0f, 4.0f)
									[SNew(STextBlock)
											.Text(Value)
											.Font(FPoolManagerStyles::GetFont(
												"Bold", 20)) // Reduced from 24 to 20
											.ColorAndOpacity(ValueColor)]

							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[SNew(STextBlock)
										.Text(Label)
										.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeSmall))
										.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())]]];
}

TSharedRef<SWidget> SPoolDashboardPanel::BuildStatCardWidget(const TAttribute<FText>& Value, const FText& Label,
	const FLinearColor& ValueColor)
{
	// This overload is for dynamic/bound text
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetBorderColor())
		.Padding(FMargin(
			1.0f))
			[SNew(SBorder)
					.BorderImage(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush("PoolManager.CardGradient"))
					.BorderBackgroundColor(FLinearColor::White)
					.Padding(FPoolManagerStyles::PaddingLarge)
					.VAlign(VAlign_Center)
						[SNew(SVerticalBox)

							+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingTiny)
									[SNew(STextBlock)
											.Text(Value)
											.Font(FPoolManagerStyles::GetFont("Bold", 20)) // Reduced from 24 to 20
											.ColorAndOpacity(ValueColor)]

							+ SVerticalBox::Slot()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
									[SNew(STextBlock)
											.Text(Label)
											.Font(FPoolManagerStyles::GetFont(
												"Regular", FPoolManagerStyles::FontSizeSmall))
											.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())]]];
}

TSharedRef<SWidget> SPoolDashboardPanel::BuildPoolHealthGrid()
{
	// Placeholder - will be populated with live pool data
	return SNew(STextBlock)
		.Text(LOCTEXT("PoolGridPlaceholder", "Pool grid coming soon"))
		.ColorAndOpacity(FPoolManagerStyles::GetTextMuted());
}

TSharedRef<SWidget> SPoolDashboardPanel::BuildNoPIEBanner()
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetAccentBlue())
		.Padding(FPoolManagerStyles::PaddingMedium)
			[SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Info")).ColorAndOpacity(FLinearColor::White)]

				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[SNew(STextBlock)
							.Text(LOCTEXT("NoPIEBanner", "Start Play In Editor (PIE) to see live pool statistics"))
							.Font(FPoolManagerStyles::GetFont("Regular", FPoolManagerStyles::FontSizeBody))
							.ColorAndOpacity(FLinearColor::White)]];
}

// ==================== DATA GETTERS ====================

FText SPoolDashboardPanel::GetActivePoolsText() const
{
	if (!IsInPIE())
	{
		return LOCTEXT("NAValue", "--");
	}
	return FText::AsNumber(CachedActivePoolCount);
}

FText SPoolDashboardPanel::GetTotalActorsText() const
{
	if (!IsInPIE())
	{
		return LOCTEXT("NAValue", "--");
	}
	return FText::AsNumber(CachedTotalActors);
}

FText SPoolDashboardPanel::GetActorsInUseText() const
{
	if (!IsInPIE())
	{
		return LOCTEXT("NAValue", "--");
	}
	return FText::AsNumber(CachedActorsInUse);
}

FText SPoolDashboardPanel::GetAvgCacheHitText() const
{
	if (!IsInPIE())
	{
		return LOCTEXT("NAValue", "--");
	}
	return FText::Format(LOCTEXT("CacheHitFormat", "{0}%"), FText::AsNumber(FMath::RoundToInt(CachedAvgCacheHit)));
}

FSlateColor SPoolDashboardPanel::GetAvgCacheHitColor() const
{
	if (CachedAvgCacheHit >= 90.0f)
	{
		return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetAccentGreen());
	}
	if (CachedAvgCacheHit >= 70.0f)
	{
		return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetAccentYellow());
	}
	return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetAccentOrange());
}

FText SPoolDashboardPanel::GetMemoryUsageText() const
{
	if (!IsInPIE())
	{
		return LOCTEXT("NAValue", "--");
	}
	return FText::Format(LOCTEXT("MemoryFormat", "{0} MB"),
		FText::AsNumber(FMath::RoundToFloat(CachedMemoryMB * 10.0f) / 10.0f));
}

FText SPoolDashboardPanel::GetLeakCountText() const
{
	if (!IsInPIE())
	{
		return LOCTEXT("NAValue", "--");
	}
	return FText::AsNumber(CachedLeakCount);
}

EVisibility SPoolDashboardPanel::GetNoPIEBannerVisibility() const
{
	return IsInPIE() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SPoolDashboardPanel::GetStatsContentVisibility() const
{
	return EVisibility::Visible; // Always show, but with "--" values when not in PIE
}

// ==================== ACTIONS ====================

FReply SPoolDashboardPanel::OnPrewarmAllClicked()
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		const TArray<TSubclassOf<AActor>>& PooledClasses = Subsystem->GetAllPooledClasses();
		for (const TSubclassOf<AActor>& PoolClass : PooledClasses)
		{
			Subsystem->PrewarmPool(PoolClass, 10); // Prewarm each pool with 10 actors
		}
		UE_LOG(LogTemp, Log, TEXT("[Pool Manager] Prewarmed all pools"));
		LogActivity(LOCTEXT("LogPrewarm", "Prewarmed all pools"), FPoolManagerStyles::GetAccentBlue());
	}
	return FReply::Handled();
}

FReply SPoolDashboardPanel::OnShrinkUnusedClicked()
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		Subsystem->ShrinkAllPools();
		UE_LOG(LogTemp, Log, TEXT("[Pool Manager] Shrunk all pools"));
		LogActivity(LOCTEXT("LogShrink", "Shrunk unused actors from all pools"), FPoolManagerStyles::GetAccentOrange());
	}
	return FReply::Handled();
}

FReply SPoolDashboardPanel::OnScanLeaksClicked()
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		// Scan all pooled classes for leaks
		const TArray<TSubclassOf<AActor>>& PooledClasses = Subsystem->GetAllPooledClasses();
		int32							   TotalLeaks = 0;
		for (const TSubclassOf<AActor>& PoolClass : PooledClasses)
		{
			TArray<FLeakedActorInfo> Leaks = Subsystem->ScanForLeaks(PoolClass, 30.0f);
			TotalLeaks += Leaks.Num();
		}
		CachedLeakCount = TotalLeaks;
		UE_LOG(LogTemp, Log, TEXT("[Pool Manager] Scanned for leaks - found %d potential leaks"), TotalLeaks);

		if (TotalLeaks > 0)
		{
			LogActivity(FText::Format(LOCTEXT("LogLeaksFound", "Scan detected {0} potential leaks"),
							FText::AsNumber(TotalLeaks)),
				FPoolManagerStyles::GetAccentRed());
		}
		else
		{
			LogActivity(LOCTEXT("LogNoLeaks", "Scan complete - no leaks found"), FPoolManagerStyles::GetAccentGreen());
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
