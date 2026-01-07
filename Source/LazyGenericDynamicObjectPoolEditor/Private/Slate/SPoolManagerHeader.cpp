// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolManagerHeader.h"
#include "Editor.h"
#include "FLazyGenericDynamicObjectPoolStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "PoolManagerStyles.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPoolManagerHeader"

// Static member initialization
FString SPoolManagerHeader::CachedVersion;

void SPoolManagerHeader::Construct(const FArguments& InArgs)
{
	// Pre-cache version
	if (CachedVersion.IsEmpty()) { CachedVersion = GetPluginVersion(); }

	// Wrap in VerticalBox to add the bottom separator line
	ChildSlot
		[SNew(SVerticalBox)

		 // Main Header Content
		 +
		 SVerticalBox::Slot().AutoHeight()
			 [SNew(SBorder)
				  .BorderImage(FLazyGenericDynamicObjectPoolStyle::Get().GetBrush("PoolManager.HeaderGradient"))
				  .BorderBackgroundColor(FLinearColor::White)
				  .Padding(FMargin(FPoolManagerStyles::PaddingMedium, FPoolManagerStyles::PaddingSmall))
					  [SNew(SHorizontalBox)

					   // ==================== LOGO ====================
					   + SHorizontalBox::Slot()
							 .AutoWidth()
							 .VAlign(VAlign_Center)
							 .Padding(0.0f, 0.0f, FPoolManagerStyles::PaddingSmall,
									  FPoolManagerStyles::PaddingNone)[SNew(SBox).WidthOverride(32.0f).HeightOverride(
								 32.0f)[SNew(SBorder)
											.BorderImage(FPoolManagerStyles::GetWhiteBrush())
											.BorderBackgroundColor(FPoolManagerStyles::GetAccentBlue())
											.Padding(6.0f)
											.HAlign(HAlign_Center)
											.VAlign(VAlign_Center)[SNew(SImage)
																	   .Image(FAppStyle::GetBrush("Icons.Refresh"))
																	   .ColorAndOpacity(
																		   FPoolManagerStyles::GetTextPrimary())
																	   .DesiredSizeOverride(FVector2D(16.0f, 16.0f))]]]

					   // ==================== TITLE + SUBTITLE ====================
					   + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							 [SNew(SHorizontalBox)

							  // Title
							  + SHorizontalBox::Slot().AutoWidth()
									[SNew(STextBlock)
										 .Text(LOCTEXT("HeaderTitle", "LAZY GENERIC OBJECT POOL MANAGER"))
										 .Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
										 .ColorAndOpacity(FPoolManagerStyles::GetTextPrimary())]

							  + SHorizontalBox::Slot().AutoWidth()[SNew(SSpacer).Size(FVector2D(10.0f, 0.0f))]

							  // Subtitle (version + PIE status)
							  + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f, 0.0f, 0.0f)
									[SNew(SHorizontalBox)

									 // Version text
									 + SHorizontalBox::Slot()
										   .AutoWidth()[SNew(STextBlock)
															.Text(this, &SPoolManagerHeader::GetVersionText)
															.Font(FPoolManagerStyles::GetFont(
																"Regular", FPoolManagerStyles::FontSizeSmall))
															.ColorAndOpacity(FPoolManagerStyles::GetTextMuted())]

									 // PIE indicator dot
									 + SHorizontalBox::Slot()
										   .AutoWidth()
										   .VAlign(VAlign_Center)
										   .Padding(FPoolManagerStyles::PaddingSmall, FPoolManagerStyles::PaddingNone,
													FPoolManagerStyles::PaddingNone, FPoolManagerStyles::PaddingNone)
											   [SNew(SBox).WidthOverride(8.0f).HeightOverride(
												   8.0f)[SNew(SImage)
															 .Image(FPoolManagerStyles::GetWhiteBrush())
															 .ColorAndOpacity(
																 this, &SPoolManagerHeader::GetPIEIndicatorColor)]]]]

					   // ==================== ACTION BUTTONS ====================

					   // Help Button
					   + SHorizontalBox::Slot()
							 .AutoWidth()
							 .VAlign(VAlign_Center)
							 .Padding(FPoolManagerStyles::PaddingTiny, FPoolManagerStyles::PaddingNone)
								 [SNew(SButton)
									  .ButtonStyle(FAppStyle::Get(), "SimpleButton")
									  .ContentPadding(FPoolManagerStyles::PaddingTiny)
									  .OnClicked(this, &SPoolManagerHeader::OnHelpButtonClicked)
									  .ToolTipText(LOCTEXT("HelpTooltip", "Open Help & Documentation"))
										  [SNew(SImage)
											   .Image(FAppStyle::GetBrush("Icons.Help"))
											   .ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())
											   .DesiredSizeOverride(FVector2D(16.0f, 16.0f))]]]]

		 // Bottom Separator Line (Border Bottom)
		 + SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(
			   1.0f)[SNew(SImage)
						 .Image(FPoolManagerStyles::GetWhiteBrush())
						 .ColorAndOpacity(FPoolManagerStyles::GetBorderColor())]]];
}

void SPoolManagerHeader::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Track PIE state changes (no-op for now, but could trigger animations)
	const bool bCurrentlyInPIE = IsInPIE();
	if (bCurrentlyInPIE != bWasInPIE)
	{
		bWasInPIE = bCurrentlyInPIE;
		// Could trigger a refresh here if needed
	}
}

FText SPoolManagerHeader::GetVersionText() const
{
	const FString Status = IsInPIE() ? TEXT("PIE Active") : TEXT("Editor");
	return FText::Format(LOCTEXT("VersionFormat", "v{0} • {1}"), FText::FromString(CachedVersion),
						 FText::FromString(Status));
}

FSlateColor SPoolManagerHeader::GetPIEIndicatorColor() const
{
	if (IsInPIE()) { return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetAccentGreen()); }
	return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetTextMuted());
}

FReply SPoolManagerHeader::OnHelpButtonClicked()
{
	// TODO: Implement help dialog with:
	// - Quick start guide
	// - Common use cases
	// - Links to documentation
	// - Keyboard shortcuts

	// For now, just log that it was clicked
	UE_LOG(LogTemp, Log, TEXT("[Pool Manager] Help button clicked - dialog coming soon"));

	return FReply::Handled();
}

FString SPoolManagerHeader::GetPluginVersion()
{
	if (!CachedVersion.IsEmpty()) { return CachedVersion; }

	IPluginManager& PluginManager = IPluginManager::Get();
	TSharedPtr<IPlugin> Plugin = PluginManager.FindPlugin(TEXT("LazyGenericDynamicObjectPool"));

	if (Plugin.IsValid())
	{
		const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
		CachedVersion = Descriptor.VersionName;
		return CachedVersion;
	}

	// Fallback
	CachedVersion = TEXT("1.0");
	return CachedVersion;
}

bool SPoolManagerHeader::IsInPIE() const { return GEditor && GEditor->PlayWorld != nullptr; }

#undef LOCTEXT_NAMESPACE
