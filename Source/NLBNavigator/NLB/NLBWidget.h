// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NLBWidget.generated.h"

/**
 * 
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
	
};
