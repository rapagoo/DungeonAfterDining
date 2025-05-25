#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "CookingStartCameraShake.generated.h"

/**
 * Camera shake effect for cooking start - more intense than ingredient addition
 */
UCLASS()
class DUNGEON_API UCookingStartCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	// Constructor
	UCookingStartCameraShake();
}; 