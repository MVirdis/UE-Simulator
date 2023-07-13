// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CustomLogging.h"
#include "Components/SceneComponent.h"
#include "SensorCaptureComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE_SIMULATOR_API USensorCaptureComponent : public USceneComponent
{
	GENERATED_BODY()

	UPROPERTY()
	class USceneCaptureComponent2D* SceneCapture;

public:
	// Unique identifier of this sensor
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int SensorID;

	// Sets default values for this component's properties
	USensorCaptureComponent();

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Called to capture the sensor frame
	TArray64<uint8> CaptureFrame();

};
