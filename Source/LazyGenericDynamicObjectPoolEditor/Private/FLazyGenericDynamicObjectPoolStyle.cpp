// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "FLazyGenericDynamicObjectPoolStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FLazyGenericDynamicObjectPoolStyle::StyleInstance = nullptr;

void FLazyGenericDynamicObjectPoolStyle::Initialize()
{
	if (StyleInstance.IsValid())
	{
		return;
	}

	StyleInstance = Create();
	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FLazyGenericDynamicObjectPoolStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

void FLazyGenericDynamicObjectPoolStyle::ReloadTextures()
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FLazyGenericDynamicObjectPoolStyle::Get()
{
	return *StyleInstance;
}

FName FLazyGenericDynamicObjectPoolStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("FLazyGenericDynamicObjectPoolStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef<FSlateStyleSet> FLazyGenericDynamicObjectPoolStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("FLazyGenericDynamicObjectPoolStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("LazyGenericDynamicObjectPool")->GetBaseDir() / TEXT("Resources"));

	Style->Set("LazyGenericPoolWindow.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	// Create a rounded border brush
	const FSlateBrush* BorderBrush = new FSlateRoundedBoxBrush(FLinearColor::White, 4.0f);

	// Create a button style using the rounded border brush
	const FButtonStyle ButtonStyle = FButtonStyle()
										 .SetNormal(*BorderBrush)
										 .SetHovered(FSlateRoundedBoxBrush(FLinearColor::White, 4.0f))
										 .SetPressed(FSlateRoundedBoxBrush(FLinearColor::White, 4.0f))
										 .SetNormalPadding(FMargin(2.0f));

	Style->Set("WhiteRoundedButton", ButtonStyle);

	// Define the progress bar style
	const FProgressBarStyle ProgressBarStyle =
		FProgressBarStyle()
			.SetBackgroundImage(FSlateColorBrush(FLinearColor(FColor(0, 0, 0, 40))))
			.SetFillImage(FSlateColorBrush(FLinearColor(1.0f, 0.5f, 0.0f)));

	Style->Set("OrangeFillProgressBar", ProgressBarStyle);

	// Gradient Header
	Style->Set("PoolManager.HeaderGradient", new IMAGE_BRUSH("HeaderGradient", FVector2D(128.0f, 128.0f)));
	Style->Set("PoolManager.CardGradient", new IMAGE_BRUSH("CardGradient", FVector2D(256.0f, 256.0f)));

	// Rounded Border Brush (4px)
	Style->Set("PoolManager.RoundedBorder", new FSlateRoundedBoxBrush(FLinearColor::White, 4.0f));
	Style->Set("PoolManager.RoundedTop",
		new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(4.0f, 4.0f, 0.0f, 0.0f)));
	Style->Set("PoolManager.RoundedBottom",
		new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(0.0f, 0.0f, 4.0f, 4.0f)));

	Style->Set("PoolManager.RoundedProgress", new FSlateRoundedBoxBrush(FLinearColor::White, 2.0f));

	// Progress bar gradient (horizontal fade - white to grey for multiply effect)
	Style->Set("PoolManager.ProgressGradient", new IMAGE_BRUSH("HeaderGradient", FVector2D(128.0f, 8.0f)));

	// Rounded Progress Bar Style for SProgressBar
	// Uses FSlateRoundedBoxBrush for smooth rounded corners on both background and fill
	const FProgressBarStyle RoundedProgressBarStyle = FProgressBarStyle()
														  .SetBackgroundImage(FSlateRoundedBoxBrush(
															  FLinearColor::FromSRGBColor(FColor::FromHex("1a1a1a")), // Dark background (GetSecondaryBackground)
															  3.0f))												  // 3px corner radius
														  .SetFillImage(FSlateRoundedBoxBrush(
															  FLinearColor::White, // White fill - color applied via ColorAndOpacity
															  3.0f));			   // 3px corner radius

	Style->Set("PoolManager.RoundedProgressBar", RoundedProgressBarStyle);

	return Style;
}
