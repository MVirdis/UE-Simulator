// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomLogging.h"
#include "GameFramework/GameModeBase.h"
#include <winsock2.h>
#include "UE_SimulatorGameMode.generated.h"

struct SocketState {
	bool read;
	bool write;
	bool exception;
};

// ---- Command Data -----------

#define MAX_CMD_CODE 2

// Command Code 0: Move agent command, expected CmdDataSize of 6*sizeof(float)
// Command Code 1: Get sensor frame command, expected CmdDataSize of 0

struct CommandHeader {
	int ClientID;
	uint8 Code;
	uint8 SensorID;
};

union CommandData {
	struct CommandDataMove {
		double LocationX;
		double LocationY;
		double LocationZ;
		double QuatX;
		double QuatY;
		double QuatZ;
		double QuatScalar;
	} CmdDataMove;
};
// -------------------------------

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

	TArray<SOCKET> ClientSockets;

	static const int CMD_DATA_SIZE[MAX_CMD_CODE];
	
	static SocketState GetSocketState(SOCKET socket);

	void ProcessCommand(CommandHeader CmdHeader, CommandData CmdData);
	// -----------------------------

public:
	AUE_SimulatorGameMode();

	~AUE_SimulatorGameMode();

	bool RegisterSensor(class USensorCaptureComponent const * sensor);

	bool InitializeNetworking();

	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	void Tick(float DeltaSeconds) override;
};
