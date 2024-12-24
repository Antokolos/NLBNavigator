// Fill out your copyright notice in the Description page of Project Settings.

#include "NLBController.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

// Sets default values
ANLBController::ANLBController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bIsImageVisible = true; // Начальное состояние — отображение изображения
}

// Called when the game starts or when spawned
void ANLBController::BeginPlay()
{
	Super::BeginPlay();
	
    // Загрузка виджета
    if (UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/NLBWidget.NLBWidget_C")))
    {
        NLBWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
        if (NLBWidget)
        {
            NLBWidget->AddToViewport();
        }
    }

    // Поиск камеры на уровне
    GameplayCamera = Cast<ACameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass()));

    // Привязка переключения вида к клавише
    EnableInput(GetWorld()->GetFirstPlayerController());
    InputComponent->BindAction("ToggleView", IE_Pressed, this, &ANLBController::ToggleView);
}

// Called every frame
void ANLBController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

