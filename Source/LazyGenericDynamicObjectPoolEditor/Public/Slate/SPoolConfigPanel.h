// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Slate/SPoolPanelBase.h"

class ULazyDynamicObjectPoolSettings;
class SCheckBox;
class SEditableTextBox;

/**
 * Configuration tab for modifying plugin settings.
 */
class LAZYGENERICDYNAMICOBJECTPOOLEDITOR_API SPoolConfigPanel : public SPoolPanelBase
{
public:
	SLATE_BEGIN_ARGS(SPoolConfigPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// UI Builders
	TSharedRef<SWidget> BuildGeneralSettingsSection();
	TSharedRef<SWidget> BuildOptimizationSettingsSection();
	TSharedRef<SWidget> BuildDebuggingSettingsSection();

	// Setting Helpers
	ULazyDynamicObjectPoolSettings* GetSettings() const;
	void SaveSettings();

	// Value Getters/Setters
	FText GetDefaultInitialPoolSizeText() const;
	void OnDefaultInitialPoolSizeChanged(const FText& Text);

	FText GetMaxPoolSizeText() const;
	void OnMaxPoolSizeChanged(const FText& Text);

	FText GetPoolGrowthFactorText() const;
	void OnPoolGrowthFactorChanged(const FText& Text);

	ECheckBoxState GetAutoShrinkState() const;
	void OnAutoShrinkChanged(ECheckBoxState NewState);

	FText GetAutoShrinkIntervalText() const;
	void OnAutoShrinkIntervalChanged(const FText& Text);

	FText GetShrinkThresholdText() const;
	void OnShrinkThresholdChanged(const FText& Text);

	ECheckBoxState GetDetailedLoggingState() const;
	void OnDetailedLoggingChanged(ECheckBoxState NewState);

	template <typename T>
	bool NumericTypeIs(const FString& InStr) const
	{
		return InStr.IsNumeric();
	}
};
