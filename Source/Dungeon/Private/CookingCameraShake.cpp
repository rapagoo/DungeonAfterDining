#include "CookingCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UCookingCameraShake::UCookingCameraShake()
	: UCameraShakeBase(FObjectInitializer::Get())
{
	// Only shake the camera if it's not already shaking
	bSingleInstance = true;
	
	UPerlinNoiseCameraShakePattern* CookingPerlinPattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("CookingPerlinPattern"));
	
	// Light shake for ingredient addition - subtle but noticeable
	CookingPerlinPattern->X.Amplitude = 2.0f;
	CookingPerlinPattern->X.Frequency = 8.0f;
	CookingPerlinPattern->Y.Amplitude = 2.0f;
	CookingPerlinPattern->Y.Frequency = 8.0f;
	CookingPerlinPattern->Z.Amplitude = 1.5f;
	CookingPerlinPattern->Z.Frequency = 8.0f;

	CookingPerlinPattern->Pitch.Amplitude = 1.0f;
	CookingPerlinPattern->Pitch.Frequency = 10.0f;
	CookingPerlinPattern->Yaw.Amplitude = 1.0f;
	CookingPerlinPattern->Yaw.Frequency = 10.0f;
	CookingPerlinPattern->Roll.Amplitude = 0.5f;
	CookingPerlinPattern->Roll.Frequency = 8.0f;

	// Very subtle FOV shake
	CookingPerlinPattern->FOV.Amplitude = 0.5f;
	CookingPerlinPattern->FOV.Frequency = 6.0f;

	// Short duration for ingredient addition
	CookingPerlinPattern->Duration = 0.3f;

	SetRootShakePattern(CookingPerlinPattern);
} 