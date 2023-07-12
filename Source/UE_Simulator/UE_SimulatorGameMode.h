// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomLogging.h"
#include "GameFramework/GameModeBase.h"
#include <winsock2.h>
#include "UE_SimulatorGameMode.generated.h"

UCLASS(ClassGroup = (Custom))
class AUE_SimulatorGameMode : public AGameModeBase
{
	GENERATED_BODY()

	// List of active sensors in the level
	TArray<class USensorCaptureComponent const *> ActiveSensors;

	// ---- Windows Networking -----
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int ServerPort;

private:
	bool isNetworkingInitialized;

	SOCKET ListenSocket;
	// -----------------------------

public:
	AUE_SimulatorGameMode();

	~AUE_SimulatorGameMode();

	bool RegisterSensor(class USensorCaptureComponent const * sensor);

	bool InitializeNetworking();

	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
