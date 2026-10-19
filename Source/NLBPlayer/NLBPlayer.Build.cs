using System.IO;
using UnrealBuildTool;

// Плеер книг NLB (ядро, движок, слой вывода, cparse) внутри проекта Unreal.
// Исходники плеера лежат в Source/NLBCore, Source/NLBReader, Source/NLBPresentation и
// Source/thirdparty/cparse (общие с CMake-сборкой); модуль компилирует их через обёртки
// Private/Generated/*.cpp (их создаёт update_sources.py). Наружу модуль отдаёт только
// UNLBPlayerSubsystem и структуры для Blueprint — классы плеера остаются внутри модуля.
public class NLBPlayer : ModuleRules
{
    public NLBPlayer(ReadOnlyTargetRules Target) : base(Target)
    {
        // Код плеера не зависит от Unreal: без общего PCH, чтобы макросы и заголовки движка
        // не попадали в его файлы
        PCHUsage = PCHUsageMode.NoPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;   // плеер и cparse используют исключения
        // RTTI выключен, как во всём движке: иначе под Android (clang) сборщику нужны typeinfo
        // классов движка (UGameInstanceSubsystem, UUserWidget…), а их в сборке движка нет.
        // Плееру RTTI не нужен (Any.h без typeid), исключения работают и без него
        bUseRTTI = false;
        bUseUnity = false;          // у файлов плеера одноимённые функции в безымянных пространствах имён
        // Предупреждения кода плеера не должны останавливать сборку игры. С UE 5.5 эти настройки
        // перенесены в CppCompileWarningSettings (старые свойства устарели); символ UE_5_5_OR_LATER
        // UBT определяет сам, поэтому файл подходит и для 5.5, и для 5.6+, и для более старых версий
#if UE_5_5_OR_LATER
        CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Off;
        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
#else
        ShadowVariableWarningLevel = WarningLevel.Off;
        UndefinedIdentifierWarningLevel = WarningLevel.Off;
#endif

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG", "Slate", "SlateCore", "InputCore" });
        // OnlineSubsystem: достижения, статистика, рекорды (Steam, EOS, консоли — по настройке проекта)
        PrivateDependencyModuleNames.AddRange(new string[] { "ImageWrapper", "OnlineSubsystem" });

        string Src = Path.GetFullPath(Path.Combine(ModuleDirectory, ".."));
        PrivateIncludePaths.AddRange(new string[] {
            Src,
            Path.Combine(Src, "NLBCore", "Public"),
            Path.Combine(Src, "NLBReader", "Public"),
            Path.Combine(Src, "NLBPresentation", "Public"),
            Path.Combine(Src, "thirdparty", "cparse"),
            Path.Combine(Src, "thirdparty"),   // audiodecoders/: stb_vorbis, dr_mp3, dr_wav
        });
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.Add("shlwapi.lib");
            PublicSystemLibraries.Add("shell32.lib");
        }
    }
}
