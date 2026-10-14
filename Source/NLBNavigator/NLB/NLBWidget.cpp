#include "NLBWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UNLBActionButton::HandleClicked()
{
	if (ANLBController* C = Owner.Get())
	{
		C->OnAction(Action, Index, Id);
	}
}

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

void UNLBWidget::AddButton(const FString& Label, ENLBAction Action, int32 Index, const FString& Id, ANLBController* Owner, bool bEnabled)
{
	UNLBActionButton* Button = WidgetTree->ConstructWidget<UNLBActionButton>(UNLBActionButton::StaticClass());
	Button->Action = Action;
	Button->Index = Index;
	Button->Id = Id;
	Button->Owner = Owner;
	Button->SetIsEnabled(bEnabled);
	Button->OnClicked.AddDynamic(Button, &UNLBActionButton::HandleClicked);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(FText::FromString(Label));
	Text->SetAutoWrapText(true);
	Button->AddChild(Text);
	if (UVerticalBoxSlot* BoxSlot = Actions->AddChildToVerticalBox(Button))
	{
		BoxSlot->SetPadding(FMargin(0.f, 2.f));
	}
}

void UNLBWidget::ShowScreen(const FNLBScreen& Screen, UNLBPlayerSubsystem* Player, ANLBController* Owner)
{
	// Текст: заголовок, текст страницы, ответ на последнее действие. Метки объектов — в «»:
	// по ним можно щёлкнуть кнопкой в списке справа
	FString Text;
	if (!Screen.Caption.IsEmpty())
	{
		Text += Screen.Caption + TEXT("\n\n");
	}
	auto AppendParagraphs = [&Text](const TArray<FNLBParagraph>& Paragraphs)
	{
		for (const FNLBParagraph& P : Paragraphs)
		{
			for (const FNLBTextRun& R : P.Runs)
			{
				Text += R.Kind == TEXT("object") ? TEXT("«") + R.Text + TEXT("»") : R.Text;
			}
			Text += TEXT("\n");
		}
	};
	AppendParagraphs(Screen.PageText);
	if (Screen.ReactionText.Num() > 0)
	{
		Text += TEXT("\n");
		AppendParagraphs(Screen.ReactionText);
	}
	UpdateText(Text);
	UpdateImage(Player ? Player->LoadTexture(Screen.PageImage) : nullptr);

	// Список кнопок создаётся один раз и кладётся в корневую панель виджета (справа)
	if (!Actions)
	{
		Actions = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NLBActions"));
		if (UPanelWidget* Root = Cast<UPanelWidget>(GetRootWidget()))
		{
			UPanelSlot* PanelSlot = Root->AddChild(Actions);
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PanelSlot))
			{
				CanvasSlot->SetAnchors(FAnchors(0.68f, 0.08f, 0.97f, 0.92f));
				CanvasSlot->SetOffsets(FMargin(0.f));
			}
		}
	}
	Actions->ClearChildren();

	// Объекты сцены, по которым можно щёлкнуть (метки «» в тексте)
	for (const FNLBSceneObject& O : Screen.Objects)
	{
		if (O.bActive && O.bVisible && !O.bCollapsed && !O.Label.IsEmpty())
		{
			// внутренние имена (abilitiesTrigger) без пробелов и кириллицы не показываем
			bool bHuman = O.Label.Contains(TEXT(" "));
			for (TCHAR C : O.Label) { bHuman = bHuman || C > 127; }
			if (bHuman)
			{
				AddButton(TEXT("«") + O.Label + TEXT("»"), ENLBAction::Object, -1, O.Id, Owner, true);
			}
		}
	}
	// Инвентарь: щелчок выбирает предмет, щелчок по объекту применяет его, повторный — осмотр
	for (const FNLBInventoryItem& I : Screen.Inventory)
	{
		const FString Mark = I.Id == Screen.SelectedItem ? TEXT("▶ ") : TEXT("• ");
		AddButton(Mark + I.Label, ENLBAction::Item, -1, I.Id, Owner, I.bActive);
	}
	// Варианты — с номерами
	int32 Number = 0;
	for (const FNLBChoice& C : Screen.Choices)
	{
		if (C.bEnabled)
		{
			AddButton(FString::Printf(TEXT("%d. %s"), ++Number, *C.Text), ENLBAction::Choice, C.Index, FString(), Owner, true);
		}
		else if (!C.AltText.IsEmpty())
		{
			AddButton(C.AltText, ENLBAction::Choice, -1, FString(), Owner, false);
		}
	}
	if (Screen.bFinished)
	{
		AddButton(TEXT("Конец. Начать заново"), ENLBAction::Choice, -2, FString(), Owner, true);
	}
}
