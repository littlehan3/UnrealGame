#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSaveGame.generated.h"

UCLASS()
class LOCOMOTION_API USettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
    // 0=Low, 1=Medium, 2=High, 3=Epic
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 GraphicsQuality;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    float MasterVolume;

    // TODO: Music, SFX 볼륨 등 추가 가능

    // SaveGame을 식별하기 위한 슬롯 이름 (하드코딩)
    FString SaveSlotName = "GameSettings";

    // 기본값을 설정하는 생성자
    USettingsSaveGame();
	
    // [추가된 변수 1]
    // VSync (수직 동기화) 켜기/끄기
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableVSync;

    // [추가된 변수 2]
    // 안티앨리어싱 품질 (0=Low, 1=Medium, 2=High, 3=Epic)
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 AntiAliasingQuality;

    // [추가된 변수]
    // 창 모드 (0=Fullscreen, 1=WindowedFullscreen, 2=Windowed)
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 WindowMode;

    // [추가된 변수]
    // 화면 해상도 (예: 1920x1080)
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    FIntPoint ScreenResolution;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    float MouseSensitivity;
};
