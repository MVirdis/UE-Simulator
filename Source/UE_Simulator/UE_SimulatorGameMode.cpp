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