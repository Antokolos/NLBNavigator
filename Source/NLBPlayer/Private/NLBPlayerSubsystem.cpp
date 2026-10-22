#include "NLBPlayerSubsystem.h"

#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

#include "NLBEngineEvents.h"
#include "UnrealAudioBackend.h"
#include "NLBOnlinePlatformServices.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "nlb/presentation/Localization.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"
#include "nlb/presentation/MediaSource.h"
#include "nlb/presentation/Session.h"

using namespace nlb::presentation;

namespace {
FString S(const std::string& V) { return UTF8_TO_TCHAR(V.c_str()); }
std::string U(const FString& V) { return std::string(TCHAR_TO_UTF8(*V)); }

FString FramePath(const ImageRef& R) {
    if (R.framePattern.empty()) return S(R.file.path);
    char Buf[1024];
    std::snprintf(Buf, sizeof(Buf), R.framePattern.c_str(), R.currentFrame > 0 ? R.currentFrame : 1);
    return S(Buf);
}

TArray<FNLBParagraph> Paragraphs(const std::vector<Paragraph>& Ps) {
    TArray<FNLBParagraph> Out;
    for (const auto& P : Ps) {
        FNLBParagraph Para;
        for (const auto& R : P.runs) {
            FNLBTextRun Run;
            Run.Kind = R.kind == TextRun::Kind::ObjectLink ? TEXT("object") : R.kind == TextRun::Kind::ChoiceLink ? TEXT("choice") : TEXT("text");
            Run.Text = S(R.text);
            Run.Target = S(R.target);
            Para.Runs.Add(Run);
        }
        Out.Add(Para);
    }
    return Out;
}
}

void FNLBSessionDeleter::operator()(nlb::presentation::Session* P) const { delete P; }
void FNLBAudioDeleter::operator()(FUnrealAudioBackend* P) const { delete P; }

bool UNLBPlayerSubsystem::IsBookOpen() const { return Session != nullptr; }

FString UNLBPlayerSubsystem::DiskPath(const FString& UePath) {
#if PLATFORM_ANDROID
    // путь Unreal («../../../Проект/Saved/…») -> настоящий путь в хранилище приложения
    return IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*UePath);
#else
    return FPaths::ConvertRelativePathToFull(UePath);
#endif
}

FString UNLBPlayerSubsystem::ResolveBookPath(const FString& BookPath) {
    if (!FPaths::IsRelative(BookPath)) return BookPath;
    // относительный путь — от каталога проекта, а если там нет — от Content (так книга попадает в
    // сборку: Content/NLBFiles, «всегда как отдельные файлы» в настройках упаковки)
    IFileManager& Files = IFileManager::Get();
    FString Ue = FPaths::ProjectDir() / BookPath;
    if (!Files.FileExists(*Ue) && !Files.DirectoryExists(*Ue)) Ue = FPaths::ProjectContentDir() / BookPath;
#if PLATFORM_ANDROID
    // В APK/OBB файлы игры — внутри архива, а плеер читает книгу обычным вводом-выводом: при первом
    // запуске (и когда книга в сборке сменилась) пакет копируется в хранилище приложения
    if (Files.DirectoryExists(*Ue)) {
        UE_LOG(LogTemp, Error, TEXT("NLB: на Android книга должна быть пакетом .nlbpack (%s)"), *Ue);
        return FString();
    }
    const int64 Size = Files.FileSize(*Ue);
    if (Size < 0) {
        UE_LOG(LogTemp, Error, TEXT("NLB: книга не найдена в сборке: %s"), *BookPath);
        return FString();
    }
    const FString Copy = FPaths::ProjectPersistentDownloadDir() / TEXT("NLBNavigator") / FPaths::GetCleanFilename(Ue);
    if (Files.FileSize(*Copy) != Size) {
        Files.MakeDirectory(*FPaths::GetPath(Copy), true);
        if (Files.Copy(*Copy, *Ue) != COPY_OK) {
            UE_LOG(LogTemp, Error, TEXT("NLB: не удалось скопировать книгу в %s"), *Copy);
            return FString();
        }
        UE_LOG(LogTemp, Log, TEXT("NLB: книга скопирована из сборки (%lld байт)"), Size);
    }
    return DiskPath(Copy);
#else
    return FPaths::ConvertRelativePathToFull(Ue);
#endif
}

bool UNLBPlayerSubsystem::OpenBook(const FString& BookPath, bool bVisualNovel, const TArray<FString>& ExternalGames) {
    Audio.reset(new FUnrealAudioBackend(this, nullptr));
    Session.reset(new nlb::presentation::Session(Audio.get()));
    Session::Options Options;
    const FString Book = ResolveBookPath(BookPath);
    if (Book.IsEmpty()) {
        Session.reset();
        return false;
    }
    Options.bookPath = U(Book);
    Options.exportMode = bVisualNovel ? PlayerEngine::ExportMode::VN : PlayerEngine::ExportMode::Standard;
    const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("NLBNavigator");
    IFileManager::Get().MakeDirectory(*(SaveDir / TEXT("saves")), true);
    // плеер пишет сохранения и достижения обычным файловым вводом-выводом — нужен путь на диске
    Options.savesDir = U(DiskPath(SaveDir / TEXT("saves")));
    Options.achievementsPath = U(DiskPath(SaveDir / TEXT("achievements.tsv")));
    for (const FString& G : ExternalGames) Options.externalGames.push_back(U(G));
    // язык: ключ запуска (-lang=, -culture=, -epiclocale= от Epic Games Launcher), язык культуры
    // Unreal (он учитывает -culture) и язык системы; книга включит первый из них, что в ней есть
    {
        TArray<FString> Tokens, Switches;
        FCommandLine::Parse(FCommandLine::Get(), Tokens, Switches);
        std::vector<std::string> Args;
        for (const FString& Sw : Switches) Args.push_back("-" + U(Sw));
        const std::string FromArgs = nlb::presentation::languageFromArgs(Args);
        if (!FromArgs.empty()) Options.preferredLanguages.push_back(FromArgs);
        Options.preferredLanguages.push_back(U(FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName()));
        Options.preferredLanguages.push_back(U(FPlatformMisc::GetDefaultLanguage()));
    }
    std::string Error;
    if (!Session->open(Options, Error)) {
        UE_LOG(LogTemp, Error, TEXT("NLB: %s (%s)"), *S(Error), *S(Options.bookPath));
        Session.reset();
        return false;
    }
    Audio->SetMedia(&Session->media());
    // Игровая площадка (OnlineSubsystem): если у книги есть platform.ini
    {
        const std::string ConfigPath = Session->platformConfigPath();
        FString PlatformLog;
        const bool bLog = FParse::Value(FCommandLine::Get(), TEXT("nlbplatformlog="), PlatformLog);
        if (!ConfigPath.empty() || bLog) {
            auto Bridge = std::make_unique<nlb::platform::PlatformBridge>(nlb::platform::PlatformConfig::load(ConfigPath));
            Bridge->add(std::make_unique<FNLBOnlinePlatformServices>(0));
            if (bLog) Bridge->add(std::make_unique<nlb::platform::LogPlatformServices>(U(PlatformLog)));
            Session->setPlatforms(std::move(Bridge));
            UE_LOG(LogTemp, Log, TEXT("NLB: площадка — OnlineSubsystem (%s)"), ConfigPath.empty() ? TEXT("без platform.ini") : *S(ConfigPath));
        }
    }
    // журнал прохождения по ключу запуска -nlblog=<файл>
    FString LogPath;
    if (FParse::Value(FCommandLine::Get(), TEXT("nlblog="), LogPath)) SetLogFile(LogPath);
    // тема игры, зашитая в пакет
    const std::string Theme = FileUtils::combinePath(Options.bookPath, BookPack::THEME_DIR);
    ThemeDir = FileUtils::isDirectory(Theme) ? S(Theme) : FString();
    const std::string BaseTheme = FileUtils::combinePath(Options.bookPath, BookPack::BASE_THEME_DIR);
    BaseThemeDir = FileUtils::isDirectory(BaseTheme) ? S(BaseTheme) : FString();
    LastRevision = Session->revision();
    EmitEvents();
    OnScreenChanged.Broadcast();
    return true;
}

void UNLBPlayerSubsystem::Deinitialize() {
    Session.reset();
    Audio.reset();
    Super::Deinitialize();
}

void UNLBPlayerSubsystem::Tick(float DeltaTime) {
    if (!Session) return;
    Session->advance(static_cast<int>(DeltaTime * 1000.0f));
    EmitEvents();
}

void UNLBPlayerSubsystem::DispatchEvents(const FNLBEngineEvents& Events) {
    using Kind = PlayerEngine::Event::Kind;
    for (const auto& E : Events.Events) {
        if (E.kind == Kind::Achievement) OnAchievement.Broadcast(S(E.text));
        else if (E.kind == Kind::ExternalGame) OnExternalGame.Broadcast(S(E.text));
        else if (E.kind == Kind::Finish) OnGameFinished.Broadcast();
    }
}

void UNLBPlayerSubsystem::EmitEvents() {
    // Пока книгу показывает экран (UNLBScreenWidget), события забирает он и сам вызывает DispatchEvents
    if (!bScreenDriven) {
        const std::vector<PlayerEngine::Event> Events = Session->takeEvents();
        DispatchEvents(FNLBEngineEvents{Events});
    }
    if (Session->revision() != LastRevision) {
        LastRevision = Session->revision();
        OnScreenChanged.Broadcast();
    }
}

FNLBScreen UNLBPlayerSubsystem::GetScreen() const {
    FNLBScreen Out;
    if (!Session) return Out;
    const ScreenModel& M = Session->screen();
    static const TCHAR* Anchors[] = {TEXT("left-top"), TEXT("middle-top"), TEXT("right-top"), TEXT("left-middle"),
        TEXT("middle-middle"), TEXT("right-middle"), TEXT("left-bottom"), TEXT("middle-bottom"), TEXT("right-bottom")};
    static const TCHAR* Effects[] = {TEXT("none"), TEXT("movein"), TEXT("moveout"), TEXT("fadein"), TEXT("fadeout"),
        TEXT("zoomin"), TEXT("zoomout"), TEXT("overlap")};
    static const TCHAR* Directions[] = {TEXT("none"), TEXT("left"), TEXT("right"), TEXT("top"), TEXT("bottom")};
    Out.Revision = static_cast<int64>(M.revision);
    Out.bVisualNovel = M.layout == Layout::VisualNovel;
    Out.PageId = S(M.pageId);
    Out.Caption = S(M.caption);
    Out.PageImage = FramePath(M.pageImage);
    Out.bImageIsBackground = M.imageIsBackground;
    Out.PageText = Paragraphs(M.pageText);
    Out.ReactionText = Paragraphs(M.reactionText);
    for (const auto& C : M.choices) {
        FNLBChoice Ch;
        Ch.Index = C.index; Ch.Text = S(C.text); Ch.bEnabled = C.enabled; Ch.AltText = S(C.altText);
        Out.Choices.Add(Ch);
    }
    for (const auto& O : M.sceneObjects) {
        FNLBSceneObject Ob;
        Ob.Id = S(O.id); Ob.ParentId = S(O.parentId); Ob.Label = S(O.label); Ob.bGraphical = O.graphical;
        Ob.Image = FramePath(O.image); Ob.HoverImage = FramePath(O.hoverImage);
        Ob.Anchor = Anchors[static_cast<int>(O.placement.anchor)];
        Ob.bHasOffset = O.placement.hasOffset;
        Ob.Offset = FVector2D(O.placement.offsetX, O.placement.offsetY);
        Ob.Arm = FVector2D(O.placement.armX, O.placement.armY);
        Ob.bHasInnerArm = O.placement.hasInnerArm;
        Ob.InnerArm = FVector2D(O.placement.innerArmX, O.placement.innerArmY);
        Ob.Effect = Effects[static_cast<int>(O.appearance.effect)];
        Ob.Direction = Directions[static_cast<int>(O.appearance.direction)];
        Ob.Steps = FMath::Max(1, O.appearance.maxStep - O.appearance.curStep);
        Ob.bVisible = O.visible; Ob.bCollapsed = O.collapsed; Ob.bActive = O.active;
        Out.Objects.Add(Ob);
    }
    for (const auto& I : M.inventory) {
        FNLBInventoryItem It;
        It.Id = S(I.id); It.ParentId = S(I.parentId); It.Label = S(I.label); It.Icon = FramePath(I.icon); It.bActive = I.active;
        Out.Inventory.Add(It);
    }
    Out.SelectedItem = S(Session->selectedItem());
    Out.bFinished = M.finished;
    Out.ExternalGame = M.externalGame ? S(*M.externalGame) : FString();
    return Out;
}

#define NLB_COMMAND(Call) \
    if (!Session) return false; \
    const bool bOk = (Call); \
    EmitEvents(); \
    return bOk;

bool UNLBPlayerSubsystem::Choose(int32 Index) { NLB_COMMAND(Session->choose(Index)) }
bool UNLBPlayerSubsystem::ClickObject(const FString& Id) { NLB_COMMAND(Session->clickObject(U(Id))) }
bool UNLBPlayerSubsystem::ClickItem(const FString& Id) { NLB_COMMAND(Session->clickItem(U(Id))) }
bool UNLBPlayerSubsystem::FinishExternalGame() { NLB_COMMAND(Session->finishExternalGame()) }

void UNLBPlayerSubsystem::ClearSelection() { if (Session) { Session->clearSelection(); EmitEvents(); } }
void UNLBPlayerSubsystem::Restart() { if (Session) { Session->restart(); EmitEvents(); } }
void UNLBPlayerSubsystem::TryAgain() { if (Session) { Session->tryAgain(); EmitEvents(); } }
void UNLBPlayerSubsystem::SetMuted(bool bMuted) { if (Session) Session->setMuted(bMuted); }
void UNLBPlayerSubsystem::SetAudioPaused(bool bPaused) { if (Audio) Audio->SetPaused(bPaused); }

bool UNLBPlayerSubsystem::IsPlatformAvailable() const {
    if (!Session || !Session->platforms()) return false;
    for (const auto& Service : Session->platforms()->services()) if (Service->available()) return true;
    return false;
}

bool UNLBPlayerSubsystem::SetLogFile(const FString& Path) {
    if (!Session) return false;
    std::string Error;
    const bool bOk = Session->setLogFile(U(Path), Error);
    // UE_LOG раскрывается в блок со своими скобками — if/else только в фигурных скобках
    if (!bOk) {
        UE_LOG(LogTemp, Warning, TEXT("NLB: %s"), *S(Error));
    } else if (!Path.IsEmpty()) {
        UE_LOG(LogTemp, Log, TEXT("NLB: журнал прохождения — %s"), *Path);
    }
    return bOk;
}

bool UNLBPlayerSubsystem::SaveGame(int32 Slot) {
    std::string Error;
    return Session && Session->save(Slot, Error);
}

bool UNLBPlayerSubsystem::LoadGame(int32 Slot) {
    std::string Error;
    if (!Session) return false;
    const bool bOk = Session->load(Slot, Error);
    EmitEvents();
    return bOk;
}

double UNLBPlayerSubsystem::GetNumber(const FString& Name) const {
    if (!Session) return 0;
    const cparse::packToken V = Session->engine().getVariable(U(Name));
    return (V->type & cparse::NUM) ? V.asDouble() : 0.0;
}
void UNLBPlayerSubsystem::SetNumber(const FString& Name, double Value) {
    if (Session) Session->engine().setVariable(U(Name), FMath::IsNearlyEqual(Value, FMath::RoundToDouble(Value))
        ? cparse::packToken(static_cast<int64_t>(Value)) : cparse::packToken(Value));
}
bool UNLBPlayerSubsystem::GetBool(const FString& Name) const {
    if (!Session) return false;
    const cparse::packToken V = Session->engine().getVariable(U(Name));
    return V->type == cparse::BOOL && V.asBool();
}
void UNLBPlayerSubsystem::SetBool(const FString& Name, bool Value) { if (Session) Session->engine().setVariable(U(Name), cparse::packToken(Value)); }
FString UNLBPlayerSubsystem::GetString(const FString& Name) const {
    if (!Session) return FString();
    const cparse::packToken V = Session->engine().getVariable(U(Name));
    return V->type == cparse::STR ? S(V.asString()) : FString();
}
void UNLBPlayerSubsystem::SetString(const FString& Name, const FString& Value) { if (Session) Session->engine().setVariable(U(Name), cparse::packToken(U(Value))); }

UTexture2D* UNLBPlayerSubsystem::LoadTexture(const FString& Path) {
    if (!Session || Path.IsEmpty()) return nullptr;
    if (TObjectPtr<UTexture2D>* Found = Textures.Find(Path)) return *Found;
    std::vector<uint8_t> Bytes;
    UTexture2D* Texture = nullptr;
    if (Session->media().read(MediaRef{U(Path)}, Bytes) && !Bytes.empty()) {
        // PNG/JPG/… из памяти — средствами движка (ImageWrapper внутри)
        Texture = FImageUtils::ImportBufferAsTexture2D(TArrayView64<const uint8>(Bytes.data(), static_cast<int64>(Bytes.size())));
    }
    Textures.Add(Path, Texture);
    return Texture;
}
