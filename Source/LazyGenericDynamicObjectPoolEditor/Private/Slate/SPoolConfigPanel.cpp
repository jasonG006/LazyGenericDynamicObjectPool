// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolConfigPanel.h"
#include "DeveloperSettings/LazyDynamicObjectPoolSettings.h"
#include "Editor.h"
#include "PoolManagerStyles.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPoolConfigPanel"

void SPoolConfigPanel::Construct(const FArguments& InArgs)
{
	SPoolPanelBase::Construct(SPoolPanelBase::FArguments());

	ChildSlot
	[
		SNew(SScrollBox)
		+
		SScrollBox::Slot().Padding(FPoolManagerStyles::PaddingMedium)
		[
			SNew(SVerticalBox)
			+
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingMedium)
			[
				BuildGeneralSettingsSection()
			]
			+
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingMedium)
			[
				BuildOptimizationSettingsSection()
			]
			+
			SVerticalBox::Slot().AutoHeight()
			[
				BuildDebuggingSettingsSection()
			]
		]
	];
}

ULazyDynamicObjectPoolSettings* SPoolConfigPanel::GetSettings() const
{
	return GetMutableDefault<ULazyDynamicObjectPoolSettings>();
}

void SPoolConfigPanel::SaveSettings()
{
	if (ULazyDynamicObjectPoolSettings* Settings = GetSettings())
	{
		Settings->SaveConfig();
		Settings->TryUpdateDefaultConfigFile();
	}
}

TSharedRef<SWidget> SPoolConfigPanel::BuildGeneralSettingsSection()
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
		.Padding(FPoolManagerStyles::PaddingMedium)
		[
			SNew(SVerticalBox)
			+
			SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingSmall)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("GeneralSettingsTitle", "General Configuration"))
				.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
				.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
			]

			// Default Initial Pool Size
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, FPoolManagerStyles::PaddingTiny)
			[
				SNew(SHorizontalBox)
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SBox).WidthOverride(
						200.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DefaultInitialSize", "Default Initial Pool Size"))
						.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
					]
				]
				+
				SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(
						100.0f)
					[
						SNew(SEditableTextBox)
						.Text(this, &SPoolConfigPanel::GetDefaultInitialPoolSizeText)
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
						{
							OnDefaultInitialPoolSizeChanged(Text);
						})
					]
				]
			]

			// Max Pool Size
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, FPoolManagerStyles::PaddingTiny)
			[
				SNew(SHorizontalBox)
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SBox).WidthOverride(
						200.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MaxSize", "Max Pool Size"))
						.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
					]
				]
				+
				SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(
						100.0f)
					[
						SNew(SEditableTextBox)
						.Text(this, &SPoolConfigPanel::GetMaxPoolSizeText)
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
						{
							OnMaxPoolSizeChanged(Text);
						})
					]
				]
			]

			// Growth Factor
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, FPoolManagerStyles::PaddingTiny)
			[
				SNew(SHorizontalBox)
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SBox).WidthOverride(
						200.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("GrowthFactor", "Pool Growth Factor"))
						.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
					]
				]
				+
				SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(
						100.0f)
					[
						SNew(SEditableTextBox)
						.Text(this, &SPoolConfigPanel::GetPoolGrowthFactorText)
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
						{
							OnPoolGrowthFactorChanged(Text);
						})
					]
				]
			]
		];
}

TSharedRef<SWidget> SPoolConfigPanel::BuildOptimizationSettingsSection()
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
		.Padding(FPoolManagerStyles::PaddingMedium)
		[
			SNew(SVerticalBox)
			+
			SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingSmall)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OptimizationSettingsTitle", "Optimization Settings"))
				.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
				.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
			]

			// Enable Auto Shrink
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, FPoolManagerStyles::PaddingTiny)
			[
				SNew(SHorizontalBox)
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SBox).WidthOverride(
						200.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("EnableAutoShrink", "Enable Auto Shrink"))
						.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
					]
				]
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(this, &SPoolConfigPanel::GetAutoShrinkState)
					.OnCheckStateChanged(this, &SPoolConfigPanel::OnAutoShrinkChanged)
				]
			]

			// Auto Shrink Interval
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, FPoolManagerStyles::PaddingTiny)
			[
				SNew(SHorizontalBox)
				.IsEnabled_Lambda([this]() { return GetAutoShrinkState() == ECheckBoxState::Checked; })
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SBox).WidthOverride(
						200.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ShrinkInterval", "Auto Shrink Interval (s)"))
						.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
					]
				]
				+
				SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(
						100.0f)
					[
						SNew(SEditableTextBox)
						.Text(this, &SPoolConfigPanel::GetAutoShrinkIntervalText)
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
						{
							OnAutoShrinkIntervalChanged(Text);
						})
					]
				]
			]

			// Shrink Threshold
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, FPoolManagerStyles::PaddingTiny)
			[
				SNew(SHorizontalBox)
				.IsEnabled_Lambda([this]() { return GetAutoShrinkState() == ECheckBoxState::Checked; })
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SBox).WidthOverride(
						200.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ShrinkThreshold", "Shrink Threshold (Ratio)"))
						.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
					]
				]
				+
				SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(
						100.0f)
					[
						SNew(SEditableTextBox)
						.Text(this, &SPoolConfigPanel::GetShrinkThresholdText)
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
						{
							OnShrinkThresholdChanged(Text);
						})
					]
				]
			]
		];
}

TSharedRef<SWidget> SPoolConfigPanel::BuildDebuggingSettingsSection()
{
	return SNew(SBorder)
		.BorderImage(FPoolManagerStyles::GetWhiteBrush())
		.BorderBackgroundColor(FPoolManagerStyles::GetCardBackground())
		.Padding(FPoolManagerStyles::PaddingMedium)
		[
			SNew(SVerticalBox)
			+
			SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, FPoolManagerStyles::PaddingSmall)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DebuggingSettingsTitle", "Debugging Settings"))
				.Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
				.ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())
			]

			// Detailed Logging
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, FPoolManagerStyles::PaddingTiny)
			[
				SNew(SHorizontalBox)
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingMedium, 0.0f)
				[
					SNew(SBox).WidthOverride(
						200.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("EnableLogging", "Enable Detailed Logging"))
						.ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
					]
				]
				+
				SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(this, &SPoolConfigPanel::GetDetailedLoggingState)
					.OnCheckStateChanged(this, &SPoolConfigPanel::OnDetailedLoggingChanged)
				]
			]
		];
}

// ==================== GETTERS/SETTERS ====================

FText SPoolConfigPanel::GetDefaultInitialPoolSizeText() const
{
	return FText::AsNumber(GetSettings()->DefaultInitialPoolSize);
}

void SPoolConfigPanel::OnDefaultInitialPoolSizeChanged(const FText& Text)
{
	if (NumericTypeIs<int32>(Text.ToString()))
	{
		GetSettings()->DefaultInitialPoolSize = FCString::Atoi(*Text.ToString());
		SaveSettings();
	}
}

FText SPoolConfigPanel::GetMaxPoolSizeText() const { return FText::AsNumber(GetSettings()->MaxPoolSize); }

void SPoolConfigPanel::OnMaxPoolSizeChanged(const FText& Text)
{
	if (NumericTypeIs<int32>(Text.ToString()))
	{
		GetSettings()->MaxPoolSize = FCString::Atoi(*Text.ToString());
		SaveSettings();
	}
}

FText SPoolConfigPanel::GetPoolGrowthFactorText() const { return FText::AsNumber(GetSettings()->PoolGrowthFactor); }

void SPoolConfigPanel::OnPoolGrowthFactorChanged(const FText& Text)
{
	if (NumericTypeIs<float>(Text.ToString()))
	{
		GetSettings()->PoolGrowthFactor = FCString::Atof(*Text.ToString());
		SaveSettings();
	}
}

ECheckBoxState SPoolConfigPanel::GetAutoShrinkState() const
{
	return GetSettings()->bEnableAutoShrink ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SPoolConfigPanel::OnAutoShrinkChanged(ECheckBoxState NewState)
{
	GetSettings()->bEnableAutoShrink = (NewState == ECheckBoxState::Checked);
	SaveSettings();
}

FText SPoolConfigPanel::GetAutoShrinkIntervalText() const { return FText::AsNumber(GetSettings()->AutoShrinkInterval); }

void SPoolConfigPanel::OnAutoShrinkIntervalChanged(const FText& Text)
{
	if (NumericTypeIs<float>(Text.ToString()))
	{
		GetSettings()->AutoShrinkInterval = FCString::Atof(*Text.ToString());
		SaveSettings();
	}
}

FText SPoolConfigPanel::GetShrinkThresholdText() const { return FText::AsNumber(GetSettings()->ShrinkThreshold); }

void SPoolConfigPanel::OnShrinkThresholdChanged(const FText& Text)
{
	if (NumericTypeIs<float>(Text.ToString()))
	{
		GetSettings()->ShrinkThreshold = FCString::Atof(*Text.ToString());
		SaveSettings();
	}
}

ECheckBoxState SPoolConfigPanel::GetDetailedLoggingState() const
{
	return GetSettings()->bEnableDetailedLogging ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SPoolConfigPanel::OnDetailedLoggingChanged(ECheckBoxState NewState)
{
	GetSettings()->bEnableDetailedLogging = (NewState == ECheckBoxState::Checked);
	SaveSettings();
}

#undef LOCTEXT_NAMESPACE
