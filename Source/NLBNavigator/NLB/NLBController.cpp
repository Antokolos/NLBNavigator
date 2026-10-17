#include "NLBController.h"

#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NLBNavigatorCharacter.h"
#include "NLBPlayerSubsystem.h"
#include "NLBScreenWidget.h"
#include "NLBVRModeComponent.h"

ANLBController::ANLBController()
{
	PrimaryActorTick.bCanEverTick = false;  // время книги ведёт экран книги

	USceneComponent* DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = DefaultSceneComponent;
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
}

void ANLBController::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		ScreenWidget = CreateWidget<UNLBScreenWidget>(PC, UNLBScreenWidget::StaticClass());
		if (ScreenWidget)
		{
			ScreenWidget->bChoicesScreen = bChoicesScreen;
			ScreenWidget->OnQuitRequested.AddDynamic(this, &ANLBController::HandleQuit);
		}
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
		P->OnExternalGame.AddDynamic(this, &ANLBController::HandleExternalGame);
		P->OnAchievement.AddDynamic(this, &ANLBController::HandleAchievement);
		bBound = true;
	}
	if (!P->IsBookOpen())
	{
		UE_LOG(LogTemp, Log, TEXT("NLB: открываю книгу %s в режиме %s"), *BookPath, bVisualNovel ? TEXT("VN") : TEXT("STANDARD"));
	}
	if (!P->IsBookOpen() && !P->OpenBook(BookPath, bVisualNovel, ExternalGames))
	{
		UE_LOG(LogTemp, Error, TEXT("NLB: не удалось открыть книгу %s"), *BookPath);
		return;
	}
	// Игрок вернулся после внешней игры (шутера) — книга продолжается с того же места;
	// экран книги подхватит изменение сам
	if (!P->GetScreen().ExternalGame.IsEmpty())
	{
		P->FinishExternalGame();
	}
}

void ANLBController::LoadNextSet()
{
	if (ScreenWidget)
	{
		ScreenWidget->NextPage();
	}
}

void ANLBController::ReturnToShooter()
{
	if (ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(GetOwner()))
	{
		Character->ReturnToShooter();
	}
}

void ANLBController::HandleExternalGame(const FString& Name)
{
	// Книга передала управление игре: вернуть игрока в шутер. Переменные книги шутер может
	// читать и менять через UNLBPlayerSubsystem (GetNumber/SetNumber/...)
	UE_LOG(LogTemp, Log, TEXT("NLB: внешняя игра %s"), *Name);
	ReturnToShooter();
}

void ANLBController::HandleAchievement(const FString& Name)
{
	UE_LOG(LogTemp, Log, TEXT("NLB: достижение %s"), *Name);
}

void ANLBController::HandleQuit()
{
	// «Выйти» в меню книги — обратно в шутер; книга остаётся открытой
	ReturnToShooter();
}

void ANLBController::AddToViewport()
{
	if (!ScreenWidget)
	{
		return;
	}
	// В VR экран окна в шлеме не виден — книга на панели перед игроком, щелчки лучом
	if (ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(GetOwner()))
	{
		if (Character->IsVRMode())
		{
			Character->GetVRMode()->ShowPanel(ScreenWidget);
			bIsVisible = true;
			return;
		}
	}
	ScreenWidget->AddToViewport();
	ScreenWidget->SetVisibility(ESlateVisibility::Visible);
	bIsVisible = true;
	// Курсор и ввод для экрана книги: мышь, колесо, клавиши (пробел, цифры, Esc — меню)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(ScreenWidget->TakeWidget());
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
}

void ANLBController::RemoveFromParent()
{
	if (ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(GetOwner()))
	{
		if (Character->IsVRMode())
		{
			Character->GetVRMode()->HidePanel();
		}
	}
	if (ScreenWidget)
	{
		ScreenWidget->RemoveFromParent();
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void ANLBController::ToggleView()
{
	// Скрыть книгу и посмотреть через камеру контроллера — и обратно (как раньше)
	if (!ScreenWidget)
	{
		return;
	}
	bIsVisible = !bIsVisible;
	ScreenWidget->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}
