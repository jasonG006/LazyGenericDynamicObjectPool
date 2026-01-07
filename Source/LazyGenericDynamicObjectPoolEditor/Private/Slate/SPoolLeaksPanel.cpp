// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolLeaksPanel.h"
#include "Editor.h"
#include "PoolManagerStyles.h"
#include "Styling/AppStyle.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "SPoolLeaksPanel"

void SPoolLeaksPanel::Construct(const FArguments& InArgs)
{
	SPoolPanelBase::Construct(SPoolPanelBase::FArguments());

	// Initialize threshold options
	ThresholdOptions.Add(MakeShared<float>(1.0f));
	ThresholdOptions.Add(MakeShared<float>(5.0f));
	ThresholdOptions.Add(MakeShared<float>(10.0f));
	ThresholdOptions.Add(MakeShared<float>(30.0f));
	ThresholdOptions.Add(MakeShared<float>(60.0f));

	ChildSlot
	[
		SNew(SVerticalBox)

		// Control Bar
		+ SVerticalBox::Slot().AutoHeight().Padding(FPoolManagerStyles::PaddingMedium)
		[
			BuildControlBar()
		]

		// Leak List
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FPoolManagerStyles::PaddingMedium, 0.0f)
		[
			BuildLeakList()
		]

		// Footer
		+ SVerticalBox::Slot().AutoHeight().Padding(FPoolManagerStyles::PaddingMedium)
		[
			BuildActionFooter()
		]
	];
}

void SPoolLeaksPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPoolPanelBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Ticker logic if needed (auto-scan)
}

TSharedRef<SWidget> SPoolLeaksPanel::BuildControlBar()
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
		.Padding(FPoolManagerStyles::PaddingSmall)
		[
			SNew(SHorizontalBox)

			// Scan Button
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
				.ContentPadding(FMargin(16.0f, 6.0f))
				.OnClicked(this, &SPoolLeaksPanel::OnScanClicked)
				.IsEnabled_Lambda([this]() { return IsInPIE(); })
				[
					SNew(SHorizontalBox)
					+
					SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Search"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+
					SHorizontalBox::Slot().AutoWidth().VAlign(
						VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ScanBtn", "Scan for Leaks"))
						.Font(FPoolManagerStyles::GetFont("Bold", 10))
					]
				]
			]

			// Threshold Dropdown
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ThresholdLabel", "Min Time (s):"))
				.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
			]
			+
			SHorizontalBox::Slot().AutoWidth().VAlign(
				VAlign_Center)
			[
				SAssignNew(ThresholdComboBox, SComboBox<TSharedPtr<float>>)
				.OptionsSource(&ThresholdOptions)
				.OnGenerateWidget_Lambda([](TSharedPtr<float> Item)
				{
					return SNew(STextBlock).Text(FText::AsNumber(*Item));
				})
				.OnSelectionChanged(this, &SPoolLeaksPanel::OnThresholdChanged)
				[
					SNew(STextBlock).Text(this, &SPoolLeaksPanel::GetScanThresholdText)
				]
			]

			// Count badge (Right aligned)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FPoolManagerStyles::GetWhiteBrush())
				.BorderBackgroundColor(FPoolManagerStyles::GetLeakBadgeBackground())
				.Padding(FMargin(8.0f, 2.0f))
				.Visibility_Lambda(
					[this]()
					{
						return DetectedLeaks.Num() > 0
							       ? EVisibility::Visible
							       : EVisibility::Hidden;
					})
				[
					SNew(STextBlock)
					.Text(this, &SPoolLeaksPanel::GetLeakCountText)
					.ColorAndOpacity(FLinearColor::White)
					.Font(FPoolManagerStyles::GetFont("Bold", 10))
				]
			]
		];
}

TSharedRef<SWidget> SPoolLeaksPanel::BuildLeakList()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0.0f)
		[
			SAssignNew(LeakListView, SListView<TSharedPtr<FPoolLeakInfo>>)
			.ListItemsSource(&DetectedLeaks)
			.OnGenerateRow(this, &SPoolLeaksPanel::OnGenerateRow)
			.SelectionMode(ESelectionMode::Single)
		];
}

TSharedRef<ITableRow> SPoolLeaksPanel::OnGenerateRow(TSharedPtr<FPoolLeakInfo> Item,
                                                     const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FPoolLeakInfo>>, OwnerTable)
		.Padding(FMargin(8.0f, 6.0f))
		[
			SNew(SHorizontalBox)
			+
			SHorizontalBox::Slot().FillWidth(0.4f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->ActorName))
				.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
			]
			+
			SHorizontalBox::Slot().FillWidth(
				0.3f)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("TimeFormat", "{0}s"), FText::AsNumber(Item->TimeInUse)))
				.ColorAndOpacity(FPoolManagerStyles::GetAccentOrange())
			]
			+
			SHorizontalBox::Slot().FillWidth(
				0.3f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->ActorClass ? Item->ActorClass->GetName() : TEXT("Unknown")))
				.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())
			]
		];
}

TSharedRef<SWidget> SPoolLeaksPanel::BuildActionFooter()
{
	return SNew(SHorizontalBox)
		+
		SHorizontalBox::Slot().AutoWidth().Padding(
			0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
			.ContentPadding(FMargin(12.0f, 6.0f))
			.OnClicked(this, &SPoolLeaksPanel::OnReturnAllClicked)
			.IsEnabled_Lambda([this]() { return IsInPIE() && DetectedLeaks.Num() > 0; })
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ReturnAllBtn", "Return All Leaks to Pool"))
				.Font(FPoolManagerStyles::GetFont("Bold", 10))
			]
		]
		+
		SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "FlatButton")
			.ContentPadding(FMargin(12.0f, 6.0f))
			.OnClicked(this, &SPoolLeaksPanel::OnClearIgnoreListClicked)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ClearIgnoreBtn", "Clear Ignore List"))
				.Font(FPoolManagerStyles::GetFont("Regular", 10))
			]
		];
}

FReply SPoolLeaksPanel::OnScanClicked()
{
	if (ULazyDynamicObjectPoolSubsystem* Subsystem = GetPoolSubsystem())
	{
		DetectedLeaks.Empty();

		const TArray<TSubclassOf<AActor>>& PooledClasses = Subsystem->GetAllPooledClasses();
		for (const TSubclassOf<AActor>& PoolClass : PooledClasses)
		{
			TArray<FLeakedActorInfo> Leaks = Subsystem->ScanForLeaks(PoolClass, ScanThreshold);
			for (const FLeakedActorInfo& Leak : Leaks)
			{
				TSharedPtr<FPoolLeakInfo> LeakInfo = MakeShared<FPoolLeakInfo>();
				LeakInfo->Actor = Leak.Actor;
				LeakInfo->ActorName = Leak.ActorName;
				LeakInfo->ActorClass = Leak.ActorClass;
				LeakInfo->TimeInUse = Leak.TimeInUse;
				LeakInfo->Location = Leak.Location;
				DetectedLeaks.Add(LeakInfo);
			}
		}

		if (LeakListView.IsValid()) { LeakListView->RequestListRefresh(); }
	}
	else if (!IsInPIE())
	{
		// Mock data for editor preview
		DetectedLeaks.Empty();
		auto MockLeak = MakeShared<FPoolLeakInfo>();
		MockLeak->ActorName = "BP_Projectile_C_23";
		MockLeak->TimeInUse = 45.2f;
		DetectedLeaks.Add(MockLeak);
		LeakListView->RequestListRefresh();
	}

	return FReply::Handled();
}

FReply SPoolLeaksPanel::OnReturnAllClicked() { return FReply::Handled(); }
FReply SPoolLeaksPanel::OnClearIgnoreListClicked() { return FReply::Handled(); }

FText SPoolLeaksPanel::GetScanThresholdText() const { return FText::AsNumber(ScanThreshold); }

void SPoolLeaksPanel::OnThresholdChanged(TSharedPtr<float> NewValue, ESelectInfo::Type SelectInfo)
{
	if (NewValue.IsValid()) { ScanThreshold = *NewValue; }
}

FText SPoolLeaksPanel::GetLeakCountText() const
{
	return FText::Format(LOCTEXT("LeakCountFmt", "{0} Leaks Found"), FText::AsNumber(DetectedLeaks.Num()));
}

#undef LOCTEXT_NAMESPACE
