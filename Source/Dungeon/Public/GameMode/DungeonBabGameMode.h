// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Inventory/MyPlayerState.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "DungeonBabGameMode.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API ADungeonBabGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADungeonBabGameMode();

protected:
	virtual void BeginPlay() override;

public:
	/** 메인 배경음악 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* BackgroundMusicComponent;

	/** 메인 배경음악 사운드 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	USoundBase* BackgroundMusic;

	/** 배경음악 볼륨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BackgroundMusicVolume = 0.7f;

	/** 게임 시작 시 자동으로 배경음악 시작 여부 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	bool bAutoStartBackgroundMusic = true;

	/* 
	 * Note: 루프 설정은 사운드 웨이브 자체의 속성으로 설정해야 합니다.
	 * 언리얼 에디터에서 사운드 웨이브를 더블클릭하여 "Looping" 체크박스를 활성화하세요.
	 */

public:
	/**
	 * 배경음악 재생
	 * @param NewMusic 재생할 음악 (nullptr이면 기본 배경음악 사용)
	 * @param bFadeOut 이전 음악을 페이드아웃할지 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayBackgroundMusic(USoundBase* NewMusic = nullptr, bool bFadeOut = true);

	/**
	 * 배경음악 정지
	 * @param bFadeOut 페이드아웃할지 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopBackgroundMusic(bool bFadeOut = true);

	/**
	 * 배경음악 일시정지
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PauseBackgroundMusic();

	/**
	 * 배경음악 재개
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void ResumeBackgroundMusic();

	/**
	 * 배경음악 볼륨 설정
	 * @param NewVolume 새로운 볼륨 (0.0 ~ 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetBackgroundMusicVolume(float NewVolume);

	/**
	 * 현재 배경음악이 재생 중인지 확인
	 * @return 재생 중이면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	bool IsBackgroundMusicPlaying() const;

	/**
	 * 글로벌 음악 매니저에서 호출되는 함수 - 미니게임 시작 시 배경음악 저장
	 * @return 현재 재생 중인 배경음악 컴포넌트
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioComponent* GetBackgroundMusicComponent() const;

protected:
	/**
	 * 배경음악 컴포넌트 초기화
	 */
	void InitializeBackgroundMusic();
	
};
