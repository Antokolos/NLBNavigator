// Мост между шутером и плеером книг NLB: камера «новеллы», экран книги и подсистема UNLBPlayerSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NLBController.generated.h"

class UCameraComponent;
class UNLBScreenWidget;
class UNLBPlayerSubsystem;

UCLASS()
class NLBNAVIGATOR_API ANLBController : public AActor
{
	GENERATED_BODY()

public:
	ANLBController();

protected:
	virtual void BeginPlay() override;

public:
	/** Книга: путь от каталога проекта или абсолютный (каталог книги или .nlbpack с медиа и темой) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
	FString BookPath = TEXT("NLBFiles/book.nlbpack");

	/** Тема VN (true) или стандартная */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
	bool bVisualNovel = true;

	/** Варианты отдельным экраном (как в INSTEAD) или в окне текста */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
	bool bChoicesScreen = false;

	/** Внешние игры: ACT/ACTF с таким именем (или «префикс*») возвращают игрока в шутер,
	 *  а книга продолжится, когда он снова откроет её (клавиша взаимодействия у актора) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
	TArray<FString> ExternalGames = { TEXT("shooter*"), TEXT("play_*") };

	/** Показать книгу: открыть (при первом вызове) и продолжить после внешней игры */
	void LoadCurrentSet();

	/** «Следующая страница» (IA_NextPage): дальше по тексту, затем экран вариантов */
	void LoadNextSet();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION()
	void AddToViewport();

	UFUNCTION()
	void RemoveFromParent();

	UFUNCTION()
	void ToggleView();

private:
	UNLBPlayerSubsystem* Player() const;
	void ReturnToShooter();

	UFUNCTION()
	void HandleExternalGame(const FString& Name);

	UFUNCTION()
	void HandleAchievement(const FString& Name);

	UFUNCTION()
	void HandleQuit();

	/** Экран книги — тот же, что в nlb_sdl и Godot (общий GameScreen) */
	UPROPERTY()
	TObjectPtr<UNLBScreenWidget> ScreenWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	bool bIsVisible = true;
	bool bBound = false;
};
