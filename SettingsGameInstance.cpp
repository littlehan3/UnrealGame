#include "SettingsGameInstance.h"
#include "SettingsSaveGame.h" // 설정 저장에 사용할 클래스
#include "Kismet/GameplayStatics.h" // 저장 불러오기 사운드 제어 사용을 위해 필요
#include "GameFramework/GameUserSettings.h" // 그래픽 및 화면 설정을 제어하기 위해 필요
#include "Sound/SoundMix.h" // 사운드 믹스 사용을 위해 필요
#include "Sound/SoundClass.h" // 사운드 클래스 사용을 위해 필요
#include "Kismet/KismetSystemLibrary.h" // 해상돋 불러오기 사용을 위해 필요
#include "MainCharacter.h" // 마우스 감도 적용을 위해 메인캐릭터 필요

void USettingsGameInstance::Init()
{
    Super::Init(); // 부모 클래스의 Init을 먼저 실행

    LoadSettings(); // 게임 시작 시 저장된 설정 로드
    ApplySettings(false); // 불러온 설정을 게임에 적용 (파일에 저장하지 않음)

    // 레벨이 로드될 때마다 OnWorldLoaded 함수를 실행하도록 등록
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USettingsGameInstance::OnWorldLoaded);
}

void USettingsGameInstance::OnWorldLoaded(UWorld* World)
{
    // 새 레벨이 열렸으므로 현재 설정값을 이 레벨에도 다시 적용 (파일에 저장하지 않음)
    ApplySettings(false);
}

void USettingsGameInstance::LoadSettings()
{
    // USettingsSaveGame의 함수 호출
    CurrentSettings = USettingsSaveGame::LoadOrCreate();

    // 만약 방금 새로 생성된 객체라면(인덱스가 초기값이라면) 즉시 파일 저장하여 물리 파일 생성
    if (CurrentSettings && !UGameplayStatics::DoesSaveGameExist(USettingsSaveGame::SaveSlotName, USettingsSaveGame::UserIndex))
    {
        SaveSettings();
    }
}

void USettingsGameInstance::SaveSettings()
{
    if (CurrentSettings) // 데이터가 유효할 때만 저장
    {
        UGameplayStatics::SaveGameToSlot(CurrentSettings, USettingsSaveGame::SaveSlotName, USettingsSaveGame::UserIndex);
    }
}

void USettingsGameInstance::ApplySettings(bool bSaveToFile)
{
    if (!CurrentSettings) return;

    // 1. 그래픽 설정 적용 함수 호출
    ApplyGraphicsSettingsOnly();
    // 2. 사운드 설정 적용 함수 호출
    ApplySoundSettingsOnly();
    // 3. bSaveToFile이 true 인 경우 파일로 저장
    if (bSaveToFile)
    {
        SaveSettings();
    }
}

void USettingsGameInstance::ApplyGraphicsSettingsOnly()
{
    if (!CurrentSettings) return; // 설정값 객체가 없으면 리턴

    // UGameuserSettings 개체를 가져옴
    UGameUserSettings* UserSettings = GEngine->GetGameUserSettings();

    // 객체가 유효하다면
    if (UserSettings)
    {
        // 1. 전반적 그래픽 품질 적용
        UserSettings->SetOverallScalabilityLevel(CurrentSettings->GraphicsQuality);
        // 2. 안티앨리어싱 품질 적용
        UserSettings->SetAntiAliasingQuality(CurrentSettings->AntiAliasingQuality);
        // 3. VSync 설정 적용
        UserSettings->SetVSyncEnabled(CurrentSettings->bEnableVSync);
        // 4. 창 모드 설정 적용
        // (EWindowMode::Type)은 int32인 WindowMode 변수를 EWindowMode 열거형으로 강제 형변환하는 C++ 문법
        UserSettings->SetFullscreenMode((EWindowMode::Type)CurrentSettings->WindowMode);

        // 4. 해상도 설정 적용
        // 저장된 해상도 값이 0보다 큰지 확인하고 유효한 경우
        if (CurrentSettings->ScreenResolution.X > 0 && CurrentSettings->ScreenResolution.Y > 0)
        {
            // 화면 해상도 설정
            UserSettings->SetScreenResolution(CurrentSettings->ScreenResolution);
        }

        UserSettings->ApplySettings(false); // 변경한 그래픽 설정들을 게임에 즉시 반영 (파일 저장안함)
    }
}

void USettingsGameInstance::ApplySoundSettingsOnly()
{
    if (!CurrentSettings) return; // 설정값 객체가 없으면 리턴

    UWorld* World = GetWorld(); // 월드 가져옴
    if (!World) return; // 유효성 검사

    // 사운드 믹스, 사운드 클래스, 월드가 유효한지 확인
    if (MasterSoundMix && MasterSoundClass && World)
    {
        // 볼륨 값이 0.0에서 1.0을 벗어나지 않도록 Clamp로 보정
        float ClampedVolume = FMath::Clamp(CurrentSettings->MasterVolume, 0.0f, 1.0f);

        // 지정한 사운드 클래스의 볼륨을 덮어씌움
        UGameplayStatics::SetSoundMixClassOverride(
            World,
            MasterSoundMix,
            MasterSoundClass,
            ClampedVolume,
            1.0f, // Pitch (음높이) 는 기본값 1.0
            0.0f, // FadeInTime 을 0으로 하여 즉시 적용
            true // bAlwaysApply true로 항상 적용
        );
        // 변경 사항이 적용된 사운드 믹스를 Push(활성화) 하여 게임 월드에 적용
        UGameplayStatics::PushSoundMixModifier(World, MasterSoundMix);
    }
}

void USettingsGameInstance::ApplyMouseSensitivityOnly(float NewSensitivity)
{
    // 1. 설정값 객체가 유효하다면
    if (CurrentSettings)
    {
        // 새로운 마우스 감도 값을 메모리 (CurrentSettings) 에 저장
        CurrentSettings->MouseSensitivity = NewSensitivity;
    }

    // 2. 현재 월드에 있는 플레이어 폰을 가져와 AMainCharacter로 변환
    AMainCharacter* MainChar = Cast<AMainCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

    // 3. 플레이어 폰을 메인캐릭터로 반환 했다면
    if (MainChar)
    {
        // 캐릭터의 마우스 감도 업데이트 함수를 호출하여 실시간으로 반영
        MainChar->UpdateMouseSensitivity(NewSensitivity);
    }
}

TArray<FString> USettingsGameInstance::GetSupportedResolutions()
{
    // 반환할 문자열 배열 형태 선언 1920 x 1080
    TArray<FString> ResolutionsList;
    // 시스템 함수가 채워줄 FIntPoint 배열을 선언
    TArray<FIntPoint> Resolutions;

    // 시스템 라이브러리 함수를 호출하여 지원되는 해상도 목록을 배열에 채움
    // 호출 성공시 true 반환
    if (UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions))
    {
        // 해상도 배열 FIntPoint를 반복하면서 Fstring 배열로 변환 
        for (const FIntPoint& Res : Resolutions)
        {
            // 너비 x 높이 형태의 문자열로 만듦
            FString ResString = FString::Printf(TEXT("%d x %d"), Res.X, Res.Y);

            // 이미 목록에 추가된 해상도인지 확인하여 중복을 거름
            if (!ResolutionsList.Contains(ResString))
            {
                // 중복이 아니라면 목록에 추가
                ResolutionsList.Add(ResString);
            }
        }
    }

    // 완성된 문자열 목록을 반환
    return ResolutionsList;
}

FString USettingsGameInstance::GetCurrentResolutionAsString()
{
    if (CurrentSettings)
    {
        // 저장된 FIntPoint 값을 "너비 x 높이" 문자열로 만듭니다.
        return FString::Printf(TEXT("%d x %d"), CurrentSettings->ScreenResolution.X, CurrentSettings->ScreenResolution.Y);
    }
    return TEXT(""); // 기본값
}

void USettingsGameInstance::SetScreenResolutionFromString(const FString& ResolutionString)
{
    if (!CurrentSettings) return; // 설정값 객체가 없으면 리턴

    // 분리된 문자열을 저장할 변수 (가로 x 세로)
    FString Left, Right;

    // 입력받은 문자열을 x 기준으로 좌와 우로 분리
    if (ResolutionString.Split(TEXT(" x "), &Left, &Right))
    {
        // 왼쪽 문자열을 정수로 변환(Atoi)
        int32 Width = FCString::Atoi(*Left);
        // 오른쪽 문자열을 정수로 변환(Atoi)
        int32 Height = FCString::Atoi(*Right);

        // 변환된 너비와 높이가 모두 0보다 큰 유효한 값이라면
        if (Width > 0 && Height > 0)
        {
            // CurrentSettings 변수에 너비값을 저장
            CurrentSettings->ScreenResolution.X = Width;
            // CurrentSettings 변수에 높이값을 저장
            CurrentSettings->ScreenResolution.Y = Height;
        }
    }
}