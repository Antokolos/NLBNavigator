#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include <memory>

#include "NLBPlayerSubsystem.generated.h"

namespace nlb::presentation { class Session; }
class FUnrealAudioBackend;

// Удалители определены в .cpp, где типы полные: сгенерированный UHT код (конструкторы,
// раскрутка при исключениях) не должен видеть ~Session и ~FUnrealAudioBackend
struct FNLBSessionDeleter { void operator()(nlb::presentation::Session* P) const; };
struct FNLBAudioDeleter { void operator()(FUnrealAudioBackend* P) const; };
class UTexture2D;

/** Фрагмент текста: обычный текст или ссылка на объект/вариант */
USTRUCT(BlueprintType)
struct NLBPLAYER_API FNLBTextRun {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Kind;    // "text", "object", "choice"
    UPROPERTY(BlueprintReadOnly) FString Text;
    UPROPERTY(BlueprintReadOnly) FString Target;
};

USTRUCT(BlueprintType)
struct NLBPLAYER_API FNLBParagraph {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TArray<FNLBTextRun> Runs;
};

USTRUCT(BlueprintType)
struct NLBPLAYER_API FNLBChoice {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Index = 0;
    UPROPERTY(BlueprintReadOnly) FString Text;
    UPROPERTY(BlueprintReadOnly) bool bEnabled = true;
    UPROPERTY(BlueprintReadOnly) FString AltText;
};

/** Объект сцены; положение — как в INSTEAD-экспорте (см. ScreenModel.h: угол, offset, arm, iarm) */
USTRUCT(BlueprintType)
struct NLBPLAYER_API FNLBSceneObject {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Id;
    UPROPERTY(BlueprintReadOnly) FString ParentId;
    UPROPERTY(BlueprintReadOnly) FString Label;
    UPROPERTY(BlueprintReadOnly) bool bGraphical = false;
    UPROPERTY(BlueprintReadOnly) FString Image;        // текущий кадр
    UPROPERTY(BlueprintReadOnly) FString HoverImage;
    UPROPERTY(BlueprintReadOnly) FString Anchor;       // left-top … right-bottom
    UPROPERTY(BlueprintReadOnly) bool bHasOffset = false;
    UPROPERTY(BlueprintReadOnly) FVector2D Offset = FVector2D::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector2D Arm = FVector2D::ZeroVector;
    UPROPERTY(BlueprintReadOnly) bool bHasInnerArm = false;
    UPROPERTY(BlueprintReadOnly) FVector2D InnerArm = FVector2D::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FString Effect;       // none, fadein, movein, …
    UPROPERTY(BlueprintReadOnly) FString Direction;    // none, left, right, top, bottom
    UPROPERTY(BlueprintReadOnly) int32 Steps = 0;      // шагов эффекта по 20 мс
    UPROPERTY(BlueprintReadOnly) bool bVisible = true;
    UPROPERTY(BlueprintReadOnly) bool bCollapsed = false;
    UPROPERTY(BlueprintReadOnly) bool bActive = true;
};

USTRUCT(BlueprintType)
struct NLBPLAYER_API FNLBInventoryItem {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Id;
    UPROPERTY(BlueprintReadOnly) FString ParentId;
    UPROPERTY(BlueprintReadOnly) FString Label;
    UPROPERTY(BlueprintReadOnly) FString Icon;
    UPROPERTY(BlueprintReadOnly) bool bActive = true;
};

/** Снимок экрана для UMG-виджета или 3D-сцены */
USTRUCT(BlueprintType)
struct NLBPLAYER_API FNLBScreen {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
    UPROPERTY(BlueprintReadOnly) bool bVisualNovel = false;
    UPROPERTY(BlueprintReadOnly) FString PageId;
    UPROPERTY(BlueprintReadOnly) FString Caption;
    UPROPERTY(BlueprintReadOnly) FString PageImage;
    UPROPERTY(BlueprintReadOnly) bool bImageIsBackground = false;
    UPROPERTY(BlueprintReadOnly) TArray<FNLBParagraph> PageText;
    UPROPERTY(BlueprintReadOnly) TArray<FNLBParagraph> ReactionText;
    UPROPERTY(BlueprintReadOnly) TArray<FNLBChoice> Choices;
    UPROPERTY(BlueprintReadOnly) TArray<FNLBSceneObject> Objects;
    UPROPERTY(BlueprintReadOnly) TArray<FNLBInventoryItem> Inventory;
    UPROPERTY(BlueprintReadOnly) FString SelectedItem;
    UPROPERTY(BlueprintReadOnly) bool bFinished = false;
    UPROPERTY(BlueprintReadOnly) FString ExternalGame;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNLBScreenChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNLBNamedEvent, const FString&, Name);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNLBGameFinished);

/**
 * Плеер NLB на уровне GameInstance: держит сессию (книга, движок, звук), двигает время каждый
 * кадр и сообщает об изменениях. Новелла передаёт управление игре событием OnExternalGame
 * (например, показать кубик в 3D); игра меняет переменные (SetNumber/SetString/…) и вызывает
 * FinishExternalGame, чтобы вернуться в новеллу.
 */
UCLASS()
class NLBPLAYER_API UNLBPlayerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject {
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    /** Открыть книгу: каталог или .nlbpack (лучше — с --with-media) */
    UFUNCTION(BlueprintCallable, Category = "NLB")
    bool OpenBook(const FString& BookPath, bool bVisualNovel, const TArray<FString>& ExternalGames);

    UFUNCTION(BlueprintPure, Category = "NLB") bool IsBookOpen() const;
    UFUNCTION(BlueprintPure, Category = "NLB") FNLBScreen GetScreen() const;
    UFUNCTION(BlueprintCallable, Category = "NLB") bool Choose(int32 Index);
    UFUNCTION(BlueprintCallable, Category = "NLB") bool ClickObject(const FString& Id);
    UFUNCTION(BlueprintCallable, Category = "NLB") bool ClickItem(const FString& Id);
    UFUNCTION(BlueprintCallable, Category = "NLB") void ClearSelection();
    UFUNCTION(BlueprintCallable, Category = "NLB") bool FinishExternalGame();
    UFUNCTION(BlueprintCallable, Category = "NLB") void Restart();
    UFUNCTION(BlueprintCallable, Category = "NLB") bool SaveGame(int32 Slot);
    UFUNCTION(BlueprintCallable, Category = "NLB") bool LoadGame(int32 Slot);
    UFUNCTION(BlueprintCallable, Category = "NLB") void SetMuted(bool bMuted);

    /** Переменные новеллы — для мини-игр и сцен, которым передано управление */
    UFUNCTION(BlueprintPure, Category = "NLB") double GetNumber(const FString& Name) const;
    UFUNCTION(BlueprintCallable, Category = "NLB") void SetNumber(const FString& Name, double Value);
    UFUNCTION(BlueprintPure, Category = "NLB") bool GetBool(const FString& Name) const;
    UFUNCTION(BlueprintCallable, Category = "NLB") void SetBool(const FString& Name, bool Value);
    UFUNCTION(BlueprintPure, Category = "NLB") FString GetString(const FString& Name) const;
    UFUNCTION(BlueprintCallable, Category = "NLB") void SetString(const FString& Name, const FString& Value);

    /** Картинка книги как текстура (кэшируется) */
    UFUNCTION(BlueprintCallable, Category = "NLB") UTexture2D* LoadTexture(const FString& Path);

    UPROPERTY(BlueprintAssignable, Category = "NLB") FNLBScreenChanged OnScreenChanged;
    UPROPERTY(BlueprintAssignable, Category = "NLB") FNLBNamedEvent OnAchievement;
    UPROPERTY(BlueprintAssignable, Category = "NLB") FNLBNamedEvent OnExternalGame;
    UPROPERTY(BlueprintAssignable, Category = "NLB") FNLBGameFinished OnGameFinished;

    // FTickableGameObject
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UNLBPlayerSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return Session != nullptr; }

private:
    void EmitEvents();

    std::unique_ptr<FUnrealAudioBackend, FNLBAudioDeleter> Audio;
    std::unique_ptr<nlb::presentation::Session, FNLBSessionDeleter> Session;
    uint64 LastRevision = 0;
    UPROPERTY() TMap<FString, TObjectPtr<UTexture2D>> Textures;
};
