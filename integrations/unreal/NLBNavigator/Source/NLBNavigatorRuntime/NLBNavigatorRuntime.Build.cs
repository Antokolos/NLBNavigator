using System.IO;
using UnrealBuildTool;

// Модуль собирает плеер NLBNavigator (ядро, движок, NLBPresentation) прямо из исходников репозитория
// и предоставляет UNLBPlayerSubsystem. Путь к репозиторию — NLBNavigatorRoot ниже (по умолчанию
// плагин лежит в integrations/unreal репозитория; при копировании в проект — поправьте путь).
public class NLBNavigatorRuntime : ModuleRules
{
    public NLBNavigatorRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        // Плеер и библиотека выражений cparse используют исключения
        bEnableExceptions = true;
        // Код плеера не рассчитан на unity-сборку и пишется без предупреждений UE как ошибок
        bUseUnity = false;
        ShadowVariableWarningLevel = WarningLevel.Off;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new string[] { "AudioMixer", "ImageWrapper", "RenderCore" });

        string NLBNavigatorRoot = Path.GetFullPath(Path.Combine(PluginDirectory, "..", "..", ".."));
        string Src = Path.Combine(NLBNavigatorRoot, "Source");
        PublicIncludePaths.AddRange(new string[] {
            Path.Combine(Src, "NLBCore", "Public"),
            Path.Combine(Src, "NLBReader", "Public"),
            Path.Combine(Src, "NLBPresentation", "Public"),
            Path.Combine(Src, "thirdparty", "cparse"),
        });
        // Исходники плеера подключаются через NLBNavigatorSources.cpp (единица трансляции с #include .cpp),
        // поэтому их каталоги нужны как пути поиска
        PrivateIncludePaths.Add(Src);
        PublicDefinitions.Add("NLB_EMBEDDED=1");
    }
}
