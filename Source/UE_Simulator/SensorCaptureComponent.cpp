// Fill out your copyright notice in the Description page of Project Settings.

#include "SensorCaptureComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ImageUtils.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UE_SimulatorGameMode.h"

// Sets default values for this component's properties
USensorCaptureComponent::USensorCaptureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize SensorID
	SensorID = 0;

	// Initialize SceneCapture
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(this);
	SceneCapture->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the Scene capture
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bAlwaysPersistRenderingState = true;
	SceneCapture->SetVisibleFlag(true);
	SceneCapture->SetHiddenInGame(false);
}


// Called when the game starts
void USensorCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize new RenderTarget
	SceneCapture->TextureTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetOuter(), 1920, 1080, ETextureRenderTargetFormat::RTF_RGBA8, FLinearColor::Black, false, false);;
	SceneCapture->TextureTarget->SizeX = 1920;
	SceneCapture->TextureTarget->SizeY = 1080;
	SceneCapture->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	SceneCapture->TextureTarget->TargetGamma = 2.2f;

	// Register this sensor to the simulation game mode
	AUE_SimulatorGameMode* GameMode = Cast<AUE_SimulatorGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode->RegisterSensor(this)) {
		UE_LOG(LogUESimulator, Log, TEXT("Successfully registered sensor %d to simulation game mode"), SensorID);
	}
	else {
		UE_LOG(LogUESimulator, Log, TEXT("Failed to register sensor %d to simulation game mode"), SensorID);
	}
	
}


// Called every frame
void USensorCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


TArray64<uint8> USensorCaptureComponent::CaptureFrame() {

	// Trigger SceneCapture to capture the scene
	SceneCapture->CaptureScene();

	// Get the raw frame data from the render target
	TArray64<uint8> RawData;
	if (FImageUtils::GetRawData(SceneCapture->TextureTarget, RawData)) {
		// Success!
		UE_LOG(LogUESimulator, Log, TEXT("Successfully captured a frame for sensor %d"), SensorID);
	}
	else {
		// There was a problem!
		UE_LOG(LogUESimulator, Log, TEXT("Error capturing a frame for sensor %d"), SensorID);
	}

	return RawData;

}
