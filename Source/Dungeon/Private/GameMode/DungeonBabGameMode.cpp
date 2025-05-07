// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/DungeonBabGameMode.h"
#include "Inventory/MyPlayerState.h"

ADungeonBabGameMode::ADungeonBabGameMode()
{
    PlayerStateClass = AMyPlayerState::StaticClass();
}

