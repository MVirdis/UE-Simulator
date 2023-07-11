// Fill out your copyright notice in the Description page of Project Settings.


#include "SensorCaptureComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"

// Sets default values for this component's properties
USensorCaptureComponent::USensorCaptureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize SensorID
	SensorID = 0;

	// Initialize RenderTarget
	RenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("RenderTarget"));
	RenderTarget->SizeX = 1920;
	RenderTarget->SizeY = 1080;
	RenderTarget->TargetGamma = 2.2f;
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;

	// Initialize SceneCapture
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(this);
	SceneCapture->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the Scene capture
	SceneCapture->TextureTarget = RenderTarget;
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

	// ...
	
}


// Called every frame
void USensorCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

