// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolCard.h"
#include "FLazyGenericDynamicObjectPoolStyle.h"
#include "PoolManagerStyles.h"
#include "Styling/AppStyle.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPoolCard"

void SPoolCard::Construct(const FArguments& InArgs)
{
	PoolClass = InArgs._PoolClass;
	CachedSubsystem = InArgs._Subsystem;
	CardWidth = InArgs._CardWidth;
	CardHeight = InArgs._CardHeight;

	ChildSlot
	[
		SNew(SBox).WidthOverride(CardWidth).HeightOverride(CardHeight)
		[
			SNew(SBorder)
			.BorderImage(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush("PoolManager.RoundedBorder"))
			.BorderBackgroundColor(FPoolManagerStyles::GetBorderColor())
			.Padding(FMargin(1.0f))
			[
				SNew(SVerticalBox)

				// 1. Top Status Strip (3px)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(3.0f)
					[
						SNew(SImage)
						.Image(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush("PoolManager."
							"RoundedTop"))
						.ColorAndOpacity_Raw(this, &SPoolCard::GetStatusColor)
					]
				]

				// 2. Card Body
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBorder)
					.BorderImage(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush(
						"PoolManager.RoundedBottom"))
					.BorderBackgroundColor(FPoolManagerStyles::GetTertiaryBackground())
					.Padding(FMargin(14.0f, 10.0f))
					[
						SNew(SVerticalBox)

						// --- Header: Name & Status Dot ---
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
						[
							SNew(SHorizontalBox)

							// Class Name (Bold 12pt)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Raw(this, &SPoolCard::GetClassName)
								.Font(FPoolManagerStyles::GetFont("Bold", 12))
								.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
							]

							// Status Dot (10px filled circle)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(8.0f, 0.0f, 0.0f,
							         0.0f)
							[
								SNew(SBox).WidthOverride(10.0f).HeightOverride(
									10.0f)
								[
									SNew(SImage)
									.Image(FAppStyle::Get().GetBrush(
										"Icons.FilledCircle"))
									.ColorAndOpacity_Raw(
										this, &SPoolCard::GetStatusColor)
								]
							]
						]

						// --- Progress Section ---
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
						[
							SNew(SVerticalBox)

							// Usage Label Row
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(
									1.0f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("UsageLabel", "Usage"))
									.Font(FPoolManagerStyles::GetFont("Regular", 9))
									.ColorAndOpacity(
										FPoolManagerStyles::GetTextSecondary())
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(STextBlock)
									.Text_Raw(this, &SPoolCard::GetUsageText)
									.Font(FPoolManagerStyles::GetFont("Regular", 9))
									.ColorAndOpacity(
										FPoolManagerStyles::GetTextSecondary())
								]
							]

							// Progress Bar (6px, rounded) - using SProgressBar for smooth animation
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(6.0f)
								[
									SNew(SProgressBar)
									.Style(&FLazyGenericDynamicObjectPoolStyle::Get().GetWidgetStyle<FProgressBarStyle>(
										"PoolManager.RoundedProgressBar"))
									.Percent_Raw(this, &SPoolCard::GetUsagePercent)
									.FillColorAndOpacity_Raw(this, &SPoolCard::GetStatusColor)
									.BarFillType(EProgressBarFillType::LeftToRight)
								]
							]
						]

						// --- Stats Grid (Centered) ---
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						.VAlign(VAlign_Bottom)
						[
							SNew(SHorizontalBox)

							// Cache Hit Column (centered)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								.Padding(0.0f, 0.0f, 0.0f, 2.0f)
								[
									SNew(STextBlock)
									.Text_Raw(this, &SPoolCard::GetCacheHitText)
									.Font(FPoolManagerStyles::GetFont("Bold", 15))
									.ColorAndOpacity_Raw(
										this, &SPoolCard::GetCacheHitColor)
								]
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("CacheHitLabel", "CACHE HIT"))
									.Font(FPoolManagerStyles::GetFont("Regular", 9))
									.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
								]
							]

							// Memory Column (centered)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								.Padding(0.0f, 0.0f, 0.0f, 2.0f)
								[
									SNew(STextBlock)
									.Text_Raw(this, &SPoolCard::GetMemoryText)
									.Font(FPoolManagerStyles::GetFont("Bold", 15))
									.ColorAndOpacity(
										FPoolManagerStyles::GetTextPrimary())
								]
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("MemoryLabel", "MEMORY"))
									.Font(FPoolManagerStyles::GetFont("Regular", 9))
									.ColorAndOpacity(
										FPoolManagerStyles::GetTextMuted())
								]
							]
						]
					]
				]
			]
		]
	];
}

// ==================== LIVE DATA GETTERS ====================

ULazyDynamicObjectPoolSubsystem* SPoolCard::GetSubsystem() const
{
	return CachedSubsystem.IsValid() ? CachedSubsystem.Get() : nullptr;
}

FText SPoolCard::GetClassName() const
{
	if (!PoolClass) { return LOCTEXT("UnknownPool", "Unknown"); }

	FString Name = PoolClass->GetName();
	if (Name.EndsWith("_C")) { Name.RemoveFromEnd("_C"); }
	return FText::FromString(Name);
}

float SPoolCard::CalculateUsagePercent() const
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetSubsystem())
	{
		const int32 InUse = Subsystem->GetInUseActorsInPool(PoolClass).Num();
		const int32 Total = Subsystem->GetPoolSize(PoolClass);
		return Total > 0 ? FMath::Clamp(static_cast<float>(InUse) / static_cast<float>(Total), 0.0f, 1.0f) : 0.0f;
	}
	return 0.0f;
}

float SPoolCard::CalculateCacheHitRate() const
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetSubsystem()) { return Subsystem->GetCacheHitRate(PoolClass); }
	return 0.0f;
}

FText SPoolCard::GetUsageText() const
{
	if (const ULazyDynamicObjectPoolSubsystem* Subsystem = GetSubsystem())
	{
		const int32 InUse = Subsystem->GetInUseActorsInPool(PoolClass).Num();
		const int32 Total = Subsystem->GetPoolSize(PoolClass);
		return FText::Format(LOCTEXT("UsageFmt", "{0}/{1}"), FText::AsNumber(InUse), FText::AsNumber(Total));
	}
	return LOCTEXT("ZeroUsage", "0/0");
}

TOptional<float> SPoolCard::GetUsagePercent() const { return CalculateUsagePercent(); }

FText SPoolCard::GetCacheHitText() const
{
	const float HitRate = CalculateCacheHitRate();
	return FText::Format(LOCTEXT("HitFmt", "{0}%"), FText::AsNumber(FMath::RoundToInt(HitRate)));
}

FSlateColor SPoolCard::GetCacheHitColor() const
{
	const float HitRate = CalculateCacheHitRate();
	return FSlateColor(FPoolManagerStyles::GetCacheHitColor(HitRate));
}

FText SPoolCard::GetMemoryText() const
{
	const ULazyDynamicObjectPoolSubsystem* Subsystem = GetSubsystem();
	if (!IsValid(Subsystem)) { return LOCTEXT("ZeroMem", "0MB"); }

	const float MemMB = Subsystem->GetPoolMemoryUsage(PoolClass) / (1024.0f * 1024.0f);
	FNumberFormattingOptions MemFormat;
	MemFormat.MinimumFractionalDigits = 0;
	MemFormat.MaximumFractionalDigits = 1;
	return FText::Format(LOCTEXT("MemFmt", "{0}MB"), FText::AsNumber(MemMB, &MemFormat));
}

FSlateColor SPoolCard::GetStatusColor() const
{
	const float Usage = CalculateUsagePercent();
	return FSlateColor(FPoolManagerStyles::GetUsageStatusColor(Usage));
}

#undef LOCTEXT_NAMESPACE
