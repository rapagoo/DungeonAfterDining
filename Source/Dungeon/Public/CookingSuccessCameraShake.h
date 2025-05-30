#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "CookingSuccessCameraShake.generated.h"

/**
 * Camera shake effect for cooking success events - positive feedback
 */
UCLASS()
class DUNGEON_API UCookingSuccessCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	// Constructor
	UCookingSuccessCameraShake();
}; 