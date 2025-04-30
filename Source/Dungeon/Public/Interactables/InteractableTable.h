// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableTable.generated.h"

class ACameraActor; // Forward declaration
class USceneComponent; // Forward declaration
class UBoxComponent; // Forward declaration for BoxComponent
class UCookingWidget; // Forward declaration for Cooking Widget
class AInventoryItemActor; // Forward declaration for Item Actor

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

	// Area on the table where ingredients can be placed and detected
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cooking")
	UBoxComponent* IngredientArea;

	// Scene component representing the location inside the pot where ingredients should go
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cooking", meta = (AllowPrivateAccess = "true"))
	USceneComponent* PotLocationComponent;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Pointer to the active cooking widget associated with this table
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Cooking") // Transient as it's set dynamically
	TWeakObjectPtr<UCookingWidget> ActiveCookingWidget;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Getter for the cooking camera
	UFUNCTION(BlueprintPure, Category = "Cooking")
	ACameraActor* GetCookingCamera() const { return CookingCameraActor; }

	// Getter for the pot location component
	UFUNCTION(BlueprintPure, Category = "Cooking")
	USceneComponent* GetPotLocationComponent() const { return PotLocationComponent; }

	// Setter for the active cooking widget
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void SetActiveCookingWidget(UCookingWidget* Widget);
};
