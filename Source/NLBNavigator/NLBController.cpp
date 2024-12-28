// Fill out your copyright notice in the Description page of Project Settings.


#include "NLBController.h"
#include "NLBWidget.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include <Camera/CameraComponent.h>

// Sets default values
ANLBController::ANLBController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    CurrentIndex = 0;

    // Create a default scene component
    USceneComponent* DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    // Set it as the root component
    RootComponent = DefaultSceneComponent;
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ANLBController::BeginPlay()
{
	Super::BeginPlay();
	
    // Инициализация виджета
    if (UClass* WidgetClass = LoadClass<UNLBWidget>(nullptr, TEXT("/Game/UI/NLBWidget.NLBWidget_C")))
    {
        NLBWidget = CreateWidget<UNLBWidget>(GetWorld(), WidgetClass);
        if (NLBWidget)
        {
            NLBWidget->AddToViewport();
        }
    }

    // Поиск файлов на диске
    FString DirectoryPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("NLBFiles/"));
    IFileManager& FileManager = IFileManager::Get();

    FileManager.FindFiles(TextFiles, *(DirectoryPath + TEXT("*.txt")), true, false);
    FileManager.FindFiles(ImageFiles, *(DirectoryPath + TEXT("*.png")), true, false);

    TextFiles.Sort();
    ImageFiles.Sort();

    // Загрузка первого набора файлов
    LoadCurrentSet();

    // Привязка обработки ввода
    EnableInput(GetWorld()->GetFirstPlayerController());
    InputComponent->BindAction("Next", IE_Pressed, this, &ANLBController::LoadNextSet);
}

// Called every frame
void ANLBController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ANLBController::LoadCurrentSet()
{
    if (CurrentIndex < TextFiles.Num() && CurrentIndex < ImageFiles.Num())
    {
        FString DirectoryPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("NLBFiles/"));
        UE_LOG(LogTemp, Error, TEXT("Loading files from DirectoryPath = %s"), *DirectoryPath);

        // Загрузка текста
        FString TextContent;
        FString TextFilePath = DirectoryPath + TextFiles[CurrentIndex];
        if (FFileHelper::LoadFileToString(TextContent, *TextFilePath))
        {
            NLBWidget->UpdateText(TextContent);
        } else {
            UE_LOG(LogTemp, Error, TEXT("Error loading text file, path = %s"), *TextFilePath);
        }

        // Загрузка изображения
        FString ImageFilePath = DirectoryPath + ImageFiles[CurrentIndex];
        if (UTexture2D* Texture = LoadTextureFromFile(ImageFilePath))
        {
            NLBWidget->UpdateImage(Texture);
        } else {
            UE_LOG(LogTemp, Error, TEXT("Error loading texture file, path = %s"), *ImageFilePath);
        }
    }
}

void ANLBController::LoadNextSet()
{
    CurrentIndex++;
    if (CurrentIndex >= FMath::Min(TextFiles.Num(), ImageFiles.Num()))
    {
        CurrentIndex = 0; // Перезапуск с начала
    }
    LoadCurrentSet();
}

UTexture2D* ANLBController::LoadTextureFromFile(const FString& FilePath)
{
    // Проверяем, существует ли файл
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *FilePath);
        return nullptr;
    }

    // Загружаем данные файла в массив байт
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
        return nullptr;
    }

    // Создаём ImageWrapper для обработки изображения
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    EImageFormat Format = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());

    if (Format == EImageFormat::Invalid)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid image format: %s"), *FilePath);
        return nullptr;
    }

    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
    if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
    {
        // Извлекаем данные изображения
        TArray<uint8> UncompressedRGBA;
        if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedRGBA))
        {
            // Создаём новую текстуру
            UTexture2D* NewTexture = UTexture2D::CreateTransient(
                ImageWrapper->GetWidth(),
                ImageWrapper->GetHeight(),
                PF_B8G8R8A8
            );

            if (!NewTexture)
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to create texture."));
                return nullptr;
            }

            // Наполняем данные текстуры
            void* TextureData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
            FMemory::Memcpy(TextureData, UncompressedRGBA.GetData(), UncompressedRGBA.Num());
            NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

            // Обновляем ресурс текстуры
            NewTexture->UpdateResource();

            return NewTexture;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to decompress image file: %s"), *FilePath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to process compressed image data: %s"), *FilePath);
    }

    return nullptr;
}