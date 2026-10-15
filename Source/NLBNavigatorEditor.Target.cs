// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NLBNavigatorEditorTarget : TargetRules
{
	public NLBNavigatorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		// Порядок включений — по версии движка: проект собирается и в 5.5, и в 5.6
#if UE_5_6_OR_LATER
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
#else
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
#endif
		ExtraModuleNames.Add("NLBNavigator");
	}
}
