#include "SettingsSaveGame.h"

USettingsSaveGame::USettingsSaveGame()
{
    // 게임 최초 실행 시 기본값
    GraphicsQuality = 3; // 3 = Epic
    MasterVolume = 0.5f;

    // [추가된 기본값]
    bEnableVSync = true;       // VSync 켜기
    AntiAliasingQuality = 3; // 3 = Epic
    // [추가된 기본값]
    WindowMode = 0; // 0 = 전체 화면 (Fullscreen)

    // [추가된 기본값]
    // 일단 1920x1080으로 잡아두고, 나중에 로드 시 바꿀 수 있음
    ScreenResolution.X = 1920;
    ScreenResolution.Y = 1080;

    MouseSensitivity = 1.0f;
}
