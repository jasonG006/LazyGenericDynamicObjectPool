// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Slate/SPoolPanelBase.h"
#include "Editor.h"
#include "Interfaces/IPluginManager.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"

#define LOCTEXT_NAMESPACE "SPoolPanelBase"

// Static member initialization
FString SPoolPanelBase::CachedPluginVersion;

void SPoolPanelBase::Construct(const FArguments& InArgs)
{
	SelectedClass = InArgs._InitialClass;

	// Pre-cache plugin version on first construct
	if (CachedPluginVersion.IsEmpty()) { CachedPluginVersion = GetPluginVersion(); }
}

// ==================== CLASS SELECTION ====================

void SPoolPanelBase::SetSelectedClass(TSubclassOf<AActor> NewClass)
{
	if (SelectedClass != NewClass)
	{
		SelectedClass = NewClass;
		OnClassChangedDelegate.Broadcast(SelectedClass);
	}
}

void SPoolPanelBase::OnActorClassSelected(const UClass* Class)
{
	SelectedClass = TSubclassOf<AActor>(const_cast<UClass*>(Class));
}

UClass* SPoolPanelBase::GetSelectedActorClass() const { return SelectedClass.Get(); }

// ==================== SUBSYSTEM ACCESS ====================

ULazyDynamicObjectPoolSubsystem* SPoolPanelBase::GetPoolSubsystem() const
{
	if (UWorld* World = GetPlayWorld()) { return World->GetSubsystem<ULazyDynamicObjectPoolSubsystem>(); }
	return nullptr;
}

UWorld* SPoolPanelBase::GetPlayWorld() const { return GEditor ? GEditor->PlayWorld : nullptr; }

bool SPoolPanelBase::IsInPIE() const { return GetPlayWorld() != nullptr; }

// ==================== COMMON UI HELPERS ====================

FText SPoolPanelBase::FormatDuration(float Seconds) const
{
	if (Seconds < 0.0f) { return LOCTEXT("InvalidDuration", "N/A"); }

	if (Seconds < 60.0f)
	{
		return FText::Format(LOCTEXT("SecondsDuration", "{0}s"), FText::AsNumber(FMath::RoundToInt(Seconds)));
	}

	const int32 Minutes = FMath::FloorToInt(Seconds / 60.0f);
	const int32 RemainingSeconds = FMath::RoundToInt(FMath::Fmod(Seconds, 60.0f));

	if (RemainingSeconds > 0)
	{
		return FText::Format(LOCTEXT("MinutesSecondsDuration", "{0}m {1}s"), Minutes, RemainingSeconds);
	}

	return FText::Format(LOCTEXT("MinutesDuration", "{0}m"), Minutes);
}

FString SPoolPanelBase::GetPluginVersion()
{
	// If already cached, return cached version
	if (!CachedPluginVersion.IsEmpty()) { return CachedPluginVersion; }

	// Try to get version from plugin manager
	IPluginManager& PluginManager = IPluginManager::Get();
	TSharedPtr<IPlugin> Plugin = PluginManager.FindPlugin(TEXT("LazyGenericDynamicObjectPool"));

	if (Plugin.IsValid())
	{
		const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
		CachedPluginVersion = Descriptor.VersionName;
		return CachedPluginVersion;
	}

	// Fallback to hardcoded version if plugin lookup fails
	CachedPluginVersion = TEXT("1.0");
	return CachedPluginVersion;
}

FText SPoolPanelBase::GetVersionWithStatus() const
{
	const FString Version = GetPluginVersion();
	const FString Status = IsInPIE() ? TEXT("PIE Active") : TEXT("Editor");

	return FText::Format(LOCTEXT("VersionWithStatus", "v{0} • {1}"), FText::FromString(Version),
						 FText::FromString(Status));
}

#undef LOCTEXT_NAMESPACE
