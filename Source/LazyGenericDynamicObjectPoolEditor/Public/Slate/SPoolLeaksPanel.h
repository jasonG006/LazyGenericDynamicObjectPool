// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Slate/SPoolPanelBase.h"

class ITableRow;
class STableViewBase;

/**
 * Leak information for a single actor
 */
struct FPoolLeakInfo
{
	TWeakObjectPtr<AActor> Actor;
	FString ActorName;
	TSubclassOf<AActor> ActorClass;
	float TimeInUse;
	FVector Location;
	bool bIsInPool;
	bool bIsInUseList;

	FPoolLeakInfo() :
		TimeInUse(0.0f), Location(FVector::ZeroVector), bIsInPool(false), bIsInUseList(false) {}

	FPoolLeakInfo(AActor* InActor, TSubclassOf<AActor> InClass, float InTimeInUse, const FVector& InLocation) :
		Actor(InActor), ActorName(InActor ? InActor->GetName() : TEXT("Invalid")), ActorClass(InClass),
		TimeInUse(InTimeInUse), Location(InLocation), bIsInPool(false), bIsInUseList(true) {}
};

/**
 * Panel for detecting and managing potential memory leaks (actors not returned to pool).
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolLeaksPanel : public SPoolPanelBase
{
public:
	SLATE_BEGIN_ARGS(SPoolLeaksPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	// UI Builders
	TSharedRef<SWidget> BuildControlBar();
	TSharedRef<SWidget> BuildLeakList();
	TSharedRef<SWidget> BuildActionFooter();

	// List View
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPoolLeakInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Actions
	FReply OnScanClicked();
	FReply OnReturnAllClicked();
	FReply OnClearIgnoreListClicked();

	// State
	TArray<TSharedPtr<FPoolLeakInfo>> DetectedLeaks;
	TSharedPtr<SListView<TSharedPtr<FPoolLeakInfo>>> LeakListView;

	float ScanThreshold = 5.0f;
	TArray<TSharedPtr<float>> ThresholdOptions;
	TSharedPtr<SComboBox<TSharedPtr<float>>> ThresholdComboBox;

	// Helpers
	FText GetScanThresholdText() const;
	void OnThresholdChanged(TSharedPtr<float> NewValue, ESelectInfo::Type SelectInfo);
	FText GetLeakCountText() const;
};
