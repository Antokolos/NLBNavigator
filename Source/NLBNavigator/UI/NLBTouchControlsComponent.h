#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NLBTouchControlsComponent.generated.h"

class UTouchInterface;
class SWidget;

/**
 * Экранное управление на телефоне (не в VR): два джойстика движка (слева — ходьба, справа —
 * обзор; это левый и правый стики геймпада, на них уже настроены IA_Move и IA_Look) и кнопки с
 * подписями поверх игры: «Меню», «Действие», «Прыжок», «Огонь» (если в руках оружие). В книге —
 * только кнопка «Назад в игру»; у книги своя кнопка меню.
 *
 * Включается на устройствах с сенсорным экраном (Android, iOS), по ключу -touch (проверка на ПК)
 * или настройке «Always Show Touch Interface».
 */
UCLASS(ClassGroup = (NLB), meta = (BlueprintSpawnableComponent))
class NLBNAVIGATOR_API UNLBTouchControlsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	enum class EMode { Hidden, Shooter, Book };

	/// Нужен ли сенсорный интерфейс на этом устройстве
	static bool IsTouchDevice();

	/// Что показать: ничего, управление игрой или кнопку выхода из книги
	void SetMode(EMode NewMode);
	EMode GetMode() const { return Mode; }

	// Действия кнопок задаёт персонаж
	TFunction<void()> OnMenu, OnInteract, OnJumpStart, OnJumpStop, OnFire, OnLeaveBook;
	TFunction<bool()> CanFire;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BuildSticks();
	void BuildOverlays();
	void ShowOverlay(const TSharedPtr<SWidget>& Widget, bool bShow);
	void ApplySticks(bool bShow);

	EMode Mode = EMode::Hidden;
	UPROPERTY(Transient)
	TObjectPtr<UTouchInterface> Sticks;
	TSharedPtr<SWidget> ShooterOverlay, BookOverlay;
	bool bShooterOverlayShown = false, bBookOverlayShown = false;
};
