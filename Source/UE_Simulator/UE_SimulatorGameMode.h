// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomLogging.h"
#include "GameFramework/GameModeBase.h"
#include "UE_SimulatorGameMode.generated.h"

UCLASS(ClassGroup = (Custom))
class AUE_SimulatorGameMode : public AGameModeBase
{
	GENERATED_BODY()

	// List of active sensors in the level
	TArray<class USensorCaptureComponent const *> ActiveSensors;

public:
	AUE_SimulatorGameMode();

	bool RegisterSensor(class USensorCaptureComponent const * sensor);
};
