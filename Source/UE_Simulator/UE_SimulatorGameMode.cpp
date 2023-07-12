// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE_SimulatorGameMode.h"
#include "UE_SimulatorCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "SensorCaptureComponent.h"

AUE_SimulatorGameMode::AUE_SimulatorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

bool AUE_SimulatorGameMode::RegisterSensor(USensorCaptureComponent const* sensor) {
	// Check whether the list already contains the sensor
	if (ActiveSensors.Find(sensor) != INDEX_NONE) {
		UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] The sensor object is already contained in the list of active sensors."));
		return false;
	}
	else if (ActiveSensors.FindByPredicate([=](USensorCaptureComponent const* ithSensor) { return (ithSensor->SensorID == sensor->SensorID); }) != nullptr) {
		UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] A sensor with the same ID (%d) is already contained in the list of active sensors."), sensor->SensorID);
		return false;
	}
	else {
		ActiveSensors.Add(sensor);
		return true;
	}
}
