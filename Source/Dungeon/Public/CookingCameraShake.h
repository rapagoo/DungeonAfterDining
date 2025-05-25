#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "CookingCameraShake.generated.h"

/**
 * Camera shake effect for cooking interactions
 */
UCLASS()
class DUNGEON_API UCookingCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	// Constructor
	UCookingCameraShake();
}; 