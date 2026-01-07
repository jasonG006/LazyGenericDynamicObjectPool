// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SObjectPoolQuickAction.h"

#include "FLazyGenericDynamicObjectPoolStyle.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "PropertyCustomizationHelpers.h"
#include "Slate/ObjectPoolActionButton.h"
#include "Slate/SPoolSparkline.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"
#include "Widgets/Images/SLayeredImage.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "FLazyGenericDynamicObjectPoolEditorModule"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SObjectPoolQuickAction::Construct(const FArguments& InArgs)
{
    const FLinearColor DarkBackgroundColor = FLinearColor(FColor(190, 190, 190));
    const FLinearColor LightBackgroundColor = FLinearColor(FColor(230, 230, 230));
    constexpr FLinearColor LightTextColor = FLinearColor(0.431f, 0.431f, 0.431f);
    
    ChildSlot
    [
        SNew(SBorder)
        .Padding(10)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
        .BorderBackgroundColor(DarkBackgroundColor)
        [
            SNew(SVerticalBox)
            // Health Warning Banner (only visible when there's a warning)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 5)
            [
                SNew(SBorder)
                .Padding(FMargin(10, 5))
                .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                .BorderBackgroundColor_Lambda([this]()
                {
                    if (!SelectedClass) return FLinearColor(0, 0, 0, 0);
                    const UWorld* World = GEditor->PlayWorld;
                    if(!IsValid(World)) return FLinearColor(0, 0, 0, 0);
                    const EPoolHealthStatus Status = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolHealth(SelectedClass);
                    if (Status == EPoolHealthStatus::Critical) return FLinearColor(1.0f, 0.2f, 0.0f, 0.9f);
                    if (Status == EPoolHealthStatus::Warning) return FLinearColor(1.0f, 0.8f, 0.0f, 0.8f);
                    return FLinearColor(0, 0, 0, 0);
                })
                .Visibility(this, &SObjectPoolQuickAction::GetHealthWarningVisibility)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(0, 0, 10, 0)
                    [
                        SNew(SImage)
                        .Image(this, &SObjectPoolQuickAction::GetHealthStatusIcon)
                        .ColorAndOpacity(FLinearColor::White)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(this, &SObjectPoolQuickAction::GetHealthWarningText)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                        .ColorAndOpacity(FLinearColor::White)
                        .AutoWrapText(true)
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                // Actor Class Selector
                + SHorizontalBox::Slot()
                .FillWidth(0.35f)
                .Padding(FMargin(5,0,5,0))
                [
                    SNew(SBorder)
                    .Padding(FMargin(TopLevelContentPadding))
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .BorderBackgroundColor(LightBackgroundColor)
                    [
                        SNew(SClassPropertyEntryBox)
                        .MetaClass(AActor::StaticClass())
                        .SelectedClass(this, reinterpret_cast<TAttribute<const UClass*>::FGetter::TConstMethodPtr<SObjectPoolQuickAction>>(&
                        SObjectPoolQuickAction::GetSelectedActorClass))
                        .OnSetClass(this, &SObjectPoolQuickAction::OnActorClassSelected)
                    ]
                ]
                // Health Status Badge
                + SHorizontalBox::Slot()
                .FillWidth(0.15f)
                .Padding(FMargin(5,0,5,0))
                [
                    SNew(SBorder)
                    .Padding(FMargin(TopLevelContentPadding))
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .BorderBackgroundColor(this, &SObjectPoolQuickAction::GetHealthStatusColor)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 5, 0)
                        [
                            SNew(SImage)
                            .Image(this, &SObjectPoolQuickAction::GetHealthStatusIcon)
                            .ColorAndOpacity(FLinearColor::White)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(this, &SObjectPoolQuickAction::GetHealthStatusText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                            .Justification(ETextJustify::Center)
                            .ColorAndOpacity(FLinearColor::White)
                        ]
                    ]
                ]
                // Actors In Use
                + SHorizontalBox::Slot()
                .FillWidth(0.16f)
                .Padding(FMargin(5,0,5,0))
                [
                    SNew(SBorder)
                    .Padding(FMargin(TopLevelContentPadding))
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .BorderBackgroundColor(LightBackgroundColor)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Justification(ETextJustify::Center)
                        .Text(this, &SObjectPoolQuickAction::GetActorsOfClassInUse)
                        .ColorAndOpacity(LightTextColor)
                    ]
                ]
                // Available Actors
                + SHorizontalBox::Slot()
                .FillWidth(0.16f)
                .Padding(FMargin(5,0,5,0))
                [
                    SNew(SBorder)
                    .Padding(FMargin(TopLevelContentPadding))
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .BorderBackgroundColor(LightBackgroundColor)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Justification(ETextJustify::Center)
                        .Text(this, &SObjectPoolQuickAction::GetActorsOfClassInPool)
                        .ColorAndOpacity(LightTextColor)
                    ]
                ]
                // Growth Rotation
                + SHorizontalBox::Slot()
                .FillWidth(0.16f)
                .Padding(FMargin(5,0,5,0))
                [
                    SNew(SBorder)
                    .Padding(FMargin(TopLevelContentPadding))
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .BorderBackgroundColor(LightBackgroundColor)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Justification(ETextJustify::Center)
                        .Text(this, &SObjectPoolQuickAction::GetPoolGrowthOperation)
                        .ColorAndOpacity(LightTextColor)
                    ]
                ]
                // Cache Hit Rate
                + SHorizontalBox::Slot()
                .FillWidth(0.15f)
                .Padding(FMargin(5,0,5,0))
                [
                    SNew(SBorder)
                    .Padding(FMargin(TopLevelContentPadding))
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .BorderBackgroundColor(LightBackgroundColor)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("CacheHitRateLabel", "Cache Hit Rate"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                            .Justification(ETextJustify::Center)
                            .ColorAndOpacity(LightTextColor)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(this, &SObjectPoolQuickAction::GetCacheHitRateText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                            .Justification(ETextJustify::Center)
                            .ColorAndOpacity(this, &SObjectPoolQuickAction::GetCacheHitRateColor)
                        ]
                    ]
                ]
                // Leak Indicator Badge
                + SHorizontalBox::Slot()
                .FillWidth(0.13f)
                .Padding(FMargin(5,0,5,0))
                [
                    SNew(SBorder)
                    .Padding(FMargin(TopLevelContentPadding))
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .BorderBackgroundColor(this, &SObjectPoolQuickAction::GetLeakBadgeColor)
                    .Visibility(this, &SObjectPoolQuickAction::GetLeakBadgeVisibility)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0, 0, 5, 0)
                        [
                            SNew(SImage)
                            .Image(FAppStyle::GetBrush("Icons.Warning"))
                            .ColorAndOpacity(FLinearColor::White)
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(this, &SObjectPoolQuickAction::GetLeakCountText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                            .Justification(ETextJustify::Center)
                            .ColorAndOpacity(FLinearColor::White)
                        ]
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.f)
            [
                SNew(SHorizontalBox)
                    
                // Action Buttons
                + SHorizontalBox::Slot()
                .FillWidth(0.7f)
                [
                    SNew(SUniformWrapPanel)
                    .SlotPadding(FMargin(10))
                    .HAlign(HAlign_Left)
                    .MinDesiredSlotWidth(100.f)
                    .MinDesiredSlotHeight(75.f)
                    .MaxDesiredSlotWidth(110.f)
                    .MaxDesiredSlotHeight(75.f)
                    // Delete Button
                    + SUniformWrapPanel::Slot()
                    [
                        SNew(SObjectPoolActionButton)
                        .ButtonText(LOCTEXT("ClearPoolText", "Clear Pool"))
                        .IconBrush(FAppStyle::GetBrush("Icons.Delete"))
                        .OnClicked(this, &SObjectPoolQuickAction::OnClearAllPoolClicked)
                    ]
                    // Shrink All Pool Button
                    + SUniformWrapPanel::Slot()
                    [
                        SNew(SObjectPoolActionButton)
                        .ButtonText(LOCTEXT("ShrinkPoolText", "Shrink Pool"))
                        .IconBrush(FAppStyle::GetBrush("Icons.Transform"))
                        .OnClicked(this, &SObjectPoolQuickAction::OnShrinkAllPoolClicked)
                    ]
                    // Prewarm Pool Button
                    + SUniformWrapPanel::Slot()
                    [
                        SNew(SObjectPoolActionButton)
                        .ButtonText(LOCTEXT("PrewarmPoolText", "Prewarm Pool"))
                        .IconBrush(FAppStyle::GetBrush("Icons.Plus"))
                        .OnClicked(this, &SObjectPoolQuickAction::OnPrewarmPoolClicked)
                    ]
                    // Optimize Pool Button
                    + SUniformWrapPanel::Slot()
                    [
                        SNew(SObjectPoolActionButton)
                        .ButtonText(LOCTEXT("OptimizePoolText", "Optimize"))
                        .IconBrush(FAppStyle::GetBrush("Icons.Settings"))
                        .OnClicked(this, &SObjectPoolQuickAction::OnOptimizePoolClicked)
                    ]
                    // Scan for Leaks Button
                    + SUniformWrapPanel::Slot()
                    [
                        SNew(SObjectPoolActionButton)
                        .ButtonText(LOCTEXT("ScanLeaksText", "Scan Leaks"))
                        .IconBrush(FAppStyle::GetBrush("Icons.Search"))
                        .OnClicked(this, &SObjectPoolQuickAction::OnScanForLeaksClicked)
                    ]
                    // Force Return Leaks Button
                    + SUniformWrapPanel::Slot()
                    [
                        SNew(SObjectPoolActionButton)
                        .ButtonText(LOCTEXT("ReturnLeaksText", "Return Leaks"))
                        .IconBrush(FAppStyle::GetBrush("Icons.Refresh"))
                        .OnClicked(this, &SObjectPoolQuickAction::OnForceReturnLeaksClicked)
                    ]
                ]
                    
                // Stats
                + SHorizontalBox::Slot()
                .Padding(FMargin(0, 0, 20, 0))
                .FillWidth(0.3f)
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    
                    // Next Pool Shrink
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("NextPoolShrinkLabel", "Pool Shrink In"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                        .ColorAndOpacity(LightTextColor)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(this, &SObjectPoolQuickAction::GetNextPoolShrinkTime)
                        .Justification(ETextJustify::Center)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(LightTextColor)
                    ]
                    
                    // Last Accessed
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5, 0, 0)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("LastAccessedLabel", "Last Accessed"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                        .ColorAndOpacity(LightTextColor)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(this, &SObjectPoolQuickAction::GetLastAccessTime)
                        .Justification(ETextJustify::Center)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(LightTextColor)
                    ]
                ]
            ]
            // Performance Sparklines
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(10, 5)
            [
                SNew(SBorder)
                .Padding(FMargin(8))
                .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                .BorderBackgroundColor(LightBackgroundColor)
                [
                    SNew(SHorizontalBox)
                    // Cache Hit Rate Sparkline
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(5, 0)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("CacheHitSparklineLabel", "Cache Hit %"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                            .ColorAndOpacity(LightTextColor)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SAssignNew(CacheHitRateSparkline, SPoolSparkline)
                            .LineColor(FLinearColor(0.0f, 0.8f, 0.0f))
                            .FillColor(FLinearColor(0.0f, 0.8f, 0.0f, 0.2f))
                            .LineThickness(1.5f)
                            .MaxDataPoints(30)
                        ]
                    ]
                    // Pool Usage Sparkline
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(5, 0)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("PoolUsageSparklineLabel", "Pool Usage %"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                            .ColorAndOpacity(LightTextColor)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SAssignNew(PoolUsageSparkline, SPoolSparkline)
                            .LineColor(FLinearColor(0.2f, 0.6f, 1.0f))
                            .FillColor(FLinearColor(0.2f, 0.6f, 1.0f, 0.2f))
                            .LineThickness(1.5f)
                            .MaxDataPoints(30)
                        ]
                    ]
                    // Growth Operations Sparkline
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(5, 0)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("GrowthOpsSparklineLabel", "Growth Rate"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                            .ColorAndOpacity(LightTextColor)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SAssignNew(GrowthOpsSparkline, SPoolSparkline)
                            .LineColor(FLinearColor(1.0f, 0.6f, 0.2f))
                            .FillColor(FLinearColor(1.0f, 0.6f, 0.2f, 0.2f))
                            .LineThickness(1.5f)
                            .MaxDataPoints(30)
                        ]
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(10)
            [
                SNew(SOverlay)
                // Progress Bar
                + SOverlay::Slot()
                [
                    SNew(SProgressBar)
                    .Percent(this, &SObjectPoolQuickAction::GetTotalPoolSizeRatio)
                    .Style(FLazyGenericDynamicObjectPoolStyle::Get(), "OrangeFillProgressBar")
                ]
                    
                // Stats Text
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Center)
                .Padding(10.0f)
                [
                    SNew(SHorizontalBox)
                    // Total Actor
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(STextBlock)
                        .Text(this, &SObjectPoolQuickAction::GetTotalActorText)
                        .ColorAndOpacity(LightTextColor)
                    ]
                        
                    // Max Pool Size
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(STextBlock)
                        .Text(this, &SObjectPoolQuickAction::GetMaxPoolSizeText)
                        .ColorAndOpacity(LightTextColor)
                    ]
                        
                    // Shrink Operations
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(STextBlock)
                        .Text(this, &SObjectPoolQuickAction::GetShrinkOperationsText)
                        .ColorAndOpacity(LightTextColor)
                    ]
                ]
            ]
        ]
    ];
}

FReply SObjectPoolQuickAction::OnClearAllPoolClicked()
{
    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FReply::Unhandled();

    World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->ClearAllPools();
    return FReply::Handled();
}

FReply SObjectPoolQuickAction::OnShrinkAllPoolClicked()
{
    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FReply::Unhandled();

    World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->ShrinkAllPools();
    return FReply::Handled();
}

void SObjectPoolQuickAction::OnActorClassSelected(const UClass* Class)
{
    SelectedClass = TSubclassOf<AActor>(const_cast<UClass*>(Class));

    // Clear sparkline data when changing classes
    if (CacheHitRateSparkline.IsValid())
    {
        CacheHitRateSparkline->ClearData();
    }
    if (PoolUsageSparkline.IsValid())
    {
        PoolUsageSparkline->ClearData();
    }
    if (GrowthOpsSparkline.IsValid())
    {
        GrowthOpsSparkline->ClearData();
    }
}

UClass* SObjectPoolQuickAction::GetSelectedActorClass()
{
    return SelectedClass;
}

FText SObjectPoolQuickAction::GetNextPoolShrinkTime() const
{
    float Time = 0;
    if (const UWorld* World = GEditor->PlayWorld)
    {
        Time = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetNextAutoShrinkTime();
    }
    
    return FText::FromString(Time != 0 ? FString::FormatAsNumber(Time).ToUpper() : "DISABLED");
}

FText SObjectPoolQuickAction::GetLastAccessTime() const
{
    if (!SelectedClass)
    {
        return FText::FromString("N/A");
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText::FromString("N/A");

    const int32 AccessCount = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolAccessCount(SelectedClass);
    return FText::Format(LOCTEXT("LastAccessCountText", "{0} times"), AccessCount);
}

FText SObjectPoolQuickAction::GetTotalActorText() const
{
    int32 TotalActors;
    if (!SelectedClass)
    {
        const UWorld* World = GEditor->PlayWorld;
        if(!IsValid(World)) return FText();
        TotalActors = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetTotalActorsInAllPools();
        
        return FText::Format(LOCTEXT("TotalActorOfClassText", "{0} Total Actor"), TotalActors);
    }
    
    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText();

    TotalActors = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetAllActorsInPool(SelectedClass).Num();
    return FText::Format(LOCTEXT("TotalActorOfClassText", "{0} Total Actor Of Class {1}"), TotalActors, FText::FromString(SelectedClass->GetName()));
}

FText SObjectPoolQuickAction::GetMaxPoolSizeText() const
{
    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText();

    const int32 PoolSize = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetMaximumPoolSize();
    // Implement logic to get max pool size
    return FText::Format(LOCTEXT("MaxPoolSizeText", "{0} Max Pool Size"), PoolSize);
}

FText SObjectPoolQuickAction::GetShrinkOperationsText() const
{
    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText();

    const int32 ShrinkOperations = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetTotalShrinkOperations();
    // Implement logic to get shrink operations count
    return FText::Format(LOCTEXT("ShrinkOperationsText", "{0} Shrink Operations Completed"), ShrinkOperations);
}

FText SObjectPoolQuickAction::GetActorsOfClassInUse() const
{
    if (!SelectedClass)
    {
        return LOCTEXT("TotalActorOfClassInUseText", "No Class Selected");
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText();

    const int32 TotalActors = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetInUseActorsInPool(SelectedClass).Num();
    return FText::Format(LOCTEXT("TotalActorOfClassInUseText", "{0} Actor In Use"), TotalActors);
}

FText SObjectPoolQuickAction::GetActorsOfClassInPool() const
{
    if (!SelectedClass)
    {
        return LOCTEXT("TotalActorOfClassAvailableText", "No Class Selected");
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText();

    const int32 TotalActors = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetAvailableActorsInPool(SelectedClass).Num();
    return FText::Format(LOCTEXT("TotalActorOfClassAvailableText", "{0} Available Actors"), TotalActors);
}

FText SObjectPoolQuickAction::GetPoolGrowthOperation() const
{
    if (!SelectedClass)
    {
        return LOCTEXT("GrowthOperationText", "No Class Selected");
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText();

    const int32 TotalGrowth = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolGrowthOperation(SelectedClass);
    return FText::Format(LOCTEXT("GrowthOperationText", "{0} Growth Operation"), TotalGrowth);
}

void SObjectPoolQuickAction::SetButtonHoverColor()
{
    ButtonColor = FSlateColor(FColor::Blue);
}


void SObjectPoolQuickAction::ResetButtonColor()
{
    ButtonColor = FSlateColor(FColor::Transparent);
}

TOptional<float> SObjectPoolQuickAction::GetTotalPoolSizeRatio() const
{
    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return TOptional<float>(0);

    return World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetTotalActorsInPoolRatio();
}

FText SObjectPoolQuickAction::GetHealthStatusText() const
{
    if (!SelectedClass)
    {
        return LOCTEXT("NoClassHealthText", "N/A");
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return LOCTEXT("NoWorldHealthText", "N/A");

    const EPoolHealthStatus HealthStatus = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolHealth(SelectedClass);

    switch (HealthStatus)
    {
        case EPoolHealthStatus::Healthy: return LOCTEXT("HealthyStatusText", "Healthy");
        case EPoolHealthStatus::Warning: return LOCTEXT("WarningStatusText", "Warning");
        case EPoolHealthStatus::Critical: return LOCTEXT("CriticalStatusText", "Critical");
        case EPoolHealthStatus::Failed: return LOCTEXT("FailedStatusText", "Failed");
        default: return LOCTEXT("UnknownStatusText", "Unknown");
    }
}

FSlateColor SObjectPoolQuickAction::GetHealthStatusColor() const
{
    if (!SelectedClass)
    {
        return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)); // Gray
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));

    const EPoolHealthStatus HealthStatus = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolHealth(SelectedClass);

    switch (HealthStatus)
    {
        case EPoolHealthStatus::Healthy: return FSlateColor(FLinearColor(0.0f, 0.8f, 0.0f)); // Green
        case EPoolHealthStatus::Warning: return FSlateColor(FLinearColor(1.0f, 0.8f, 0.0f)); // Yellow
        case EPoolHealthStatus::Critical: return FSlateColor(FLinearColor(1.0f, 0.4f, 0.0f)); // Orange
        case EPoolHealthStatus::Failed: return FSlateColor(FLinearColor(1.0f, 0.0f, 0.0f)); // Red
        default: return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)); // Gray
    }
}

const FSlateBrush* SObjectPoolQuickAction::GetHealthStatusIcon() const
{
    if (!SelectedClass)
    {
        return FAppStyle::GetBrush("Icons.Help");
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FAppStyle::GetBrush("Icons.Help");

    const EPoolHealthStatus HealthStatus = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolHealth(SelectedClass);

    switch (HealthStatus)
    {
        case EPoolHealthStatus::Healthy: return FAppStyle::GetBrush("Icons.Check");
        case EPoolHealthStatus::Warning: return FAppStyle::GetBrush("Icons.Warning");
        case EPoolHealthStatus::Critical: return FAppStyle::GetBrush("Icons.Error");
        case EPoolHealthStatus::Failed: return FAppStyle::GetBrush("Icons.X");
        default: return FAppStyle::GetBrush("Icons.Help");
    }
}

EVisibility SObjectPoolQuickAction::GetHealthWarningVisibility() const
{
    if (!SelectedClass)
    {
        return EVisibility::Collapsed;
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return EVisibility::Collapsed;

    const EPoolHealthStatus HealthStatus = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolHealth(SelectedClass);

    return (HealthStatus == EPoolHealthStatus::Warning ||
            HealthStatus == EPoolHealthStatus::Critical ||
            HealthStatus == EPoolHealthStatus::Failed) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SObjectPoolQuickAction::GetHealthWarningText() const
{
    if (!SelectedClass)
    {
        return FText::GetEmpty();
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FText::GetEmpty();

    const EPoolHealthStatus HealthStatus = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetPoolHealth(SelectedClass);
    const float CacheHitRate = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetCacheHitRate(SelectedClass);

    switch (HealthStatus)
    {
        case EPoolHealthStatus::Warning:
            if (CacheHitRate < 80.0f)
            {
                return LOCTEXT("LowCacheHitWarning", "Low cache hit rate. Consider prewarming this pool.");
            }
            return LOCTEXT("PoolWarningText", "Pool usage is high (75-90%). Consider increasing pool size.");

        case EPoolHealthStatus::Critical:
            return LOCTEXT("PoolCriticalText", "Pool usage is critical (>90%)! Increase pool size immediately.");

        case EPoolHealthStatus::Failed:
            return LOCTEXT("PoolFailedText", "Pool has failed! Check logs for details.");

        default:
            return FText::GetEmpty();
    }
}

FText SObjectPoolQuickAction::GetCacheHitRateText() const
{
    if (!SelectedClass)
    {
        return LOCTEXT("NoCacheHitRateText", "N/A");
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return LOCTEXT("NoCacheHitRateText", "N/A");

    const float CacheHitRate = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetCacheHitRate(SelectedClass);
    return FText::Format(LOCTEXT("CacheHitRateText", "{0}%"), FText::AsNumber(FMath::RoundToInt(CacheHitRate)));
}

FSlateColor SObjectPoolQuickAction::GetCacheHitRateColor() const
{
    if (!SelectedClass)
    {
        return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));

    const float CacheHitRate = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->GetCacheHitRate(SelectedClass);

    if (CacheHitRate >= 95.0f) return FSlateColor(FLinearColor(0.0f, 0.8f, 0.0f)); // Green - Excellent
    if (CacheHitRate >= 90.0f) return FSlateColor(FLinearColor(0.5f, 0.8f, 0.0f)); // Yellow-Green - Good
    if (CacheHitRate >= 80.0f) return FSlateColor(FLinearColor(1.0f, 0.8f, 0.0f)); // Yellow - Acceptable
    if (CacheHitRate >= 70.0f) return FSlateColor(FLinearColor(1.0f, 0.5f, 0.0f)); // Orange - Poor
    return FSlateColor(FLinearColor(1.0f, 0.0f, 0.0f)); // Red - Critical
}

FReply SObjectPoolQuickAction::OnPrewarmPoolClicked()
{
    if (!SelectedClass)
    {
        return FReply::Unhandled();
    }

    const UWorld* World = GEditor->PlayWorld;
    if(!IsValid(World)) return FReply::Unhandled();

    // Prewarm to 150% of current in-use count, minimum 10
    ULazyDynamicObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
    const int32 InUseCount = PoolSubsystem->GetInUseActorsInPool(SelectedClass).Num();
    const int32 PrewarmSize = FMath::Max(10, FMath::CeilToInt(InUseCount * 1.5f));

    PoolSubsystem->PrewarmPool(SelectedClass, PrewarmSize);

    return FReply::Handled();
}

FReply SObjectPoolQuickAction::OnOptimizePoolClicked()
{
    if (!SelectedClass)
    {
        const UWorld* World = GEditor->PlayWorld;
        if(!IsValid(World)) return FReply::Unhandled();

        // Optimize all pools
        World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>()->OptimizeAllPools();
    }
    else
    {
        const UWorld* World = GEditor->PlayWorld;
        if(!IsValid(World)) return FReply::Unhandled();

        // Optimize selected pool
        ULazyDynamicObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
        PoolSubsystem->CompactPoolMemory(SelectedClass);
    }

    return FReply::Handled();
}

void SObjectPoolQuickAction::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    // Update sparklines every 0.5 seconds
    if (InCurrentTime - LastSparklineUpdate >= 0.5)
    {
        UpdateSparklineData();
        LastSparklineUpdate = InCurrentTime;
    }

    // Update leak count periodically (every 2 seconds)
    if (InCurrentTime - LastLeakScanTime >= 2.0)
    {
        const UWorld* World = GEditor->PlayWorld;
        if (IsValid(World) && SelectedClass)
        {
            ULazyDynamicObjectPoolSubsystem* SubSys = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
            if (SubSys)
            {
                TArray<FLeakedActorInfo> Leaks = SubSys->ScanForLeaks(SelectedClass, 30.0f);
                CachedLeakCount = Leaks.Num();
            }
        }
        LastLeakScanTime = InCurrentTime;
    }
}

void SObjectPoolQuickAction::UpdateSparklineData()
{
    if (!SelectedClass || !CacheHitRateSparkline.IsValid() || !PoolUsageSparkline.IsValid() || !GrowthOpsSparkline.IsValid())
    {
        return;
    }

    const UWorld* World = GEditor->PlayWorld;
    if (!IsValid(World))
    {
        return;
    }

    ULazyDynamicObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
    if (!PoolSubsystem)
    {
        return;
    }

    // Update Cache Hit Rate sparkline
    const float CacheHitRate = PoolSubsystem->GetCacheHitRate(SelectedClass);
    CacheHitRateSparkline->AddDataPoint(CacheHitRate);

    // Update Pool Usage sparkline
    const TArray<AActor*> AllActors = PoolSubsystem->GetAllActorsInPool(SelectedClass);
    const TArray<AActor*> InUseActors = PoolSubsystem->GetInUseActorsInPool(SelectedClass);

    float UsagePercent = 0.0f;
    if (AllActors.Num() > 0)
    {
        UsagePercent = (static_cast<float>(InUseActors.Num()) / AllActors.Num()) * 100.0f;
    }
    PoolUsageSparkline->AddDataPoint(UsagePercent);

    // Update Growth Operations sparkline (growth operations per second)
    static int32 LastGrowthOps = 0;
    const int32 CurrentGrowthOps = PoolSubsystem->GetPoolGrowthOperation(SelectedClass);
    const int32 GrowthDelta = FMath::Max(0, CurrentGrowthOps - LastGrowthOps);
    LastGrowthOps = CurrentGrowthOps;

    // Scale to operations per second (update every 0.5s, so multiply by 2)
    GrowthOpsSparkline->AddDataPoint(static_cast<float>(GrowthDelta) * 2.0f);
}

FText SObjectPoolQuickAction::GetLeakCountText() const
{
    if (CachedLeakCount == 0)
    {
        return LOCTEXT("NoLeaksText", "No Leaks");
    }
    return FText::Format(LOCTEXT("LeakCountText", "{0} Leaks"), CachedLeakCount);
}

FSlateColor SObjectPoolQuickAction::GetLeakBadgeColor() const
{
    if (CachedLeakCount == 0)
    {
        return FSlateColor(FLinearColor(0.0f, 0.8f, 0.0f, 0.8f)); // Green - no leaks
    }
    else if (CachedLeakCount < 5)
    {
        return FSlateColor(FLinearColor(1.0f, 0.8f, 0.0f, 0.9f)); // Yellow - few leaks
    }
    else
    {
        return FSlateColor(FLinearColor(1.0f, 0.2f, 0.0f, 0.9f)); // Red - many leaks
    }
}

EVisibility SObjectPoolQuickAction::GetLeakBadgeVisibility() const
{
    // Show badge if there are leaks or if we're actively monitoring
    return (SelectedClass && CachedLeakCount > 0) ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SObjectPoolQuickAction::OnScanForLeaksClicked()
{
    if (!SelectedClass)
    {
        return FReply::Unhandled();
    }

    const UWorld* World = GEditor->PlayWorld;
    if (!IsValid(World)) return FReply::Unhandled();

    ULazyDynamicObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
    if (!PoolSubsystem)
    {
        return FReply::Unhandled();
    }

    // Perform scan with 30 second threshold
    TArray<FLeakedActorInfo> Leaks = PoolSubsystem->ScanForLeaks(SelectedClass, 30.0f);
    CachedLeakCount = Leaks.Num();

    // Log results
    if (Leaks.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Found %d potentially leaked actors in pool %s:"), Leaks.Num(), *SelectedClass->GetName());
        for (const FLeakedActorInfo& Leak : Leaks)
        {
            if (Leak.Actor.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("  - %s (in use for %.1f seconds) at %s"),
                    *Leak.ActorName, Leak.TimeInUse, *Leak.Location.ToString());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("No leaked actors found in pool %s"), *SelectedClass->GetName());
    }

    return FReply::Handled();
}

FReply SObjectPoolQuickAction::OnForceReturnLeaksClicked()
{
    if (!SelectedClass)
    {
        return FReply::Unhandled();
    }

    const UWorld* World = GEditor->PlayWorld;
    if (!IsValid(World)) return FReply::Unhandled();

    ULazyDynamicObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>();
    if (!PoolSubsystem)
    {
        return FReply::Unhandled();
    }

    // Force return all leaked actors (30 second threshold)
    const int32 ReturnedCount = PoolSubsystem->ForceReturnAllLeakedActors(SelectedClass, 30.0f);

    UE_LOG(LogTemp, Log, TEXT("Force returned %d leaked actors from pool %s"), ReturnedCount, *SelectedClass->GetName());

    // Refresh leak count
    CachedLeakCount = 0;

    return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE