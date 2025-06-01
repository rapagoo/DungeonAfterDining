// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/DungeonBabGameMode.h"
#include "Inventory/MyPlayerState.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ADungeonBabGameMode::ADungeonBabGameMode()
{
    PlayerStateClass = AMyPlayerState::StaticClass();
    
    // 배경음악 컴포넌트 생성
    BackgroundMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BackgroundMusicComponent"));
    if (BackgroundMusicComponent)
    {
        BackgroundMusicComponent->bAutoActivate = false;
        BackgroundMusicComponent->SetVolumeMultiplier(BackgroundMusicVolume);
    }
}

void ADungeonBabGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeBackgroundMusic();
    
    // 자동으로 배경음악 시작
    if (bAutoStartBackgroundMusic && BackgroundMusic)
    {
        PlayBackgroundMusic();
    }
    
    UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::BeginPlay - Game mode started with background music system"));
}

void ADungeonBabGameMode::InitializeBackgroundMusic()
{
    if (!BackgroundMusicComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonBabGameMode::InitializeBackgroundMusic - BackgroundMusicComponent is null"));
        return;
    }
    
    // 배경음악 설정
    if (BackgroundMusic)
    {
        BackgroundMusicComponent->SetSound(BackgroundMusic);
        BackgroundMusicComponent->SetVolumeMultiplier(BackgroundMusicVolume);
        
        UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::InitializeBackgroundMusic - Background music initialized: %s"), 
               *BackgroundMusic->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonBabGameMode::InitializeBackgroundMusic - No background music assigned"));
    }
}

void ADungeonBabGameMode::PlayBackgroundMusic(USoundBase* NewMusic, bool bFadeOut)
{
    if (!BackgroundMusicComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonBabGameMode::PlayBackgroundMusic - BackgroundMusicComponent is null"));
        return;
    }
    
    // 새로운 음악이 지정되었으면 설정
    if (NewMusic)
    {
        // 이전 음악이 재생 중이면 정지
        if (BackgroundMusicComponent->IsPlaying())
        {
            if (bFadeOut)
            {
                BackgroundMusicComponent->FadeOut(1.0f, 0.0f);
            }
            else
            {
                BackgroundMusicComponent->Stop();
            }
        }
        
        BackgroundMusicComponent->SetSound(NewMusic);
        
        UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::PlayBackgroundMusic - Switching to new music: %s"), 
               *NewMusic->GetName());
    }
    else if (!BackgroundMusic)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonBabGameMode::PlayBackgroundMusic - No music to play"));
        return;
    }
    
    // 음악 재생
    if (bFadeOut && !BackgroundMusicComponent->IsPlaying())
    {
        BackgroundMusicComponent->FadeIn(1.0f, BackgroundMusicVolume);
    }
    else if (!BackgroundMusicComponent->IsPlaying())
    {
        BackgroundMusicComponent->Play();
    }
    
    UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::PlayBackgroundMusic - Background music started"));
}

void ADungeonBabGameMode::StopBackgroundMusic(bool bFadeOut)
{
    if (!BackgroundMusicComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonBabGameMode::StopBackgroundMusic - BackgroundMusicComponent is null"));
        return;
    }
    
    if (BackgroundMusicComponent->IsPlaying())
    {
        if (bFadeOut)
        {
            BackgroundMusicComponent->FadeOut(1.0f, 0.0f);
        }
        else
        {
            BackgroundMusicComponent->Stop();
        }
        
        UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::StopBackgroundMusic - Background music stopped"));
    }
}

void ADungeonBabGameMode::PauseBackgroundMusic()
{
    if (!BackgroundMusicComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonBabGameMode::PauseBackgroundMusic - BackgroundMusicComponent is null"));
        return;
    }
    
    if (BackgroundMusicComponent->IsPlaying())
    {
        BackgroundMusicComponent->SetPaused(true);
        UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::PauseBackgroundMusic - Background music paused"));
    }
}

void ADungeonBabGameMode::ResumeBackgroundMusic()
{
    if (!BackgroundMusicComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonBabGameMode::ResumeBackgroundMusic - BackgroundMusicComponent is null"));
        return;
    }
    
    if (BackgroundMusicComponent->GetSound() && !BackgroundMusicComponent->IsPlaying())
    {
        BackgroundMusicComponent->SetPaused(false);
        UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::ResumeBackgroundMusic - Background music resumed"));
    }
}

void ADungeonBabGameMode::SetBackgroundMusicVolume(float NewVolume)
{
    BackgroundMusicVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
    
    if (BackgroundMusicComponent)
    {
        BackgroundMusicComponent->SetVolumeMultiplier(BackgroundMusicVolume);
        UE_LOG(LogTemp, Log, TEXT("ADungeonBabGameMode::SetBackgroundMusicVolume - Volume set to: %f"), 
               BackgroundMusicVolume);
    }
}

bool ADungeonBabGameMode::IsBackgroundMusicPlaying() const
{
    if (BackgroundMusicComponent)
    {
        return BackgroundMusicComponent->IsPlaying();
    }
    return false;
}

UAudioComponent* ADungeonBabGameMode::GetBackgroundMusicComponent() const
{
    return BackgroundMusicComponent;
}

