#include "NLBInputDevice.h"

#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	/// Следит за последним вводом через предобработку Slate (видит всё: и игру, и интерфейс)
	class FDeviceTracker : public IInputProcessor
	{
	public:
		bool bGamepad = false;
		virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& E) override
		{
			bGamepad = E.GetKey().IsGamepadKey();
			return false;  // не перехватывать — только смотреть
		}
		virtual bool HandleAnalogInputEvent(FSlateApplication&, const FAnalogInputEvent& E) override
		{
			if (FMath::Abs(E.GetAnalogValue()) > 0.5f) bGamepad = true;  // стик отклонён (не дрейф)
			return false;
		}
		virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& E) override
		{
			if (!E.GetCursorDelta().IsNearlyZero(2.0f)) bGamepad = false;
			return false;
		}
		virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent&) override
		{
			bGamepad = false;
			return false;
		}
		virtual const TCHAR* GetDebugName() const override { return TEXT("NLBInputDeviceTracker"); }
	};

	TSharedPtr<FDeviceTracker>& Tracker()
	{
		static TSharedPtr<FDeviceTracker> Instance;
		if (!Instance && FSlateApplication::IsInitialized())
		{
			Instance = MakeShared<FDeviceTracker>();
			FSlateApplication::Get().RegisterInputPreProcessor(Instance);
		}
		return Instance;
	}

	enum class EPadStyle { Xbox, Nintendo, PlayStation };

	EPadStyle PadStyle()
	{
#if defined(PLATFORM_SWITCH) && PLATFORM_SWITCH
		return EPadStyle::Nintendo;
#elif (defined(PLATFORM_PS4) && PLATFORM_PS4) || (defined(PLATFORM_PS5) && PLATFORM_PS5)
		return EPadStyle::PlayStation;
#else
		return EPadStyle::Xbox;  // ПК: самые частые — Xbox-совместимые
#endif
	}
}

bool NLBInputDevice::IsUsingGamepad()
{
#if !PLATFORM_DESKTOP && !PLATFORM_ANDROID
	return true;  // консоли
#else
	const TSharedPtr<FDeviceTracker>& T = Tracker();
	return T.IsValid() && T->bGamepad;
#endif
}

FString NLBInputDevice::GamepadButtonName(const FKey& Key, bool bRu)
{
	const EPadStyle Style = PadStyle();
	// лицевые кнопки — по месту: снизу, справа, слева, сверху
	const TCHAR* Face[4][3] = {
		// Xbox, Nintendo, PlayStation
		{TEXT("A"), TEXT("B"), bRu ? TEXT("Крестик") : TEXT("Cross")},     // снизу
		{TEXT("B"), TEXT("A"), bRu ? TEXT("Кружок") : TEXT("Circle")},     // справа
		{TEXT("X"), TEXT("Y"), bRu ? TEXT("Квадрат") : TEXT("Square")},    // слева
		{TEXT("Y"), TEXT("X"), bRu ? TEXT("Треугольник") : TEXT("Triangle")},  // сверху
	};
	const TCHAR* Shoulder[4][3] = {
		{TEXT("LB"), TEXT("L"), TEXT("L1")},
		{TEXT("RB"), TEXT("R"), TEXT("R1")},
		{TEXT("LT"), TEXT("ZL"), TEXT("L2")},
		{TEXT("RT"), TEXT("ZR"), TEXT("R2")},
	};
	const int32 S = static_cast<int32>(Style);
	if (Key == EKeys::Gamepad_FaceButton_Bottom) return Face[0][S];
	if (Key == EKeys::Gamepad_FaceButton_Right) return Face[1][S];
	if (Key == EKeys::Gamepad_FaceButton_Left) return Face[2][S];
	if (Key == EKeys::Gamepad_FaceButton_Top) return Face[3][S];
	if (Key == EKeys::Gamepad_LeftShoulder) return Shoulder[0][S];
	if (Key == EKeys::Gamepad_RightShoulder) return Shoulder[1][S];
	if (Key == EKeys::Gamepad_LeftTrigger || Key == EKeys::Gamepad_LeftTriggerAxis) return Shoulder[2][S];
	if (Key == EKeys::Gamepad_RightTrigger || Key == EKeys::Gamepad_RightTriggerAxis) return Shoulder[3][S];
	if (Key == EKeys::Gamepad_LeftThumbstick) return Style == EPadStyle::Xbox ? TEXT("LS") : TEXT("L3");
	if (Key == EKeys::Gamepad_RightThumbstick) return Style == EPadStyle::Xbox ? TEXT("RS") : TEXT("R3");
	if (Key == EKeys::Gamepad_Special_Right) return Style == EPadStyle::Nintendo ? TEXT("+") : Style == EPadStyle::PlayStation ? TEXT("Options") : TEXT("Menu");
	if (Key == EKeys::Gamepad_Special_Left) return Style == EPadStyle::Nintendo ? TEXT("−") : Style == EPadStyle::PlayStation ? TEXT("Create") : TEXT("View");
	if (Key == EKeys::Gamepad_DPad_Up) return bRu ? TEXT("Вверх") : TEXT("Up");
	if (Key == EKeys::Gamepad_DPad_Down) return bRu ? TEXT("Вниз") : TEXT("Down");
	if (Key == EKeys::Gamepad_DPad_Left) return bRu ? TEXT("Влево") : TEXT("Left");
	if (Key == EKeys::Gamepad_DPad_Right) return bRu ? TEXT("Вправо") : TEXT("Right");
	return FString();
}
