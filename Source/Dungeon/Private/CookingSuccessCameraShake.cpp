#include "CookingSuccessCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UCookingSuccessCameraShake::UCookingSuccessCameraShake()
	: UCameraShakeBase(FObjectInitializer::Get())
{
	// Only shake the camera if it's not already shaking
	bSingleInstance = true;
	
	UPerlinNoiseCameraShakePattern* SuccessPerlinPattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("SuccessPerlinPattern"));
	
	// Medium intensity shake for success - uplifting and positive feel
	SuccessPerlinPattern->X.Amplitude = 3.0f;
	SuccessPerlinPattern->X.Frequency = 15.0f;
	SuccessPerlinPattern->Y.Amplitude = 3.0f;
	SuccessPerlinPattern->Y.Frequency = 15.0f;
	SuccessPerlinPattern->Z.Amplitude = 2.5f;
	SuccessPerlinPattern->Z.Frequency = 18.0f;

	SuccessPerlinPattern->Pitch.Amplitude = 1.5f;
	SuccessPerlinPattern->Pitch.Frequency = 20.0f;
	SuccessPerlinPattern->Yaw.Amplitude = 1.5f;
	SuccessPerlinPattern->Yaw.Frequency = 20.0f;
	SuccessPerlinPattern->Roll.Amplitude = 1.0f;
	SuccessPerlinPattern->Roll.Frequency = 18.0f;

	// Subtle positive FOV shake
	SuccessPerlinPattern->FOV.Amplitude = 0.8f;
	SuccessPerlinPattern->FOV.Frequency = 12.0f;

	// Short, snappy duration for immediate feedback
	SuccessPerlinPattern->Duration = 0.4f;

	SetRootShakePattern(SuccessPerlinPattern);
} 