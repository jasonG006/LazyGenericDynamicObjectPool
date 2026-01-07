// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "FLazyGenericDynamicObjectPoolStyle.h"

class FLazyGenericDynamicObjectPoolCommands : public TCommands<FLazyGenericDynamicObjectPoolCommands>
{
public:
	FLazyGenericDynamicObjectPoolCommands()
	: TCommands<FLazyGenericDynamicObjectPoolCommands>
	(TEXT("LazyGenericPoolWindow"), NSLOCTEXT("Contexts", "LazyGenericPoolWindow", "Lazy Generic Pool Window Plugin"),
		NAME_None, FLazyGenericDynamicObjectPoolStyle::GetStyleSetName())
	{}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
