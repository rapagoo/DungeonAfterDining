#include "CookingPerfectCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UCookingPerfectCameraShake::UCookingPerfectCameraShake()
	: UCameraShakeBase(FObjectInitializer::Get())
{
	// Only shake the camera if it's not already shaking
	bSingleInstance = true;
	
	UPerlinNoiseCameraShakePattern* PerfectPerlinPattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("PerfectPerlinPattern"));
	
	// Exciting shake for perfect timing - celebratory and energetic
	PerfectPerlinPattern->X.Amplitude = 4.5f;
	PerfectPerlinPattern->X.Frequency = 25.0f;
	PerfectPerlinPattern->Y.Amplitude = 4.5f;
	PerfectPerlinPattern->Y.Frequency = 25.0f;
	PerfectPerlinPattern->Z.Amplitude = 3.5f;
	PerfectPerlinPattern->Z.Frequency = 28.0f;

	PerfectPerlinPattern->Pitch.Amplitude = 2.5f;
	PerfectPerlinPattern->Pitch.Frequency = 30.0f;
	PerfectPerlinPattern->Yaw.Amplitude = 2.5f;
	PerfectPerlinPattern->Yaw.Frequency = 30.0f;
	PerfectPerlinPattern->Roll.Amplitude = 2.0f;
	PerfectPerlinPattern->Roll.Frequency = 25.0f;

	// Dynamic FOV shake for perfect excitement
	PerfectPerlinPattern->FOV.Amplitude = 1.2f;
	PerfectPerlinPattern->FOV.Frequency = 20.0f;

	// Longer duration to celebrate the perfect timing
	PerfectPerlinPattern->Duration = 0.8f;

	SetRootShakePattern(PerfectPerlinPattern);
} 