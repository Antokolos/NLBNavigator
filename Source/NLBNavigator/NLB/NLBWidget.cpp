#include "NLBWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

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
		if (NewImage)
		{
			ImageWidget->SetBrushFromTexture(NewImage, false);
			ImageWidget->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ImageWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
