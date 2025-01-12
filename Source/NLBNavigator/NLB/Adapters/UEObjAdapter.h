#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NLBCore/Public/Obj.h"
#include "UEMultiLangStringAdapter.h"
#include "UEThemeAdapter.h"
#include "UEObjAdapter.generated.h"

UENUM(BlueprintType)
enum class EContainerType : uint8
{
    Page        UMETA(DisplayName = "Page"),
    Obj         UMETA(DisplayName = "Obj"),
    None        UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class EMovementDirection : uint8
{
    Top         UMETA(DisplayName = "Top"),
    Left        UMETA(DisplayName = "Left"),
    None        UMETA(DisplayName = "None"),
    Right       UMETA(DisplayName = "Right"),
    Bottom      UMETA(DisplayName = "Bottom")
};

UENUM(BlueprintType)
enum class EEffect : uint8
{
    MoveIn      UMETA(DisplayName = "MoveIn"),
    MoveOut     UMETA(DisplayName = "MoveOut"),
    FadeIn      UMETA(DisplayName = "FadeIn"),
    FadeOut     UMETA(DisplayName = "FadeOut"),
    ZoomIn      UMETA(DisplayName = "ZoomIn"),
    ZoomOut     UMETA(DisplayName = "ZoomOut"),
    Overlap     UMETA(DisplayName = "Overlap"),
    None        UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class ECoordsOrigin : uint8
{
    LeftTop         UMETA(DisplayName = "LeftTop"),
    MiddleTop       UMETA(DisplayName = "MiddleTop"),
    RightTop        UMETA(DisplayName = "RightTop"),
    LeftMiddle      UMETA(DisplayName = "LeftMiddle"),
    MiddleMiddle    UMETA(DisplayName = "MiddleMiddle"),
    RightMiddle     UMETA(DisplayName = "RightMiddle"),
    LeftBottom      UMETA(DisplayName = "LeftBottom"),
    MiddleBottom    UMETA(DisplayName = "MiddleBottom"),
    RightBottom     UMETA(DisplayName = "RightBottom")
};

/**
 * @class UUEObjAdapter
 * @brief Unreal Engine adapter for the Obj interface
 */
UCLASS(Blueprintable)
class NLBNAVIGATOR_API UUEObjAdapter : public UObject
{
    GENERATED_BODY()

private:
    Obj* CoreObj;

    UUEMultiLangStringAdapter* ConvertToAdapter(const MultiLangString& mls) const;
    UUEThemeAdapter* ConvertToAdapter(const Theme& theme) const;

public:
    /** Sets the CoreObj instance */
    void SetCoreObj(Obj* InCoreObj);

    /** Gets the CoreObj instance */
    Obj* GetCoreObj() const;

    // Text-related methods
    UFUNCTION(BlueprintCallable, Category = "Obj|Text")
    FString GetText() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Text")
    FString GetActText() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Text")
    FString GetNouseText() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Text")
    UUEMultiLangStringAdapter* GetTexts() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Text")
    UUEMultiLangStringAdapter* GetActTexts() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Text")
    UUEMultiLangStringAdapter* GetNouseTexts() const;

    // Theme and display methods
    UFUNCTION(BlueprintCallable, Category = "Obj|Display")
    UUEThemeAdapter* GetEffectiveTheme() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Display")
    FString GetDisp() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Display")
    UUEMultiLangStringAdapter* GetDisps() const;

    // General attributes
    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    FString GetVarId() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    FString GetConstrId() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    FString GetCommonToId() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    FString GetName() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    FString GetImageFileName() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    FString GetSoundFileName() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    bool IsSoundSFX() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Attributes")
    bool IsAnimatedImage() const;

    // Flags and behavior
    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsGraphical() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsShowOnCursor() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsPreserved() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsLoadOnce() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsCollapsable() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsClearUnderTooltip() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsActOnKey() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsTakable() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsCallback() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsImageInScene() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Flags")
    bool IsImageInInventory() const;

    // Movement and effects
    UFUNCTION(BlueprintCallable, Category = "Obj|Movement")
    EMovementDirection GetMovementDirection() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Effects")
    EEffect GetEffect() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Frames")
    int32 GetStartFrame() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Frames")
    int32 GetMaxFrame() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Frames")
    int32 GetPreloadFrames() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Frames")
    int32 GetPauseFrames() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Coords")
    ECoordsOrigin GetCoordsOrigin() const;

    // Containers
    UFUNCTION(BlueprintCallable, Category = "Obj|Containers")
    FString GetContainerId() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Containers")
    EContainerType GetContainerType() const;

    UFUNCTION(BlueprintCallable, Category = "Obj|Containers")
    FString GetCumulativeText(const TArray<FString>& ObjIdsToBeExcluded, TMap<FString, FString>& VisitedVars) const;
};