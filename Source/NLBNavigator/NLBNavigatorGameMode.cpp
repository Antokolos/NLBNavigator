// Copyright Epic Games, Inc. All Rights Reserved.

#include "NLBNavigatorGameMode.h"
#include "NLBNavigatorCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANLBNavigatorGameMode::ANLBNavigatorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
