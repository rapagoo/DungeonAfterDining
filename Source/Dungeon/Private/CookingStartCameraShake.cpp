#include "CookingStartCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UCookingStartCameraShake::UCookingStartCameraShake()
	: UCameraShakeBase(FObjectInitializer::Get())
{
	// Only shake the camera if it's not already shaking
	bSingleInstance = true;
	
	UPerlinNoiseCameraShakePattern* CookingStartPerlinPattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("CookingStartPerlinPattern"));
	
	// More intense shake for cooking start - represents the initial burst of activity
	CookingStartPerlinPattern->X.Amplitude = 4.0f;
	CookingStartPerlinPattern->X.Frequency = 12.0f;
	CookingStartPerlinPattern->Y.Amplitude = 4.0f;
	CookingStartPerlinPattern->Y.Frequency = 12.0f;
	CookingStartPerlinPattern->Z.Amplitude = 3.0f;
	CookingStartPerlinPattern->Z.Frequency = 10.0f;

	CookingStartPerlinPattern->Pitch.Amplitude = 2.0f;
	CookingStartPerlinPattern->Pitch.Frequency = 15.0f;
	CookingStartPerlinPattern->Yaw.Amplitude = 2.0f;
	CookingStartPerlinPattern->Yaw.Frequency = 15.0f;
	CookingStartPerlinPattern->Roll.Amplitude = 1.0f;
	CookingStartPerlinPattern->Roll.Frequency = 12.0f;

	// Slightly more noticeable FOV shake
	CookingStartPerlinPattern->FOV.Amplitude = 1.0f;
	CookingStartPerlinPattern->FOV.Frequency = 8.0f;

	// Longer duration for cooking start to show the transition
	CookingStartPerlinPattern->Duration = 0.6f;

	SetRootShakePattern(CookingStartPerlinPattern);
} 