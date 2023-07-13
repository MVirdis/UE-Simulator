#pragma comment(lib, "Ws2_32.lib")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include "UE_SimulatorGameMode.h"
#include "UE_SimulatorCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "SensorCaptureComponent.h"

// Command Data Size Lookup-table
const int AUE_SimulatorGameMode::CMD_DATA_SIZE[MAX_CMD_CODE] = {
	7 * sizeof(double),	// Command Code == 0 => Move Agent
	0					// Command Code == 1 => Get Sensor Frame
};

AUE_SimulatorGameMode::AUE_SimulatorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// On construction networking is disabled
	isNetworkingInitialized = false;
	ServerPort = 22077;
	ListenSocket = INVALID_SOCKET;

	// Give ticking capabilities
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

bool AUE_SimulatorGameMode::RegisterSensor(USensorCaptureComponent* sensor) {
	// Check whether the list already contains the sensor
	if (ActiveSensors.Find(sensor) != INDEX_NONE) {
		UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] The sensor object is already contained in the list of active sensors."));
		return false;
	}
	else if (ActiveSensors.FindByPredicate([=](USensorCaptureComponent* ithSensor) { return (ithSensor->SensorID == sensor->SensorID); }) != nullptr) {
		UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] A sensor with the same ID (%d) is already contained in the list of active sensors."), sensor->SensorID);
		return false;
	}
	else {
		ActiveSensors.Add(sensor);
		return true;
	}
}

bool AUE_SimulatorGameMode::InitializeNetworking() {
	if (isNetworkingInitialized) return false;
	
	UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Initializing Server Listening Socket..."));

	// Make sure that networking is consistent if we exit early with error
	isNetworkingInitialized = false;
	ListenSocket = INVALID_SOCKET;

	WSADATA wsaData;
	int iResult;
	u_long iMode = 1; // Non-blocking

	// Initialize Winsock version 2
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		UE_LOG(LogUESimulator, Error, TEXT("[AUE_SimulatorGameMode] WSAStartup failed with error %d"), iResult);
		return false;
	}

	// Setup address info
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	FString PortString = FString::FromInt(ServerPort);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; // TCP transport protocol
	hints.ai_protocol = IPPROTO_TCP; // TCP transport protocol
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, (PCSTR) TCHAR_TO_ANSI(*PortString), &hints, &result);
	if (iResult != 0) {
		UE_LOG(LogUESimulator, Error, TEXT("[AUE_SimulatorGameMode] getaddrinfo failed with error %d"), iResult);
		WSACleanup();
		return false;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		UE_LOG(LogUESimulator, Error, TEXT("[AUE_SimulatorGameMode] Error at socket() with error %ld"), WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// Set the mode of the socket to non-blocking
	iResult = ioctlsocket(ListenSocket, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		UE_LOG(LogUESimulator, Error, TEXT("[AUE_SimulatorGameMode] Failed to set in non-blocking mode with error %d"), WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		ListenSocket = INVALID_SOCKET;
		WSACleanup();
		return false;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		UE_LOG(LogUESimulator, Error, TEXT("[AUE_SimulatorGameMode] bind failed with error %d"), WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		ListenSocket = INVALID_SOCKET;
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	// Put the socket into listen mode (non-blocking)
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		UE_LOG(LogUESimulator, Error, TEXT("[AUE_SimulatorGameMode] listen failed with error %d"), WSAGetLastError());
		closesocket(ListenSocket);
		ListenSocket = INVALID_SOCKET;
		WSACleanup();
		return false;
	}

	UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Successfully initialized server networking. Listening on port %d"), ServerPort);

	return isNetworkingInitialized = true;
}

AUE_SimulatorGameMode::~AUE_SimulatorGameMode() {
	UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Closing socket on destruction"));

	// General Socket cleanup
	closesocket(ListenSocket);
	WSACleanup();
}

void AUE_SimulatorGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) {
	Super::InitGame(MapName, Options, ErrorMessage);
	InitializeNetworking();
}

SocketState AUE_SimulatorGameMode::GetSocketState(SOCKET socket) {
	fd_set socketSetR, socketSetW, socketSetE;
	struct timeval selectTimeout = { 0, 0 };
	SocketState result = { false,false,false };

	if (socket == INVALID_SOCKET) {
		return result;
	}

	FD_ZERO(&socketSetR); FD_ZERO(&socketSetW); FD_ZERO(&socketSetE);
	FD_SET(socket, &socketSetR); FD_SET(socket, &socketSetW); FD_SET(socket, &socketSetE);
	select(0, &socketSetR, &socketSetW, &socketSetE, &selectTimeout);
	result.read = (bool) FD_ISSET(socket, &socketSetR);
	result.write = (bool) FD_ISSET(socket, &socketSetW);
	result.exception = (bool) FD_ISSET(socket, &socketSetE);

	return result;
}

void AUE_SimulatorGameMode::Tick(float DeltaSeconds) {
	SOCKET ClientSocket = INVALID_SOCKET;
	SocketState ListenSocketState = { false,false,false }, ClientSocketState = { false,false,false };
	int iResult;
	CommandHeader CmdHeader = {0,0,0};
	CommandData CmdData;
	bool lost = false, invalid = true;
	TArray<int> ToRemove;

	// Check whether someone tried to connect
	ListenSocketState = GetSocketState(ListenSocket);
	if (ListenSocketState.read) {
		UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Someone asked to connect."));

		// Open another socket to handle this communication
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] Error connecting with the client."));
		}
		else {
			ClientSockets.Add(ClientSocket);
			UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Successfully connected with the client. Given id %d"), ClientSockets.Num() - 1);
		}
	}

	// No clients to listen to
	if (ClientSockets.Num() == 0) return;

	// Listen to connected clients
	for (int ClientID = 0; ClientID < ClientSockets.Num(); ++ClientID) {
		lost = false; invalid = true;
		ClientSocketState.read = false; ClientSocketState.write = false; ClientSocketState.exception = false;
		
		ClientSocket = ClientSockets[ClientID];
		ClientSocketState = GetSocketState(ClientSocket);

		if (ClientSocketState.read) {
			UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Client %d has updates."), ClientID);

			// Receive the Command Header
			CmdHeader.ClientID = ClientID;
			iResult = recv(ClientSocket, reinterpret_cast<char*>( &(CmdHeader.Code) ), 2, 0);
			if (iResult > 0) { // Success we have a command
				UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Received %d bytes from client %d. Will constitute Command Header."), iResult, ClientID);
				UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Command Code: %d, Sensor ID: %d"), CmdHeader.Code, CmdHeader.SensorID);

				// Get the command data
				if (CmdHeader.Code < MAX_CMD_CODE && CMD_DATA_SIZE[CmdHeader.Code] > 0) {
					iResult = recv(ClientSocket, reinterpret_cast<char*>( &CmdData ), CMD_DATA_SIZE[CmdHeader.Code], 0);
					if (iResult > 0) {
						UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Received %d bytes of %d from client %d. Will constitute Command Data."), iResult, CMD_DATA_SIZE[CmdHeader.Code], ClientID);
						if (iResult == CMD_DATA_SIZE[CmdHeader.Code]) invalid = false;
						else {
							UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] Invalid command. Discarding"));
						}
					}
					else {
						UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Lost connection with client %d. Error code is %d. Discarding Command."), ClientID, iResult);
						lost = true;
					}
				}
				else { // If no cmd data is required we assume the command is valid
					invalid = false;
				}
			}
			else {
				UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Lost connection with client %d error code: %d."), ClientID, iResult);
				lost = true;
			}

			if (lost) {
				ToRemove.Add(ClientID);
			}
			else if(!invalid) {
				// Dispatch to command processor
				ProcessCommand(CmdHeader, CmdData);
			}
		}

	}

	// Remove lost clients
	if (ToRemove.Num() > 0) {
		for (int i = 0; i < ToRemove.Num(); ++i) {
			ClientSockets.RemoveAt(ToRemove[i]);
		}
		ToRemove.Empty();
	}
}

void AUE_SimulatorGameMode::ProcessCommand(CommandHeader CmdHeader, CommandData CmdData) {
	UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Command Processor received command %d from client %d."), CmdHeader.Code, CmdHeader.ClientID);

	USensorCaptureComponent** MatchingSensors = nullptr;
	MatchingSensors = ActiveSensors.FindByPredicate([&](USensorCaptureComponent* ithSensorCapture) { return CmdHeader.SensorID == ithSensorCapture->SensorID; });
	if (MatchingSensors == nullptr) {
		UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Cannot find sensor with id %d as requested by client %d."), CmdHeader.SensorID, CmdHeader.ClientID);
		return;
	}
	USensorCaptureComponent* MatchingSensor = MatchingSensors[0];
	
	// Various local variables
	AActor* TargetAgent = nullptr;
	TArray64<uint8> RawFrameData;
	SocketState ClientSocketState = { false,false,false };
	SOCKET ClientSocket = INVALID_SOCKET;
	int iResult;

	switch (CmdHeader.Code) {
	case 0: // Move Agent possessing the specified sensor
		UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Interpreted command: Client %d has requested to move agent with sensor %d to Location: [%.4f, %.4f, %.4f], with Rotation: [%.4f, %.4f, %.4f, %.4f] (Scalar last)"),
			CmdHeader.ClientID, CmdHeader.SensorID, CmdData.CmdDataMove.LocationX, CmdData.CmdDataMove.LocationY, CmdData.CmdDataMove.LocationZ,
			CmdData.CmdDataMove.QuatX, CmdData.CmdDataMove.QuatY, CmdData.CmdDataMove.QuatZ, CmdData.CmdDataMove.QuatScalar);

		TargetAgent = MatchingSensor->GetOwner();

		if (TargetAgent != nullptr)
			TargetAgent->SetActorLocationAndRotation(FVector(CmdData.CmdDataMove.LocationX, CmdData.CmdDataMove.LocationY, CmdData.CmdDataMove.LocationZ),
				FQuat(CmdData.CmdDataMove.QuatX, CmdData.CmdDataMove.QuatY, CmdData.CmdDataMove.QuatZ, CmdData.CmdDataMove.QuatScalar));
		break;
	case 1: // Get sensor frame

		RawFrameData = MatchingSensor->CaptureFrame();

		// Send data back to client
		ClientSocket = ClientSockets[CmdHeader.ClientID];
		ClientSocketState = GetSocketState(ClientSocket);
		if (ClientSocketState.write) {
			iResult = send(ClientSocket, reinterpret_cast<char*>(RawFrameData.GetData()), RawFrameData.Num(), 0);
			if (iResult == SOCKET_ERROR) {
				UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] Could not send frame back to client %d. The socket returned error %d"), CmdHeader.ClientID, iResult);
			}
			else {
				UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Sent frame back to client %d"), CmdHeader.ClientID);
			}
		}
		else {
			UE_LOG(LogUESimulator, Warning, TEXT("[AUE_SimulatorGameMode] Cannot send frame back to client %d. The socket is not ready to be written"), CmdHeader.ClientID);
		}

		break;
	default:
		UE_LOG(LogUESimulator, Log, TEXT("[AUE_SimulatorGameMode] Command code %d not yet implemented. Requested by client %d."), CmdHeader.Code, CmdHeader.ClientID);
	}
}