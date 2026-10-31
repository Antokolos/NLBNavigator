#include "NLBTouchControlsComponent.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/TouchInterface.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/CommandLine.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/// Подпись по языку системы (интерфейс игры — русский и английский)
	FText Label(const TCHAR* Ru, const TCHAR* En)
	{
		const bool bRu = FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("ru");
		return FText::FromString(bRu ? Ru : En);
	}

	TSharedRef<SWidget> MakeButton(const FText& Text, TFunction<void()> Pressed, TFunction<void()> Released = nullptr,
	                               TAttribute<EVisibility> Visibility = EVisibility::Visible)
	{
		return SNew(SBox)
			.MinDesiredWidth(220)
			.MinDesiredHeight(96)
			.Visibility(Visibility)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f))
				.ForegroundColor(FLinearColor::White)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ContentPadding(FMargin(18, 10))
				.OnPressed_Lambda([Pressed]() { if (Pressed) Pressed(); })
				.OnReleased_Lambda([Released]() { if (Released) Released(); })
				[
					SNew(STextBlock)
					.Text(Text)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
					.ColorAndOpacity(FLinearColor::White)
				]
			];
	}
}

bool UNLBTouchControlsComponent::IsTouchDevice()
{
	return FPlatformMisc::GetUseVirtualJoysticks()
		|| GetDefault<UInputSettings>()->bAlwaysShowTouchInterface
		|| FParse::Param(FCommandLine::Get(), TEXT("touch"));
}

void UNLBTouchControlsComponent::SetMode(EMode NewMode)
{
	if (!IsTouchDevice())
	{
		NewMode = EMode::Hidden;
	}
	Mode = NewMode;
	if (!ShooterOverlay)
	{
		BuildOverlays();
	}
	ApplySticks(Mode == EMode::Shooter);
	ShowOverlay(ShooterOverlay, Mode == EMode::Shooter);
	ShowOverlay(BookOverlay, Mode == EMode::Book);
}

void UNLBTouchControlsComponent::BuildSticks()
{
	// Джойстики — сенсорный интерфейс движка (как MobileControls шаблона): оси левого и правого
	// стиков геймпада. Положение: доля экрана (0..1) или от правого/нижнего края (-1..0)
	UTexture2D* Back = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/MobileResources/HUD/VirtualJoystick_Background.VirtualJoystick_Background"));
	UTexture2D* Thumb = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/MobileResources/HUD/VirtualJoystick_Thumb.VirtualJoystick_Thumb"));
	if (!Back || !Thumb)
	{
		UE_LOG(LogTemp, Warning, TEXT("NLB: нет текстур экранного джойстика (/Engine/MobileResources/HUD) — джойстики выключены"));
		return;
	}
	Sticks = NewObject<UTouchInterface>(this);
	auto Stick = [&](FVector2D Center, const FKey& X, const FKey& Y) {
		FTouchInputControl C;
		C.Image1 = Thumb;
		C.Image2 = Back;
		C.Center = Center;
		C.VisualSize = FVector2D(170, 170);
		C.ThumbSize = FVector2D(80, 80);
		C.InteractionSize = FVector2D(260, 260);
		C.InputScale = FVector2D(1, 1);
		C.MainInputKey = X;
		C.AltInputKey = Y;
		Sticks->Controls.Add(C);
	};
	Stick(FVector2D(0.14f, -0.22f), EKeys::Gamepad_LeftX, EKeys::Gamepad_LeftY);    // ходьба
	Stick(FVector2D(-0.30f, -0.22f), EKeys::Gamepad_RightX, EKeys::Gamepad_RightY); // обзор (правее — кнопки)
	Sticks->ActiveOpacity = 0.8f;
	Sticks->InactiveOpacity = 0.35f;
	Sticks->TimeUntilDeactive = 0.5f;
	Sticks->TimeUntilReset = 2.0f;
}

void UNLBTouchControlsComponent::BuildOverlays()
{
	TWeakObjectPtr<UNLBTouchControlsComponent> Self(this);
	auto Call = [Self](TFunction<void()> UNLBTouchControlsComponent::*Member) {
		return [Self, Member]() { if (Self.IsValid() && (Self.Get()->*Member)) (Self.Get()->*Member)(); };
	};
	const TAttribute<EVisibility> FireVisible = TAttribute<EVisibility>::CreateLambda([Self]() {
		return Self.IsValid() && Self->CanFire && Self->CanFire() ? EVisibility::Visible : EVisibility::Collapsed;
	});
	// игра: «Меню» слева вверху; справа по центру — «Действие», «Прыжок», «Огонь»
	ShooterOverlay = SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(32, 28))
		[
			MakeButton(Label(TEXT("Меню"), TEXT("Menu")), Call(&UNLBTouchControlsComponent::OnMenu))
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin(0, 0, 40, 0))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)
			[
				MakeButton(Label(TEXT("Действие"), TEXT("Action")), Call(&UNLBTouchControlsComponent::OnInteract))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)
			[
				MakeButton(Label(TEXT("Прыжок"), TEXT("Jump")), Call(&UNLBTouchControlsComponent::OnJumpStart),
				           Call(&UNLBTouchControlsComponent::OnJumpStop))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)
			[
				MakeButton(Label(TEXT("Огонь"), TEXT("Fire")), Call(&UNLBTouchControlsComponent::OnFire), nullptr, FireVisible)
			]
		];
	// книга: только выход в игру слева вверху (кнопка меню книги — своя, справа вверху)
	BookOverlay = SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(32, 28))
		[
			MakeButton(Label(TEXT("← В игру"), TEXT("← Back")), Call(&UNLBTouchControlsComponent::OnLeaveBook))
		];
}

void UNLBTouchControlsComponent::ApplySticks(bool bShow)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}
	if (bShow && !Sticks)
	{
		BuildSticks();
	}
	PC->ActivateTouchInterface(bShow ? Sticks.Get() : nullptr);
}

void UNLBTouchControlsComponent::ShowOverlay(const TSharedPtr<SWidget>& Widget, bool bShow)
{
	bool& bShown = Widget == ShooterOverlay ? bShooterOverlayShown : bBookOverlayShown;
	if (!Widget || bShown == bShow || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	if (bShow)
	{
		GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), 20);
	}
	else
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
	}
	bShown = bShow;
}

void UNLBTouchControlsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShowOverlay(ShooterOverlay, false);
	ShowOverlay(BookOverlay, false);
	Super::EndPlay(EndPlayReason);
}
