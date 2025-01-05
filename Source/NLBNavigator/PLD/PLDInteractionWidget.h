#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PLDInteractionWidget.generated.h"

UCLASS()
class NLBNAVIGATOR_API UPLDInteractionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateText(const FString& NewText);
	
};
