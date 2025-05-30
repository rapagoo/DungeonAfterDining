#include "Audio/CookingAudioManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CookingCameraShake.h"
#include "CookingStartCameraShake.h"
#include "CookingSuccessCameraShake.h"
#include "CookingFailCameraShake.h"
#include "CookingPerfectCameraShake.h"

UCookingAudioManager::UCookingAudioManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // 기본 설정
    BackgroundMusicVolume = 0.7f;
    EffectSoundVolume = 1.0f;
    
    // 컴포넌트 초기화
    BackgroundMusicComponent = nullptr;
    PreviousBackgroundMusic = nullptr;
    CurrentAudioSettings = nullptr;

    // NEW: 기존 카메라 쉐이크 클래스들 사용
    CameraShakeSettings.CookingShake = UCookingCameraShake::StaticClass();
    CameraShakeSettings.SuccessShake = UCookingSuccessCameraShake::StaticClass();
    CameraShakeSettings.FailShake = UCookingFailCameraShake::StaticClass();
    CameraShakeSettings.PerfectShake = UCookingPerfectCameraShake::StaticClass();
    CameraShakeSettings.StartCookingShake = UCookingStartCameraShake::StaticClass();
}

void UCookingAudioManager::BeginPlay()
{
    Super::BeginPlay();
    
    // 오디오 컴포넌트 생성
    BackgroundMusicComponent = NewObject<UAudioComponent>(this);
    if (BackgroundMusicComponent)
    {
        BackgroundMusicComponent->AttachToComponent(GetOwner()->GetRootComponent(), 
            FAttachmentTransformRules::KeepWorldTransform);
        BackgroundMusicComponent->SetVolumeMultiplier(BackgroundMusicVolume);
        BackgroundMusicComponent->bAutoActivate = false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::BeginPlay - Audio manager initialized"));
}

void UCookingAudioManager::StartCookingAudio(const FString& MinigameType)
{
    UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::StartCookingAudio - Starting audio for %s"), *MinigameType);
    
    // 현재 오디오 설정 가져오기
    CurrentAudioSettings = GetAudioSettingsForMinigame(MinigameType);
    
    if (!CurrentAudioSettings)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::StartCookingAudio - No audio settings found for %s"), *MinigameType);
        return;
    }
    
    // 이전 배경음악 저장 (나중에 복원)
    if (BackgroundMusicComponent && BackgroundMusicComponent->IsPlaying())
    {
        PreviousBackgroundMusic = BackgroundMusicComponent;
        BackgroundMusicComponent->Stop();
    }
    
    // 요리 배경음악 시작
    if (CurrentAudioSettings->BackgroundMusic.LoadSynchronous() && BackgroundMusicComponent)
    {
        BackgroundMusicComponent->SetSound(CurrentAudioSettings->BackgroundMusic.Get());
        BackgroundMusicComponent->SetVolumeMultiplier(BackgroundMusicVolume);
        // Note: Looping should be set in the sound asset itself in the editor
        BackgroundMusicComponent->Play();
        
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::StartCookingAudio - Background music started"));
    }
    
    // 요리 시작 카메라 쉐이크
    TriggerCameraShake(TEXT("StartCooking"));
}

void UCookingAudioManager::StopCookingAudio()
{
    UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::StopCookingAudio - Stopping cooking audio"));
    
    // 요리 배경음악 중지
    if (BackgroundMusicComponent && BackgroundMusicComponent->IsPlaying())
    {
        BackgroundMusicComponent->Stop();
    }
    
    // 이전 배경음악 복원 (있다면)
    if (PreviousBackgroundMusic && PreviousBackgroundMusic->IsValidLowLevel())
    {
        BackgroundMusicComponent->SetSound(PreviousBackgroundMusic->GetSound());
        BackgroundMusicComponent->Play();
        PreviousBackgroundMusic = nullptr;
        
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::StopCookingAudio - Previous background music restored"));
    }
    
    CurrentAudioSettings = nullptr;
}

void UCookingAudioManager::PlayNotificationSound()
{
    if (!CurrentAudioSettings)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::PlayNotificationSound - No current audio settings"));
        return;
    }
    
    if (CurrentAudioSettings->NotificationSound.LoadSynchronous())
    {
        FVector Location = GetOwner()->GetActorLocation();
        PlaySoundAtLocation(CurrentAudioSettings->NotificationSound, Location, EffectSoundVolume * 1.2f);
        
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::PlayNotificationSound - Notification sound played"));
    }
}

void UCookingAudioManager::PlayButtonClickSound()
{
    if (!CurrentAudioSettings)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::PlayButtonClickSound - No current audio settings"));
        return;
    }
    
    if (CurrentAudioSettings->ButtonClickSound.LoadSynchronous())
    {
        FVector Location = GetOwner()->GetActorLocation();
        PlaySoundAtLocation(CurrentAudioSettings->ButtonClickSound, Location, EffectSoundVolume);
        
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::PlayButtonClickSound - Button click sound played"));
    }
}

void UCookingAudioManager::PlaySuccessSound(bool bPerfect)
{
    if (!CurrentAudioSettings)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::PlaySuccessSound - No current audio settings"));
        return;
    }
    
    FVector Location = GetOwner()->GetActorLocation();
    
    if (bPerfect && CurrentAudioSettings->PerfectSound.LoadSynchronous())
    {
        PlaySoundAtLocation(CurrentAudioSettings->PerfectSound, Location, EffectSoundVolume * 1.3f);
        TriggerCameraShake(TEXT("Perfect"));
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::PlaySuccessSound - Perfect sound played"));
    }
    else if (CurrentAudioSettings->SuccessSound.LoadSynchronous())
    {
        PlaySoundAtLocation(CurrentAudioSettings->SuccessSound, Location, EffectSoundVolume);
        TriggerCameraShake(TEXT("Success"));
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::PlaySuccessSound - Success sound played"));
    }
}

void UCookingAudioManager::PlayFailSound()
{
    if (!CurrentAudioSettings)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::PlayFailSound - No current audio settings"));
        return;
    }
    
    if (CurrentAudioSettings->FailSound.LoadSynchronous())
    {
        FVector Location = GetOwner()->GetActorLocation();
        PlaySoundAtLocation(CurrentAudioSettings->FailSound, Location, EffectSoundVolume * 0.8f);
        TriggerCameraShake(TEXT("Fail"));
        
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::PlayFailSound - Fail sound played"));
    }
}

void UCookingAudioManager::TriggerCameraShake(const FString& ShakeType)
{
    APlayerController* PlayerController = GetPlayerController();
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::TriggerCameraShake - No player controller found"));
        return;
    }
    
    TSubclassOf<UCameraShakeBase> ShakeClass = nullptr;
    
    if (ShakeType == TEXT("Cooking"))
    {
        ShakeClass = CameraShakeSettings.CookingShake;
    }
    else if (ShakeType == TEXT("Success"))
    {
        ShakeClass = CameraShakeSettings.SuccessShake;
    }
    else if (ShakeType == TEXT("Fail"))
    {
        ShakeClass = CameraShakeSettings.FailShake;
    }
    else if (ShakeType == TEXT("Perfect"))
    {
        ShakeClass = CameraShakeSettings.PerfectShake;
    }
    else if (ShakeType == TEXT("StartCooking"))
    {
        ShakeClass = CameraShakeSettings.StartCookingShake;
    }
    
    if (ShakeClass)
    {
        PlayerController->ClientStartCameraShake(ShakeClass);
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::TriggerCameraShake - %s shake triggered"), *ShakeType);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::TriggerCameraShake - No shake class for %s"), *ShakeType);
    }
}

void UCookingAudioManager::PlayScoreBasedFeedback(float ScoreDifference)
{
    if (ScoreDifference > 0)
    {
        // 성공적인 입력
        if (ScoreDifference >= 100.0f)
        {
            // Perfect
            PlaySuccessSound(true);
        }
        else if (ScoreDifference >= 75.0f)
        {
            // Good
            PlaySuccessSound(false);
        }
        else
        {
            // Hit
            PlayButtonClickSound();
            TriggerCameraShake(TEXT("Success"));
        }
    }
    else if (ScoreDifference < 0)
    {
        // 실패한 입력
        PlayFailSound();
    }
    else
    {
        // 점수 변화 없음 (잘못된 타이밍)
        PlayButtonClickSound();
    }
}

FCookingAudioSettings* UCookingAudioManager::GetAudioSettingsForMinigame(const FString& MinigameType)
{
    if (MinigameType.Contains(TEXT("Rhythm")))
    {
        return &RhythmGameAudio;
    }
    else if (MinigameType.Contains(TEXT("Grilling")))
    {
        return &GrillingGameAudio;
    }
    else
    {
        return &DefaultGameAudio;
    }
}

void UCookingAudioManager::PlaySoundAtLocation(TSoftObjectPtr<USoundBase> Sound, const FVector& Location, float VolumeMultiplier)
{
    if (!Sound.LoadSynchronous())
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::PlaySoundAtLocation - Sound failed to load"));
        return;
    }
    
    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(), 
        Sound.Get(), 
        Location, 
        VolumeMultiplier * EffectSoundVolume
    );
}

APlayerController* UCookingAudioManager::GetPlayerController() const
{
    if (UWorld* World = GetWorld())
    {
        return World->GetFirstPlayerController();
    }
    return nullptr;
}

void UCookingAudioManager::PlayCookingMusic(const FString& CookingType)
{
    UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::PlayCookingMusic - Starting music for %s"), *CookingType);
    
    // 요리 타입에 맞는 오디오 설정 가져오기
    FCookingAudioSettings* Settings = GetAudioSettingsForMinigame(CookingType);
    
    if (!Settings)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingAudioManager::PlayCookingMusic - No audio settings found for %s"), *CookingType);
        return;
    }
    
    // 현재 재생 중인 배경음악 저장 (복원용)
    if (BackgroundMusicComponent && BackgroundMusicComponent->IsPlaying())
    {
        PreviousBackgroundMusic = BackgroundMusicComponent;
        BackgroundMusicComponent->Stop();
    }
    
    // 새로운 배경음악 시작
    if (Settings->BackgroundMusic.LoadSynchronous() && BackgroundMusicComponent)
    {
        BackgroundMusicComponent->SetSound(Settings->BackgroundMusic.Get());
        BackgroundMusicComponent->SetVolumeMultiplier(BackgroundMusicVolume);
        BackgroundMusicComponent->Play();
        
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::PlayCookingMusic - Background music started for %s"), *CookingType);
    }
}

void UCookingAudioManager::RestoreBackgroundMusic()
{
    UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::RestoreBackgroundMusic - Restoring previous background music"));
    
    // 현재 배경음악 중지
    if (BackgroundMusicComponent && BackgroundMusicComponent->IsPlaying())
    {
        BackgroundMusicComponent->Stop();
    }
    
    // 이전 배경음악 복원
    if (PreviousBackgroundMusic && PreviousBackgroundMusic->IsValidLowLevel())
    {
        BackgroundMusicComponent->SetSound(PreviousBackgroundMusic->GetSound());
        BackgroundMusicComponent->Play();
        PreviousBackgroundMusic = nullptr;
        
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::RestoreBackgroundMusic - Previous background music restored"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UCookingAudioManager::RestoreBackgroundMusic - No previous background music to restore"));
    }
} 