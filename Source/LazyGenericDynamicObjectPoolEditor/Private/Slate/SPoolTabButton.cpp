// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolTabButton.h"
#include "PoolManagerStyles.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPoolTabButton"

void SPoolTabButton::Construct(const FArguments& InArgs)
{
	TabIndex = InArgs._TabIndex;
	Label = InArgs._Label;
	IconBrush = InArgs._Icon;
	BadgeCount = InArgs._BadgeCount;
	IsActive = InArgs._IsActive;
	OnTabClicked = InArgs._OnTabClicked;

	ChildSlot
		[SNew(SButton)
			 .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			 .ContentPadding(FMargin(FPoolManagerStyles::PaddingSmall, FPoolManagerStyles::PaddingNone))
			 .OnClicked(this, &SPoolTabButton::HandleButtonClicked)
			 .OnHovered_Lambda([this]() { bIsHovered = true; })
			 .OnUnhovered_Lambda([this]() { bIsHovered = false; })
				 [SNew(SOverlay)

				  // 1. Background & Content Container
				  +
				  SOverlay::Slot()
					  [SNew(SBorder)
						   .BorderImage(FPoolManagerStyles::GetWhiteBrush())
						   .BorderBackgroundColor(this, &SPoolTabButton::GetBackgroundColor)
						   .Padding(FMargin(FPoolManagerStyles::PaddingLarge, FPoolManagerStyles::PaddingMedium))
						   .HAlign(HAlign_Fill)
						   .VAlign(VAlign_Fill)
							   [SNew(SHorizontalBox)

								// Icon
								+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .VAlign(VAlign_Center)
									  .Padding(0.0f, 0.0f, IconBrush ? 6.0f : 0.0f, 0.0f)
										  [SNew(SImage)
											   .Image(IconBrush)
											   .Visibility(IconBrush ? EVisibility::Visible : EVisibility::Collapsed)
											   .ColorAndOpacity(this, &SPoolTabButton::GetIconColor)
											   .DesiredSizeOverride(FVector2D(14.0f, 14.0f))]

								// Label
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									  [SNew(STextBlock)
										   .Text(Label)
										   .Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeBody))
										   .ColorAndOpacity(this, &SPoolTabButton::GetTextColor)]]]

				  // 2. Active Selection Indicator
				  + SOverlay::Slot().VAlign(VAlign_Bottom)[SNew(SBox).HeightOverride(
						2.0f)[SNew(SImage)
								  .Image(FPoolManagerStyles::GetWhiteBrush())
								  .ColorAndOpacity(FPoolManagerStyles::GetTabActiveAccent())
								  .Visibility(this, &SPoolTabButton::GetUnderlineVisibility)]]

				  // 3. Badge overlay
				  + SOverlay::Slot()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Top)
						.Padding(FMargin(0.0f, 2.0f, 2.0f, 0.0f))
							[SNew(SBox)
								 .Visibility(this, &SPoolTabButton::GetBadgeVisibility)
								 .WidthOverride(16.0f)
								 .HeightOverride(
									 16.0f)[SNew(SBorder)
												.BorderImage(FPoolManagerStyles::GetWhiteBrush())
												.BorderBackgroundColor(FPoolManagerStyles::GetLeakBadgeBackground())
												.Padding(FMargin(0.0f))
												.HAlign(HAlign_Center)
												.VAlign(VAlign_Center)[SNew(STextBlock)
																		   .Text(this, &SPoolTabButton::GetBadgeText)
																		   .Font(FPoolManagerStyles::GetFont("Bold", 8))
																		   .ColorAndOpacity(
																			   FPoolManagerStyles::GetTextPrimary())
																		   .Justification(ETextJustify::Center)]]]]];
}

FReply SPoolTabButton::HandleButtonClicked()
{
	if (OnTabClicked.IsBound()) { return OnTabClicked.Execute(); }
	return FReply::Handled();
}

FSlateColor SPoolTabButton::GetTextColor() const
{
	if (IsActive.Get()) { return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetTabActiveText()); }
	return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetTabInactiveText());
}

FSlateColor SPoolTabButton::GetIconColor() const
{
	return GetTextColor(); // Same as text
}

EVisibility SPoolTabButton::GetUnderlineVisibility() const
{
	return IsActive.Get() ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility SPoolTabButton::GetBadgeVisibility() const
{
	return BadgeCount.Get() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SPoolTabButton::GetBadgeText() const
{
	const int32 Count = BadgeCount.Get();
	if (Count > 99) { return LOCTEXT("BadgeOverflow", "99+"); }
	return FText::AsNumber(Count);
}

FSlateColor SPoolTabButton::GetBackgroundColor() const
{
	if (IsActive.Get()) { return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetCardBackground()); }
	if (bIsHovered) { return FPoolManagerStyles::AsSlateColor(FPoolManagerStyles::GetHoverBackground()); }
	return FSlateColor(FLinearColor::Transparent);
}

#undef LOCTEXT_NAMESPACE
