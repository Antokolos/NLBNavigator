#include "PLDInteractionWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

void UPLDInteractionWidget::UpdateText(const FString& NewText)
{
    if (UTextBlock* TextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock"))))
    {
        TextBlock->SetText(FText::FromString(NewText));
    }
}