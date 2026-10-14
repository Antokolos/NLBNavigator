// Экран книги NLB поверх игры: картинка, текст и кнопки (варианты, объекты, инвентарь).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "NLBPlayerSubsystem.h"
#include "NLBController.h"
#include "NLBWidget.generated.h"

class UVerticalBox;

/** Кнопка действия книги: помнит, что она означает, и сообщает контроллеру */
UCLASS()
class NLBNAVIGATOR_API UNLBActionButton : public UButton
{
	GENERATED_BODY()

public:
	ENLBAction Action = ENLBAction::Choice;
	int32 Index = -1;
	FString Id;
	TWeakObjectPtr<ANLBController> Owner;

	UFUNCTION()
	void HandleClicked();
};

/**
 * Виджет из /Game/NLB/UI/NLBWidget (его разметка: TextBlock «TextBlock» и Image «Image»).
 * Кнопки создаются в коде в вертикальном списке поверх корневой панели — ассет менять не нужно.
 */
UCLASS()
class NLBNAVIGATOR_API UNLBWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateText(const FString& NewText);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateImage(UTexture2D* NewImage);

	/** Показать снимок экрана книги */
	void ShowScreen(const FNLBScreen& Screen, UNLBPlayerSubsystem* Player, ANLBController* Owner);

private:
	void AddButton(const FString& Label, ENLBAction Action, int32 Index, const FString& Id, ANLBController* Owner, bool bEnabled);

	UPROPERTY()
	TObjectPtr<UVerticalBox> Actions;
};
