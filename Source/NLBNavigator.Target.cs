// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NLBNavigatorTarget : TargetRules
{
	public NLBNavigatorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		// Порядок включений — по версии движка: проект собирается и в 5.5, и в 5.6
#if UE_5_6_OR_LATER
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
#else
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
#endif
		ExtraModuleNames.Add("NLBNavigator");
#if UE_5_6_OR_LATER
		// Android и iOS: аллокатор движка — системный (malloc). Плеер книг и libc++ собраны в игру
		// статически: часть памяти libc++ выделяет своим operator new (malloc), а освобождает её
		// код плеера через аллокатор движка — Binned3 такой указатель не узнаёт и падает
		// («FMallocBinned … unrecognized pointer»). С системным аллокатором и выделение, и
		// освобождение — malloc/free. Работает и с собранным (не исходным) движком, с 5.6
		if (Target.Platform == UnrealTargetPlatform.Android || Target.Platform == UnrealTargetPlatform.IOS)
		{
			StaticAllocator = StaticAllocatorType.Ansi;
		}
#endif
	}
}
