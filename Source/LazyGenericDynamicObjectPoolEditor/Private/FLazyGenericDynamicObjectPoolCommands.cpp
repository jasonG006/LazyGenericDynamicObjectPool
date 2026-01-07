// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "FLazyGenericDynamicObjectPoolCommands.h"

#define LOCTEXT_NAMESPACE "FLazyGenericDynamicObjectPoolEditorModule"

void FLazyGenericDynamicObjectPoolCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "LazyGenericPoolWindow", "Open Lazy Generic Pool Debug Console", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE