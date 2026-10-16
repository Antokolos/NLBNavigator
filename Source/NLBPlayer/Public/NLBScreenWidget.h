#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include <memory>

#include "NLBScreenWidget.generated.h"

class UNLBPlayerSubsystem;
class UTexture2D;
namespace nlb::presentation { class GameScreen; class Session; }
class FNLBSlateRenderer;

struct FNLBGameScreenDeleter { void operator()(nlb::presentation::GameScreen* P) const; };
struct FNLBSlateRendererDeleter { void operator()(FNLBSlateRenderer* P) const; };

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNLBQuitRequested);

/**
 * Экран книги NLB в Unreal — тот же, что в nlb_sdl и Godot: общий nlb::presentation::GameScreen
 * (тема INSTEAD, полоса текста VN, выравнивание по ширине, листание, экран вариантов, спрайты по
 * правилам vn.lua с эффектами, меню). Виджет только рисует его команды средствами Slate и передаёт
 * ему мышь и клавиатуру. Книгу открывает UNLBPlayerSubsystem::OpenBook; пока виджет на экране,
 * время книги ведёт он.
 */
UCLASS()
class NLBPLAYER_API UNLBScreenWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Варианты отдельным экраном (как в INSTEAD с длинным текстом); иначе — в окне текста */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
    bool bChoicesScreen = false;

    /** Игрок выбрал «Выйти» в меню книги */
    UPROPERTY(BlueprintAssignable, Category = "NLB")
    FNLBQuitRequested OnQuitRequested;

    /** «Дальше» с клавиатуры или геймпада игры: следующая часть текста, затем экран вариантов */
    UFUNCTION(BlueprintCallable, Category = "NLB")
    void NextPage();

    // ---- для FNLBSlateRenderer
    UTexture2D* TextureFor(const FString& Path);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                              FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
                              bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    UNLBPlayerSubsystem* Player() const;
    bool EnsureScreen();
    /// Координаты виджета -> экран книги 1920×1080 (с полями)
    FVector2D ToScreen(const FGeometry& Geometry, const FVector2D& AbsolutePosition) const;

    std::unique_ptr<nlb::presentation::GameScreen, FNLBGameScreenDeleter> Screen;
    std::unique_ptr<FNLBSlateRenderer, FNLBSlateRendererDeleter> Renderer;
    const nlb::presentation::Session* ScreenSession = nullptr;  ///< сессия, для которой создан экран

    /** Картинки темы и книги, загруженные экраном (держат текстуры от сборщика мусора) */
    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>> Textures;
};
