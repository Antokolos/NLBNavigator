// Мост между шутером и плеером книг NLB: камера «новеллы», виджет и подсистема UNLBPlayerSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NLBController.generated.h"

class UCameraComponent;
class UNLBWidget;
class UNLBPlayerSubsystem;

/** Что нажали в виджете */
UENUM()
enum class ENLBAction : uint8 { Choice, Object, Item };

UCLASS()
class NLBNAVIGATOR_API ANLBController : public AActor
{
	GENERATED_BODY()

public:
	ANLBController();

protected:
	virtual void BeginPlay() override;

public:
	/** Книга: путь от каталога проекта или абсолютный (каталог книги или .nlbpack с медиа) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
	FString BookPath = TEXT("NLBFiles/book.nlbpack");

	/** Тема VN (true) или стандартная */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
	bool bVisualNovel = true;

	/** Внешние игры: ACT/ACTF с таким именем (или «префикс*») возвращают игрока в шутер,
	 *  а книга продолжится, когда он снова откроет её (клавиша взаимодействия у актора) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NLB")
	TArray<FString> ExternalGames = { TEXT("shooter*"), TEXT("play_*") };

	/** Показать книгу: открыть (при первом вызове), продолжить после внешней игры, обновить виджет */
	void LoadCurrentSet();

	/** «Следующая страница»: если доступен ровно один вариант — выбрать его */
	void LoadNextSet();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION()
	void AddToViewport();

	UFUNCTION()
	void RemoveFromParent();

	UFUNCTION()
	void ToggleView();

	/** Нажатие в виджете: вариант, объект сцены или предмет инвентаря */
	void OnAction(ENLBAction Action, int32 Index, const FString& Id);

private:
	UNLBPlayerSubsystem* Player() const;
	void Refresh();

	UFUNCTION()
	void HandleScreenChanged();

	UFUNCTION()
	void HandleExternalGame(const FString& Name);

	UFUNCTION()
	void HandleAchievement(const FString& Name);

	UPROPERTY()
	TObjectPtr<UNLBWidget> NLBWidget;

	UPROPERTY()
	TSubclassOf<UNLBWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	bool bIsImageVisible = true;
	bool bInViewport = false;
	bool bBound = false;
};
