#include "NLBPlayerSubsystem.h"

#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#include "UnrealAudioBackend.h"
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

bool UNLBPlayerSubsystem::OpenBook(const FString& BookPath, bool bVisualNovel, const TArray<FString>& ExternalGames) {
    Audio = std::make_unique<FUnrealAudioBackend>(this, nullptr);
    Session = std::make_unique<nlb::presentation::Session>(Audio.get());
    Session::Options Options;
    Options.bookPath = U(FPaths::ConvertRelativePathToFull(BookPath));
    Options.exportMode = bVisualNovel ? PlayerEngine::ExportMode::VN : PlayerEngine::ExportMode::Standard;
    const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("NLBNavigator");
    IFileManager::Get().MakeDirectory(*(SaveDir / TEXT("saves")), true);
    Options.savesDir = U(SaveDir / TEXT("saves"));
    Options.achievementsPath = U(SaveDir / TEXT("achievements.tsv"));
    for (const FString& G : ExternalGames) Options.externalGames.push_back(U(G));
    std::string Error;
    if (!Session->open(Options, Error)) {
        UE_LOG(LogTemp, Error, TEXT("NLB: %s"), *S(Error));
        Session.reset();
        return false;
    }
    Audio->SetMedia(&Session->media());
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

void UNLBPlayerSubsystem::EmitEvents() {
    using Kind = PlayerEngine::Event::Kind;
    for (const auto& E : Session->takeEvents()) {
        if (E.kind == Kind::Achievement) OnAchievement.Broadcast(S(E.text));
        else if (E.kind == Kind::ExternalGame) OnExternalGame.Broadcast(S(E.text));
        else if (E.kind == Kind::Finish) OnGameFinished.Broadcast();
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
void UNLBPlayerSubsystem::SetMuted(bool bMuted) { if (Session) Session->setMuted(bMuted); }

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
        IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
        const EImageFormat Format = Module.DetectImageFormat(Bytes.data(), static_cast<int64>(Bytes.size()));
        TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(Format);
        TArray<uint8> Raw;
        if (Wrapper.IsValid() && Wrapper->SetCompressed(Bytes.data(), static_cast<int64>(Bytes.size()))
            && Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw)) {
            Texture = UTexture2D::CreateTransient(Wrapper->GetWidth(), Wrapper->GetHeight(), PF_B8G8R8A8);
            void* Data = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
            FMemory::Memcpy(Data, Raw.GetData(), Raw.Num());
            Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
            Texture->UpdateResource();
        }
    }
    Textures.Add(Path, Texture);
    return Texture;
}
