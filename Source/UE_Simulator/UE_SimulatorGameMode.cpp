// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE_SimulatorGameMode.h"
#include "UE_SimulatorCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUE_SimulatorGameMode::AUE_SimulatorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
