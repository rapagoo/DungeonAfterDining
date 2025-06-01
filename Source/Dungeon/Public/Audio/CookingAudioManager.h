#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraShakeBase.h"
#include "CookingAudioManager.generated.h"

// Forward declarations
class USoundBase;
class UAudioComponent;
class UCameraShakeBase;

/**
 * 요리 오디오 설정 구조체
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FCookingAudioSettings
{
    GENERATED_BODY()

    /** 배경음악 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> BackgroundMusic;

    /** 알림음 (버튼 입력 지시) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> NotificationSound;

    /** 성공 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SuccessSound;

    /** 실패 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> FailSound;

    /** 버튼 클릭 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> ButtonClickSound;

    /** 완벽한 타이밍 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> PerfectSound;

    FCookingAudioSettings()
    {
        BackgroundMusic = nullptr;
        NotificationSound = nullptr;
        SuccessSound = nullptr;
        FailSound = nullptr;
        ButtonClickSound = nullptr;
        PerfectSound = nullptr;
    }
};

/**
 * 카메라 쉐이크 설정 구조체
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FCookingCameraShakeSettings
{
    GENERATED_BODY()

    /** 기본 요리 쉐이크 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
    TSubclassOf<UCameraShakeBase> CookingShake;

    /** 성공 시 쉐이크 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
    TSubclassOf<UCameraShakeBase> SuccessShake;

    /** 실패 시 쉐이크 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
    TSubclassOf<UCameraShakeBase> FailShake;

    /** 완벽한 타이밍 쉐이크 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
    TSubclassOf<UCameraShakeBase> PerfectShake;

    /** 요리 시작 시 쉐이크 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
    TSubclassOf<UCameraShakeBase> StartCookingShake;

    FCookingCameraShakeSettings()
    {
        CookingShake = nullptr;
        SuccessShake = nullptr;
        FailShake = nullptr;
        PerfectShake = nullptr;
        StartCookingShake = nullptr;
    }
};

/**
 * 요리 오디오 및 이펙트 매니저
 * 요리 미니게임 중 사운드와 카메라 효과를 관리합니다
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DUNGEON_API UCookingAudioManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UCookingAudioManager();

protected:
    virtual void BeginPlay() override;

    /** 현재 재생 중인 배경음악 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
    UAudioComponent* BackgroundMusicComponent;

    /** 이전 배경음악 (복원용) */
    UPROPERTY()
    UAudioComponent* PreviousBackgroundMusic;

    /** 리듬 게임 오디오 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings")
    FCookingAudioSettings RhythmGameAudio;

    /** 굽기 게임 오디오 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings")
    FCookingAudioSettings GrillingGameAudio;

    /** 기본 게임 오디오 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings")
    FCookingAudioSettings DefaultGameAudio;

    /** 카메라 쉐이크 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Effects")
    FCookingCameraShakeSettings CameraShakeSettings;

    /** 배경음악 볼륨 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BackgroundMusicVolume = 0.7f;

    /** 효과음 볼륨 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EffectSoundVolume = 1.0f;

    /** 현재 활성화된 오디오 설정 */
    FCookingAudioSettings* CurrentAudioSettings;

public:
    /**
     * 요리 미니게임 시작 시 호출
     * @param MinigameType 미니게임 타입 ("Rhythm", "Grilling", etc.)
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void StartCookingAudio(const FString& MinigameType);

    /**
     * 요리 미니게임 종료 시 호출
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void StopCookingAudio();

    /**
     * 알림음 재생 (버튼 입력 지시)
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void PlayNotificationSound();

    /**
     * 버튼 클릭 사운드 재생
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void PlayButtonClickSound();

    /**
     * 성공 사운드 재생
     * @param bPerfect 완벽한 타이밍인지 여부
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void PlaySuccessSound(bool bPerfect = false);

    /**
     * 실패 사운드 재생
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void PlayFailSound();

    /**
     * 카메라 쉐이크 실행
     * @param ShakeType 쉐이크 타입 ("Cooking", "Success", "Fail", "Perfect", "StartCooking")
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Effects")
    void TriggerCameraShake(const FString& ShakeType);

    /**
     * 점수 기반 피드백 재생
     * @param ScoreDifference 점수 변화량
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void PlayScoreBasedFeedback(float ScoreDifference);

    /**
     * 특정 요리 타입의 배경음악 재생
     * @param CookingType 요리 타입 ("Frying", "Grilling", etc.)
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void PlayCookingMusic(const FString& CookingType);

    /**
     * 이전 배경음악으로 복원
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void RestoreBackgroundMusic();

    /**
     * GameMode의 배경음악을 저장하고 미니게임 음악 시작
     * @param MinigameType 미니게임 타입
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void SaveGameModeBackgroundMusicAndStartMinigame(const FString& MinigameType);

    /**
     * GameMode 배경음악 복원
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Audio")
    void RestoreGameModeBackgroundMusic();

protected:
    /**
     * 미니게임 타입에 따른 오디오 설정 가져오기
     * @param MinigameType 미니게임 타입
     * @return 해당하는 오디오 설정
     */
    FCookingAudioSettings* GetAudioSettingsForMinigame(const FString& MinigameType);

    /**
     * 지정된 위치에서 사운드 재생
     * @param Sound 재생할 사운드
     * @param Location 재생 위치
     * @param VolumeMultiplier 볼륨 배수
     */
    void PlaySoundAtLocation(TSoftObjectPtr<USoundBase> Sound, const FVector& Location, float VolumeMultiplier = 1.0f);

    /**
     * 플레이어 컨트롤러 가져오기
     * @return 플레이어 컨트롤러
     */
    APlayerController* GetPlayerController() const;

    /**
     * 현재 GameMode 가져오기
     * @return GameMode 포인터
     */
    class ADungeonBabGameMode* GetGameMode() const;

private:
    /** GameMode에서 가져온 이전 배경음악 컴포넌트 (복원용) */
    UPROPERTY()
    UAudioComponent* SavedGameModeBackgroundMusic;
}; 