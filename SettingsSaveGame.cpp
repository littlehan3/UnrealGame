#include "SettingsSaveGame.h"
#include "Kismet/GameplayStatics.h"

FString USettingsSaveGame::SaveSlotName = TEXT("GameSettingsSlot"); // 정적 변수 초기화
const int32 USettingsSaveGame::UserIndex = 0; // 단일 사용자 인덱스 0으로 초기화

USettingsSaveGame::USettingsSaveGame()
{
    // 게임 최초 실행 시 기본값
    GraphicsQuality = 3; // 3 = Epic
    MasterVolume = 0.5f; // 소리 50%
    bEnableVSync = true;       // VSync 켜기
    AntiAliasingQuality = 3; // 3 = Epic
    WindowMode = 0; // 0 = 전체 화면 (Fullscreen)
    ScreenResolution.X = 1920;
    ScreenResolution.Y = 1080;
    MouseSensitivity = 1.0f;
}

USettingsSaveGame* USettingsSaveGame::LoadOrCreate()
{
    // 세이브 파일이 하드디스크에 존재하는지 확인
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
    {
        // 파일이 있다면 로드하여 USettingsSaveGame 타입으로 형변환 후 반환
        return Cast<USettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));
    }

    // 파일이 없다면 기본값이 담긴 새로운 세이브 객체를 생성하여 반환
    return Cast<USettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
}