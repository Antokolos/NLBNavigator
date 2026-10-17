#include "NLBScreenWidget.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

#include "NLBEngineEvents.h"
#include "NLBPlayerSubsystem.h"
#include "nlb/presentation/GameScreen.h"
#include "nlb/presentation/MediaSource.h"
#include "nlb/presentation/Session.h"
#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"

#include <map>

using namespace nlb::presentation;

namespace {
FString S(const std::string& V) { return UTF8_TO_TCHAR(V.c_str()); }
std::string U(const FString& V) { return std::string(TCHAR_TO_UTF8(*V)); }
FLinearColor ToLinear(Color C, float Alpha) {
    return FLinearColor(FColor(C.r, C.g, C.b, static_cast<uint8>(FMath::Clamp(C.a * Alpha, 0.0f, 255.0f))));
}
}

/**
 * Рисование команд GameScreen средствами Slate. Экран книги 1920×1080 вписывается в виджет
 * с полями; все координаты переводятся одним преобразованием (масштаб + сдвиг).
 */
class FNLBSlateRenderer : public IScreenRenderer {
public:
    explicit FNLBSlateRenderer(UNLBScreenWidget* InOwner) : Owner(InOwner) {}

    void Begin(const FGeometry& InGeometry, FSlateWindowElementList& InElements, int32 InLayer) {
        Geometry = &InGeometry;
        Elements = &InElements;
        Layer = InLayer;
        const FVector2D Size = InGeometry.GetLocalSize();
        Scale = FMath::Min(Size.X / GameScreen::SCREEN_W, Size.Y / GameScreen::SCREEN_H);
        Offset = (Size - FVector2D(GameScreen::SCREEN_W, GameScreen::SCREEN_H) * Scale) * 0.5f;
    }
    int32 End() {
        if (bClip) { Elements->PopClip(); bClip = false; }
        Geometry = nullptr;
        Elements = nullptr;
        return Layer;
    }

    bool imageSize(const std::string& Path, float& W, float& H) override {
        UTexture2D* T = Owner->TextureFor(S(Path));
        if (!T) return false;
        W = static_cast<float>(T->GetSizeX());
        H = static_cast<float>(T->GetSizeY());
        return true;
    }

    void drawImage(const std::string& Path, const RectF& Dst, float Alpha) override {
        UTexture2D* T = Owner->TextureFor(S(Path));
        if (!T || !Elements) return;
        FSlateBrush& Brush = Brushes[Path];
        if (Brush.GetResourceObject() != T) {
            Brush.SetResourceObject(T);
            Brush.ImageSize = FVector2D(T->GetSizeX(), T->GetSizeY());
            Brush.DrawAs = ESlateBrushDrawType::Image;
        }
        FSlateDrawElement::MakeBox(*Elements, ++Layer, Paint(Dst), &Brush, ESlateDrawEffect::None, FLinearColor(1, 1, 1, Alpha));
    }

    void fillRect(const RectF& R, Color C) override {
        if (!Elements) return;
        FSlateDrawElement::MakeBox(*Elements, ++Layer, Paint(R), FCoreStyle::Get().GetBrush("WhiteBrush"),
                                   ESlateDrawEffect::None, ToLinear(C, 1.0f));
    }

    float textWidth(const FontRef& Font, const std::string& Text) override {
        if (Text.empty()) return 0;
        return static_cast<float>(Measure()->Measure(S(Text), FontInfo(Font)).X);
    }
    float fontHeight(const FontRef& Font) override {
        return static_cast<float>(Measure()->GetMaxCharacterHeight(FontInfo(Font)));
    }
    float lineSkip(const FontRef& Font) override {
        return static_cast<float>(Measure()->GetMaxCharacterHeight(FontInfo(Font)));
    }

    void drawText(const FontRef& Font, const std::string& Text, Color C, float X, float Y, float Alpha) override {
        if (!Elements || Text.empty()) return;
        const FString T = S(Text);
        const FSlateFontInfo& Info = FontInfo(Font);
        const FVector2D Size = Measure()->Measure(T, Info);
        FSlateDrawElement::MakeText(*Elements, ++Layer, Paint({X, Y, static_cast<float>(Size.X), static_cast<float>(Size.Y)}),
                                    T, Info, ESlateDrawEffect::None, ToLinear(C, Alpha));
    }

    void setClip(const RectF* Clip) override {
        if (!Elements) return;
        if (bClip) { Elements->PopClip(); bClip = false; }
        if (Clip) {
            Elements->PushClip(FSlateClippingZone(Paint(*Clip)));
            bClip = true;
        }
    }

private:
    FPaintGeometry Paint(const RectF& R) const {
        return Geometry->ToPaintGeometry(FVector2D(R.w, R.h), FSlateLayoutTransform(Scale, Offset + FVector2D(R.x, R.y) * Scale));
    }
    TSharedRef<FSlateFontMeasure> Measure() const {
        return FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    }
    /// Шрифт темы (файл .ttf) или шрифт Slate по умолчанию. Размер в INSTEAD/SDL — пиксели,
    /// в Slate — пункты при 96 точках на дюйм: пиксели × 0.75
    const FSlateFontInfo& FontInfo(const FontRef& Font) {
        auto It = Fonts.find(Font);
        if (It != Fonts.end()) return It->second;
        const float Points = Font.size * 0.75f;
        FString File = Font.file.empty() ? FString() : S(Font.file);
        std::string Rel;
        if (!Font.file.empty() && BookPack::resolve(Font.file, Rel)) {
            // шрифт из пакета: Slate читает шрифты с диска — копия в Saved/NLBNavigator/Fonts
            const FString Copy = FPaths::ProjectSavedDir() / TEXT("NLBNavigator/Fonts") / FPaths::GetCleanFilename(File);
            if (!FPaths::FileExists(Copy)) {
                std::string Data;
                IFileManager::Get().MakeDirectory(*FPaths::GetPath(Copy), true);
                if (FileUtils::readFile(Font.file, Data)) {
                    TArray<uint8> Bytes(reinterpret_cast<const uint8*>(Data.data()), static_cast<int32>(Data.size()));
                    FFileHelper::SaveArrayToFile(Bytes, *Copy);
                }
            }
            File = Copy;
        }
        FSlateFontInfo Info = (!File.IsEmpty() && FPaths::FileExists(File))
            ? FSlateFontInfo(File, Points)
            : FCoreStyle::GetDefaultFontStyle("Regular", Points);
        return Fonts.emplace(Font, Info).first->second;
    }

    UNLBScreenWidget* Owner;
    const FGeometry* Geometry = nullptr;
    FSlateWindowElementList* Elements = nullptr;
    int32 Layer = 0;
    float Scale = 1.0f;
    FVector2D Offset = FVector2D::ZeroVector;
    bool bClip = false;
    std::map<std::string, FSlateBrush> Brushes;
    std::map<FontRef, FSlateFontInfo> Fonts;
};

void FNLBGameScreenDeleter::operator()(GameScreen* P) const { delete P; }
void FNLBSlateRendererDeleter::operator()(FNLBSlateRenderer* P) const { delete P; }

UNLBPlayerSubsystem* UNLBScreenWidget::Player() const {
    UGameInstance* GI = GetGameInstance();
    return GI ? GI->GetSubsystem<UNLBPlayerSubsystem>() : nullptr;
}

void UNLBScreenWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();
    SetIsFocusable(true);
}

bool UNLBScreenWidget::EnsureScreen() {
    UNLBPlayerSubsystem* P = Player();
    if (!P || !P->GetSession()) return false;
    if (Screen && ScreenSession == P->GetSession()) return true;
    Screen.reset();  // книгу открыли заново — экран для новой сессии
    ScreenSession = P->GetSession();
    if (!Renderer) Renderer.reset(new FNLBSlateRenderer(this));
    GameScreen::Options O;
    O.themeDir = U(P->GetThemeDir());
    O.baseThemeDir = U(P->GetBaseThemeDir());
    // VN — всегда экран вариантов (как paginator в VN-INSTEAD), Standard — по свойству
    O.choicesScreen = P->GetSession()->exportMode() == PlayerEngine::ExportMode::VN || bChoicesScreen;
    O.soundAvailable = true;
    O.clock = [] { return static_cast<uint64_t>(FPlatformTime::Seconds() * 1000.0); };
    Screen.reset(new GameScreen(*P->GetSession(), O));
    // события, которые забирает экран, — в делегаты подсистемы (контроллер игры слушает их)
    TWeakObjectPtr<UNLBPlayerSubsystem> Weak(P);
    Screen->setEventListener([Weak](const std::vector<PlayerEngine::Event>& Events) {
        if (UNLBPlayerSubsystem* Sub = Weak.Get()) Sub->DispatchEvents(FNLBEngineEvents{Events});
    });
    // внешнюю игру ведёт сама игра (OnExternalGame) — сообщение «её нет» не нужно
    Screen->setExternalGameHandler([](const std::string&) {});
    P->SetScreenDriven(true);
    // Полный экран — пункт меню книги: режим окна игры (UGameUserSettings), как в настройках игры
    Screen->setFullscreenHandler(
        [](bool bOn) {
            if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr) {
                S->SetFullscreenMode(bOn ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
                S->ApplySettings(false);
            }
        },
        [] {
            const UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
            return S && S->GetFullscreenMode() != EWindowMode::Windowed;
        });
    UE_LOG(LogTemp, Log, TEXT("NLB: оформление: %s"), UTF8_TO_TCHAR(Screen->describeThemes().c_str()));
    return true;
}

void UNLBScreenWidget::NativeConstruct() {
    Super::NativeConstruct();
    EnsureScreen();
}

void UNLBScreenWidget::NativeDestruct() {
    if (UNLBPlayerSubsystem* P = Player()) P->SetScreenDriven(false);
    Screen.reset();
    Super::NativeDestruct();
}

void UNLBScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime) {
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!EnsureScreen()) return;
    Screen->update(static_cast<int>(InDeltaTime * 1000.0f));
    if (Screen->quitRequested()) {
        Screen->clearQuitRequest();
        OnQuitRequested.Broadcast();
    }
}

int32 UNLBScreenWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
                                    bool bParentEnabled) const {
    int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    if (!Screen || !Renderer) return Layer;
    // GameScreen хранит состояние кадра (области щелчков, эффекты) — рисование меняет его
    UNLBScreenWidget* Self = const_cast<UNLBScreenWidget*>(this);
    Self->Renderer->Begin(AllottedGeometry, OutDrawElements, Layer);
    Self->Screen->render(*Self->Renderer);
    return Self->Renderer->End();
}

FVector2D UNLBScreenWidget::ToScreen(const FGeometry& Geometry, const FVector2D& AbsolutePosition) const {
    const FVector2D Local = Geometry.AbsoluteToLocal(AbsolutePosition);
    const FVector2D Size = Geometry.GetLocalSize();
    const float Scale = FMath::Min(Size.X / GameScreen::SCREEN_W, Size.Y / GameScreen::SCREEN_H);
    const FVector2D Offset = (Size - FVector2D(GameScreen::SCREEN_W, GameScreen::SCREEN_H) * Scale) * 0.5f;
    return Scale > 0 ? (Local - Offset) / Scale : FVector2D::ZeroVector;
}

FReply UNLBScreenWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
    if (!EnsureScreen()) return FReply::Unhandled();
    const FVector2D P = ToScreen(InGeometry, InMouseEvent.GetScreenSpacePosition());
    Screen->click(static_cast<float>(P.X), static_cast<float>(P.Y), InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton);
    return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

FReply UNLBScreenWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
    if (!EnsureScreen()) return FReply::Unhandled();
    const FVector2D P = ToScreen(InGeometry, InMouseEvent.GetScreenSpacePosition());
    Screen->mouseMove(static_cast<float>(P.X), static_cast<float>(P.Y));
    return FReply::Handled();
}

FReply UNLBScreenWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
    if (!EnsureScreen()) return FReply::Unhandled();
    Screen->wheel(InMouseEvent.GetWheelDelta());
    return FReply::Handled();
}

FReply UNLBScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) {
    if (!EnsureScreen()) return FReply::Unhandled();
    using Key = GameScreen::Key;
    const FKey K = InKeyEvent.GetKey();
    static const FKey Digits[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
                                  EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
    if (K == EKeys::Escape) Screen->key(Key::Escape);
    else if (K == EKeys::SpaceBar) Screen->key(Key::Space);
    else if (K == EKeys::Enter) Screen->key(Key::Enter);
    else if (K == EKeys::PageUp) Screen->key(Key::PageUp);
    else if (K == EKeys::PageDown) Screen->key(Key::PageDown);
    else if (K == EKeys::F5) Screen->key(Key::Save);
    else if (K == EKeys::F9) Screen->key(Key::Load);
    else {
        for (int I = 0; I < 9; ++I) {
            if (K == Digits[I]) { Screen->key(Key::Digit, I + 1); return FReply::Handled(); }
        }
        return FReply::Unhandled();
    }
    return FReply::Handled();
}

void UNLBScreenWidget::NextPage() {
    if (EnsureScreen()) Screen->key(GameScreen::Key::Space);
}

void UNLBScreenWidget::Back() {
    if (EnsureScreen()) Screen->key(GameScreen::Key::Escape);
}

UTexture2D* UNLBScreenWidget::TextureFor(const FString& Path) {
    if (Path.IsEmpty()) return nullptr;
    if (TObjectPtr<UTexture2D>* Found = Textures.Find(Path)) return *Found;
    UTexture2D* Texture = nullptr;
    const std::string P = U(Path);
    std::string Direct;
    std::vector<uint8_t> Bytes;
    // полный путь (картинки темы) — файл с диска или из пакета; иначе — медиа книги
    if (P.find_first_of("/\\") != std::string::npos && FileUtils::exists(P) && !FileUtils::isDirectory(P) && FileUtils::readFile(P, Direct)) {
        Bytes.assign(Direct.begin(), Direct.end());
    } else if (UNLBPlayerSubsystem* Sub = Player(); Sub && Sub->GetSession()) {
        Sub->GetSession()->media().read(MediaRef{P}, Bytes);
    }
    if (!Bytes.empty()) {
        Texture = FImageUtils::ImportBufferAsTexture2D(TArrayView64<const uint8>(Bytes.data(), static_cast<int64>(Bytes.size())));
    }
    if (Textures.Num() > 600) Textures.Empty();  // анимации из сотен кадров: не держать всё
    Textures.Add(Path, Texture);  // в том числе nullptr — отсутствующий файл не ищем каждый кадр
    return Texture;
}
