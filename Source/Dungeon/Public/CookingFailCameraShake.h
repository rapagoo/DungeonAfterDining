#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "CookingFailCameraShake.generated.h"

/**
 * Camera shake effect for cooking failure events - harsh negative feedback
 */
UCLASS()
class DUNGEON_API UCookingFailCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	// Constructor
	UCookingFailCameraShake();
}; 