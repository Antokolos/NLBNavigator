// Меню паузы игры: продолжить, режим «обычный / VR», выход (с подтверждением). Строится в коде,
// ассет не нужен. В обычном режиме — поверх экрана, в VR — на панели перед игроком.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "NLBPauseMenuWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class ANLBNavigatorCharacter;
class UNLBPauseMenuWidget;

/** Кнопка меню: помнит свой пункт и сообщает меню */
UCLASS()
class NLBNAVIGATOR_API UNLBPauseMenuButton : public UButton
{
	GENERATED_BODY()

public:
	int32 Item = 0;
	TWeakObjectPtr<UNLBPauseMenuWidget> Menu;

	UFUNCTION()
	void HandleClicked();
};

UCLASS()
class NLBNAVIGATOR_API UNLBPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Персонаж, которым управляет меню (переключение режима, закрытие) */
	TWeakObjectPtr<ANLBNavigatorCharacter> Owner;

	/** Перестроить пункты (после смены режима или подтверждения выхода) */
	void Refresh();
	void OnItem(int32 Item);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void AddItem(const FString& Text, int32 Item);

	UPROPERTY() TObjectPtr<UVerticalBox> Items;
	UPROPERTY() TObjectPtr<UTextBlock> Status;
	bool bConfirmQuit = false;
	FString StatusText;
};
