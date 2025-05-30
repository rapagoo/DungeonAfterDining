#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "CookingPerfectCameraShake.generated.h"

/**
 * Camera shake effect for perfect cooking events - celebratory and exciting
 */
UCLASS()
class DUNGEON_API UCookingPerfectCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	// Constructor
	UCookingPerfectCameraShake();
}; 