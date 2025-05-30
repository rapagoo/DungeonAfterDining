#include "CookingFailCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UCookingFailCameraShake::UCookingFailCameraShake()
	: UCameraShakeBase(FObjectInitializer::Get())
{
	// Only shake the camera if it's not already shaking
	bSingleInstance = true;
	
	UPerlinNoiseCameraShakePattern* FailPerlinPattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("FailPerlinPattern"));
	
	// Harsh shake for failure - jarring and negative feel
	FailPerlinPattern->X.Amplitude = 5.0f;
	FailPerlinPattern->X.Frequency = 8.0f;
	FailPerlinPattern->Y.Amplitude = 5.0f;
	FailPerlinPattern->Y.Frequency = 8.0f;
	FailPerlinPattern->Z.Amplitude = 4.0f;
	FailPerlinPattern->Z.Frequency = 6.0f;

	FailPerlinPattern->Pitch.Amplitude = 3.0f;
	FailPerlinPattern->Pitch.Frequency = 7.0f;
	FailPerlinPattern->Yaw.Amplitude = 3.0f;
	FailPerlinPattern->Yaw.Frequency = 7.0f;
	FailPerlinPattern->Roll.Amplitude = 2.5f;
	FailPerlinPattern->Roll.Frequency = 9.0f;

	// More pronounced FOV shake for failure impact
	FailPerlinPattern->FOV.Amplitude = 1.5f;
	FailPerlinPattern->FOV.Frequency = 5.0f;

	// Longer duration to emphasize the failure
	FailPerlinPattern->Duration = 0.7f;

	SetRootShakePattern(FailPerlinPattern);
} 