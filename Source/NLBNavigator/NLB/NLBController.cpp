#include "NLBController.h"

#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NLBNavigatorCharacter.h"
#include "NLBPlayerSubsystem.h"
#include "NLBWidget.h"
#include "UObject/ConstructorHelpers.h"

ANLBController::ANLBController()
{
	PrimaryActorTick.bCanEverTick = false;  // время книги двигает UNLBPlayerSubsystem

	USceneComponent* DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = DefaultSceneComponent;
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);

	static ConstructorHelpers::FClassFinder<UNLBWidget> WidgetFinder(TEXT("/Game/NLB/UI/NLBWidget"));
	if (WidgetFinder.Succeeded())
	{
		WidgetClass = WidgetFinder.Class;
	}
}

void ANLBController::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		NLBWidget = CreateWidget<UNLBWidget>(PC, WidgetClass ? *WidgetClass : UNLBWidget::StaticClass());
	}
}

UNLBPlayerSubsystem* ANLBController::Player() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UNLBPlayerSubsystem>() : nullptr;
}

void ANLBController::LoadCurrentSet()
{
	UNLBPlayerSubsystem* P = Player();
	if (!P)
	{
		return;
	}
	if (!bBound)
	{
		P->OnScreenChanged.AddDynamic(this, &ANLBController::HandleScreenChanged);
		P->OnExternalGame.AddDynamic(this, &ANLBController::HandleExternalGame);
		P->OnAchievement.AddDynamic(this, &ANLBController::HandleAchievement);
		bBound = true;
	}
	if (!P->IsBookOpen() && !P->OpenBook(BookPath, bVisualNovel, ExternalGames))
	{
		UE_LOG(LogTemp, Error, TEXT("NLB: не удалось открыть книгу %s"), *BookPath);
		if (NLBWidget)
		{
			NLBWidget->UpdateText(FString::Printf(TEXT("Не удалось открыть книгу %s"), *BookPath));
		}
		return;
	}
	// Игрок вернулся после внешней игры (шутера) — книга продолжается с того же места
	if (!P->GetScreen().ExternalGame.IsEmpty())
	{
		P->FinishExternalGame();
	}
	Refresh();
}

void ANLBController::LoadNextSet()
{
	UNLBPlayerSubsystem* P = Player();
	if (!P || !P->IsBookOpen())
	{
		return;
	}
	const FNLBScreen Screen = P->GetScreen();
	int32 Available = 0, Index = -1;
	for (const FNLBChoice& C : Screen.Choices)
	{
		if (C.bEnabled)
		{
			++Available;
			Index = C.Index;
		}
	}
	if (Available == 1)
	{
		P->Choose(Index);
	}
}

void ANLBController::OnAction(ENLBAction Action, int32 Index, const FString& Id)
{
	UNLBPlayerSubsystem* P = Player();
	if (!P)
	{
		return;
	}
	switch (Action)
	{
	case ENLBAction::Choice:
		if (Index == -2) P->Restart();
		else if (Index >= 0) P->Choose(Index);
		break;
	case ENLBAction::Object:
		P->ClickObject(Id);
		break;
	case ENLBAction::Item:
		P->ClickItem(Id);
		break;
	}
	Refresh();  // выбор предмета меняет вид, даже если снимок движка тот же
}

void ANLBController::Refresh()
{
	UNLBPlayerSubsystem* P = Player();
	if (NLBWidget && P && P->IsBookOpen() && bInViewport)
	{
		NLBWidget->ShowScreen(P->GetScreen(), P, this);
	}
}

void ANLBController::HandleScreenChanged()
{
	Refresh();
}

void ANLBController::HandleExternalGame(const FString& Name)
{
	// Книга передала управление игре: вернуть игрока в шутер. Переменные книги шутер может
	// читать и менять через UNLBPlayerSubsystem (GetNumber/SetNumber/...)
	UE_LOG(LogTemp, Log, TEXT("NLB: внешняя игра %s"), *Name);
	if (ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(GetOwner()))
	{
		Character->ReturnToShooter();
	}
}

void ANLBController::HandleAchievement(const FString& Name)
{
	UE_LOG(LogTemp, Log, TEXT("NLB: достижение %s"), *Name);
}

void ANLBController::AddToViewport()
{
	if (!NLBWidget)
	{
		return;
	}
	NLBWidget->AddToViewport();
	NLBWidget->SetVisibility(ESlateVisibility::Visible);
	bInViewport = true;
	bIsImageVisible = true;
	// Курсор и ввод для интерфейса: по кнопкам щёлкают мышью
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(NLBWidget->TakeWidget());
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
}

void ANLBController::RemoveFromParent()
{
	if (NLBWidget)
	{
		NLBWidget->RemoveFromParent();
	}
	bInViewport = false;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void ANLBController::ToggleView()
{
	// Скрыть книгу и посмотреть через камеру контроллера — и обратно (как раньше)
	if (!NLBWidget)
	{
		return;
	}
	bIsImageVisible = !bIsImageVisible;
	NLBWidget->SetVisibility(bIsImageVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}
