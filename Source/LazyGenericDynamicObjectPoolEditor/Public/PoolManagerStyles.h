// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"

/**
 * Centralized style constants for the Pool Manager Editor UI.
 * Using a namespace rather than a class for compile-time constants.
 */
namespace FPoolManagerStyles
{
	// ==================== BACKGROUND COLORS (from index.css) ====================

	/** --bg-base: #222222 */
	inline FLinearColor GetPanelBackground() { return FLinearColor::FromSRGBColor(FColor::FromHex("0d0d0d")); }

	/** --bg-primary: #141414 */
	inline FLinearColor GetHeaderBackground() { return FLinearColor::FromSRGBColor(FColor::FromHex("222222")); }

	/** --bg-secondary: #1a1a1a */
	inline FLinearColor GetCardBackground() { return FLinearColor::FromSRGBColor(FColor::FromHex("1a1a1a")); }

	/** --bg-secondary for progress bar tracks */
	inline FLinearColor GetSecondaryBackground() { return FLinearColor::FromSRGBColor(FColor::FromHex("1a1a1a")); }

	/** --bg-tertiary: #222222 */
	inline FLinearColor GetTertiaryBackground() { return FLinearColor::FromSRGBColor(FColor::FromHex("222222")); }

	/** --bg-panel: #1e1e1e */
	inline FLinearColor GetContainerBackground() { return FLinearColor::FromSRGBColor(FColor::FromHex("1e1e1e")); }

	/** --bg-hover: #2a2a2a */
	inline FLinearColor GetHoverBackground() { return FLinearColor::FromSRGBColor(FColor::FromHex("2a2a2a")); }

	/** --border-color: #2a2a2a */
	inline FLinearColor GetBorderColor() { return FLinearColor::FromSRGBColor(FColor::FromHex("2a2a2a")); }

	// ==================== ACCENT COLORS (from index.css) ====================

	/** --accent-blue: #3b82f6 */
	inline FLinearColor GetAccentBlue() { return FLinearColor::FromSRGBColor(FColor::FromHex("3b82f6")); }

	/** --accent-green: #22c55e */
	inline FLinearColor GetAccentGreen() { return FLinearColor::FromSRGBColor(FColor::FromHex("22c55e")); }

	/** --accent-yellow: #eab308 */
	inline FLinearColor GetAccentYellow() { return FLinearColor::FromSRGBColor(FColor::FromHex("eab308")); }

	/** --accent-orange: #f97316 */
	inline FLinearColor GetAccentOrange() { return FLinearColor::FromSRGBColor(FColor::FromHex("f97316")); }

	/** --accent-red: #ef4444 */
	inline FLinearColor GetAccentRed() { return FLinearColor::FromSRGBColor(FColor::FromHex("ef4444")); }

	/** --accent-purple: #a855f7 */
	inline FLinearColor GetAccentPurple() { return FLinearColor::FromSRGBColor(FColor::FromHex("a855f7")); }

	// ==================== TEXT COLORS (from index.css) ====================

	/** --text-primary: #f0f0f0 */
	inline FLinearColor GetTextPrimary() { return FLinearColor::FromSRGBColor(FColor::FromHex("f0f0f0")); }

	/** --text-secondary: #a8a8a8 */
	inline FLinearColor GetTextSecondary() { return FLinearColor::FromSRGBColor(FColor::FromHex("a8a8a8")); }

	/** --text-muted: #666666 */
	inline FLinearColor GetTextMuted() { return FLinearColor::FromSRGBColor(FColor::FromHex("666666")); }

	// ==================== TAB STYLING ====================

	inline FLinearColor GetTabActiveText() { return FLinearColor::FromSRGBColor(FColor::FromHex("60a5fa")); }
	// --accent-blue-light
	inline FLinearColor GetTabInactiveText() { return GetTextSecondary(); }
	inline FLinearColor GetTabActiveAccent() { return GetAccentBlue(); }

	// ==================== HEALTH STATUS ====================

	inline FLinearColor GetHealthyColor() { return GetAccentGreen(); }
	inline FLinearColor GetWarningColor() { return GetAccentYellow(); }
	inline FLinearColor GetCriticalColor() { return GetAccentOrange(); } // Mapped to orange in CSS logic
	inline FLinearColor GetFailedColor() { return GetAccentRed(); }

	// ==================== BADGE COLORS ====================

	/** Leak badge background */
	inline FLinearColor GetLeakBadgeBackground() { return GetAccentRed(); }

	// ==================== LEGACY / COMPATIBILITY ====================

	/** Darkest background for backward compat */
	inline FLinearColor GetBackgroundDark() { return GetPanelBackground(); }

	// ==================== COLOR UTILITIES ====================

	/** Light green for good cache hit rates (75-94%) */
	inline FLinearColor GetLightGreen() { return FLinearColor(0.2f, 0.8f, 0.4f); }

	/**
	 * Get appropriate color for cache hit percentage
	 * @param HitPercent Cache hit rate (0-100)
	 * @return Color ranging from red (poor) to green (excellent)
	 */
	inline FLinearColor GetCacheHitColor(float HitPercent)
	{
		if (HitPercent >= 95.f) return GetAccentGreen();
		if (HitPercent >= 75.f) return GetLightGreen();
		if (HitPercent >= 50.f) return GetWarningColor();
		return GetCriticalColor();
	}

	/**
	 * Get appropriate color for pool usage percentage
	 * @param UsagePercent Usage ratio (0.0-1.0)
	 * @return Color ranging from green (low) to red (critical)
	 */
	inline FLinearColor GetUsageStatusColor(float UsagePercent)
	{
		if (UsagePercent > 0.9f) return GetCriticalColor();
		if (UsagePercent > 0.75f) return GetWarningColor();
		if (UsagePercent > 0.5f) return GetAccentOrange();
		return GetAccentGreen();
	}

	// ==================== FONTS & SIZES ====================

	constexpr int32 FontSizeTitle = 16;
	constexpr int32 FontSizeHeader = 12;
	constexpr int32 FontSizeBody = 10;
	constexpr int32 FontSizeSmall = 9;
	constexpr int32 FontSizeTiny = 8;

	constexpr float PaddingLarge = 16.0f;
	constexpr float PaddingMedium = 12.0f;
	constexpr float PaddingSmall = 8.0f;
	constexpr float PaddingTiny = 4.0f;
	constexpr float PaddingNone = 0.0f;

	// ==================== HELPERS ====================

	inline FSlateFontInfo GetFont(const FName& TypefaceName = "Bold", int32 Size = FontSizeBody)
	{
		return FCoreStyle::GetDefaultFontStyle(TypefaceName, Size);
	}

	inline const FSlateBrush* GetPanelBrush() { return FAppStyle::Get().GetBrush("Brushes.Panel"); }
	inline const FSlateBrush* GetWhiteBrush() { return FAppStyle::Get().GetBrush("Brushes.White"); }
	inline const FSlateBrush* GetRoundedBox() { return FAppStyle::Get().GetBrush("Brushes.White"); }

	inline FSlateColor AsSlateColor(const FLinearColor& Color) { return FSlateColor(Color); }

} // namespace FPoolManagerStyles
