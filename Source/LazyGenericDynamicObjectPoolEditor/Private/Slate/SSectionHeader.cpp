// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SSectionHeader.h"
#include "PoolManagerStyles.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SSectionHeader::Construct(const FArguments& InArgs)
{
	ChildSlot[SNew(SBorder)
				  .BorderImage(FPoolManagerStyles::GetWhiteBrush())
				  .BorderBackgroundColor(FPoolManagerStyles::GetHeaderBackground())
				  .Padding(FPoolManagerStyles::PaddingNone)
					  [SNew(SVerticalBox)

					   // Header Row (Icon + Title + Optional Action)
					   + SVerticalBox::Slot().AutoHeight().Padding(FMargin(FPoolManagerStyles::PaddingMedium))
							 [SNew(SHorizontalBox)

							  // Icon
							  + SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(0.0f, 0.0f, 8.0f, 0.0f)[SNew(SImage)
																		 .Image(InArgs._Icon)
																		 .ColorAndOpacity(InArgs._IconColor)
																		 .DesiredSizeOverride(FVector2D(16.0f, 16.0f))]

							  // Title
							  + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
									[SNew(STextBlock)
										 .Text(InArgs._Title)
										 .Font(FPoolManagerStyles::GetFont("Bold", FPoolManagerStyles::FontSizeHeader))
										 .ColorAndOpacity(FPoolManagerStyles::GetTextSecondary())]

							  // Optional Action Content
							  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[InArgs._ActionContent.Widget]]

					   // Separator Line
					   + SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(
							 InArgs._ShowSeparator ? 1.0f : 0.0f)[SNew(SImage)
																	  .Image(FPoolManagerStyles::GetWhiteBrush())
																	  .ColorAndOpacity(
																		  FPoolManagerStyles::GetBorderColor())]]]];
}
