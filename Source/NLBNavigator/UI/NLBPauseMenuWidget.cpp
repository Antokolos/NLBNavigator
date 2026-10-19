#include "NLBPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"

#include "NLBNavigatorCharacter.h"

namespace
{
enum EItem : int32 { Continue = 0, ToggleVR = 1, Quit = 2, QuitYes = 3, QuitNo = 4 };
}

void UNLBPauseMenuButton::HandleClicked()
{
	if (UNLBPauseMenuWidget* M = Menu.Get())
	{
		M->OnItem(Item);
	}
}

void UNLBPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	// Затемнение во весь экран и список пунктов по центру
	UBorder* Back = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Back"));
	Back->SetBrushColor(FLinearColor(0, 0, 0, 0.75f));
	WidgetTree->RootWidget = Back;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Back->SetContent(Column);
	if (UBorderSlot* S = Cast<UBorderSlot>(Column->Slot))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("Пауза")));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 40;
	Title->SetFont(TitleFont);
	Title->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 30));
	Items = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Items"));
	Column->AddChildToVerticalBox(Items);
	Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Status->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.8f, 0.4f)));
	Status->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Status)->SetPadding(FMargin(0, 20, 0, 0));
	Refresh();
}

void UNLBPauseMenuWidget::AddItem(const FString& Text, int32 Item)
{
	UNLBPauseMenuButton* B = WidgetTree->ConstructWidget<UNLBPauseMenuButton>(UNLBPauseMenuButton::StaticClass());
	B->Item = Item;
	B->Menu = this;
	B->OnClicked.AddDynamic(B, &UNLBPauseMenuButton::HandleClicked);
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Text));
	FSlateFontInfo F = T->GetFont();
	F.Size = 28;
	T->SetFont(F);
	B->AddChild(T);
	Items->AddChildToVerticalBox(B)->SetPadding(FMargin(0, 6));
}

void UNLBPauseMenuWidget::Refresh()
{
	if (!Items)
	{
		return;
	}
	Items->ClearChildren();
	ANLBNavigatorCharacter* C = Owner.Get();
	if (bConfirmQuit)
	{
		AddItem(TEXT("Выйти из игры? Да"), QuitYes);
		AddItem(TEXT("Нет, вернуться"), QuitNo);
	}
	else
	{
		AddItem(TEXT("Продолжить (Esc)"), Continue);
#if !PLATFORM_ANDROID
		// на автономном шлеме (Quest) только VR — переключать нечего
		const bool bVR = C && C->IsVRMode();
		AddItem(bVR ? TEXT("Режим: VR → обычный") : TEXT("Режим: обычный → VR"), ToggleVR);
#endif
		AddItem(TEXT("Выйти из игры"), Quit);
	}
	Status->SetText(FText::FromString(StatusText));
}

void UNLBPauseMenuWidget::OnItem(int32 Item)
{
	ANLBNavigatorCharacter* C = Owner.Get();
	StatusText.Reset();
	switch (Item)
	{
	case Continue:
		if (C) C->TogglePauseMenu();
		return;
	case ToggleVR:
		if (C)
		{
			FString Error;
			// меню закроется само: панель VR и экран окна — разные места
			if (!C->SetVRMode(!C->IsVRMode(), Error))
			{
				StatusText = Error;
			}
			else
			{
				C->TogglePauseMenu();
				return;
			}
		}
		break;
	case Quit:
		bConfirmQuit = true;
		break;
	case QuitYes:
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		return;
	case QuitNo:
		bConfirmQuit = false;
		break;
	default:
		break;
	}
	Refresh();
}

FReply UNLBPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P)
	{
		if (bConfirmQuit) { bConfirmQuit = false; Refresh(); }
		else if (ANLBNavigatorCharacter* C = Owner.Get()) C->TogglePauseMenu();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
