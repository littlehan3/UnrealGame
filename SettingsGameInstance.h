#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SettingsGameInstance.generated.h"

class USettingsSaveGame;
class USoundMix;
class USoundClass;

UCLASS()
class LOCOMOTION_API USettingsGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // 게임 인스턴스가 처음 초기화될 때 게임 시작 시 1회 호출
    virtual void Init() override;

    // 레벨이 로드될 때마다 호출될 함수
    virtual void OnWorldLoaded(UWorld* World);

    // 현재 설정값을 저장하는 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings")
    USettingsSaveGame* CurrentSettings = nullptr;

    // 설정을 파일에서 로드 없으면 기본값으로 새로 만듦
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void LoadSettings();

    // 현재 설정을 파일에 저장
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SaveSettings();

    // param bSaveToFile true면 적용 후 파일로 즉시 저장
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplySettings(bool bSaveToFile);

    // 그래픽 설정만 적용하는 함수
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplyGraphicsSettingsOnly();

    // 사운드 설정만 적용하는 함수
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplySoundSettingsOnly();

    // 마우스 감도만 적용하는 함수
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplyMouseSensitivityOnly(float NewSensitivity);

    // 오디오 제어용 마스터 사운드 믹스 (에디터에서 설정)
    UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
    USoundMix* MasterSoundMix = nullptr;

    // 오디오 제어를 위한 마스터 사운드 클래스 (에디터에서 설정)
    UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
    USoundClass* MasterSoundClass = nullptr;

    // 현재 모니터가 지원하는 해상도 목록을 FString 배열로 반환
    UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
    TArray<FString> GetSupportedResolutions();

    // 현재 저장된 해상도를 1920 x 1080 형식의 FString으로 반환
    UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
    FString GetCurrentResolutionAsString();

    // "1920 x 1080" 형식의 FString을 받아서 해상도를 저장
    UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
    void SetScreenResolutionFromString(const FString& ResolutionString);
};
