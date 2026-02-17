#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSaveGame.generated.h"

UCLASS()
class LOCOMOTION_API USettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

    //// SaveGame을 식별하기 위한 슬롯 이름 (하드코딩)
    //FString SaveSlotName = "GameSettings";

    // 슬롯 이름과 유저 인덱스를 static으로 관리하여 외부(Option UI 등)에서 접근을 통일합니다.
    static FString SaveSlotName; // 세이브 파일명을 저장하는 정적 변수입니다.
    static const int32 UserIndex; // 사용자 인덱스를 정의하는 정적 상수입니다.

    // 설정 데이터를 로드하거나 없으면 새로 생성하는 정적 함수입니다.
    static USettingsSaveGame* LoadOrCreate();

    // 기본값을 설정하는 생성자
    USettingsSaveGame();
	

    // 그래픽 품질 0=Low, 1=Medium, 2=High, 3=Epic
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 GraphicsQuality;

    // 볼륨설정 값
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    float MasterVolume;

    // VSync (수직 동기화) 켜기/끄기
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableVSync;

    // 안티앨리어싱 품질 (0=Low, 1=Medium, 2=High, 3=Epic)
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 AntiAliasingQuality;

    // 창 모드 (0=Fullscreen, 1=WindowedFullscreen, 2=Windowed)
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 WindowMode;

    // 화면 해상도 저장 X,Y 좌표값 세트 (1920x1080)
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    FIntPoint ScreenResolution;

    // 마우스 감도 설정값
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Settings")
    float MouseSensitivity;
};
