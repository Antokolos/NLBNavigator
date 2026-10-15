// Прежний виджет книги (TextBlock + Image). Книгу теперь показывает UNLBScreenWidget (модуль
// NLBPlayer); класс оставлен, чтобы ассет /Game/NLB/UI/NLBWidget, унаследованный от него, загружался.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NLBWidget.generated.h"

UCLASS()
class NLBNAVIGATOR_API UNLBWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateText(const FString& NewText);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateImage(UTexture2D* NewImage);
};
