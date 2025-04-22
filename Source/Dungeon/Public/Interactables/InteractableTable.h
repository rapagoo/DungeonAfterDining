// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableTable.generated.h"

class ACameraActor; // Forward declaration
class USceneComponent; // Forward declaration

UCLASS()
class DUNGEON_API AInteractableTable : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AInteractableTable();

protected:
	// Root component for visual representation (can be replaced with StaticMeshComponent if needed)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultSceneRoot;

	// Cooking camera actor assigned in the editor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	ACameraActor* CookingCameraActor;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Getter for the cooking camera
	UFUNCTION(BlueprintPure, Category = "Cooking")
	ACameraActor* GetCookingCamera() const { return CookingCameraActor; }
};
