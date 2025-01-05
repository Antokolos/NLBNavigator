// Fill out your copyright notice in the Description page of Project Settings.


#include "NLBWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

void UNLBWidget::UpdateText(const FString& NewText)
{
    if (UTextBlock* TextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock"))))
    {
        TextBlock->SetText(FText::FromString(NewText));
    }
}

void UNLBWidget::UpdateImage(UTexture2D* NewImage)
{
    if (UImage* ImageWidget = Cast<UImage>(GetWidgetFromName(TEXT("Image"))))
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(NewImage);
        ImageWidget->SetBrush(Brush);
    }
}
