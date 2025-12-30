// 이 파일이 항상 .cpp 파일의 *첫 번째* include여야 합니다.
#include "MainMenuWidget.h" 

// --- 기능에 필요한 헤더 파일들 ---
#include "Components/Button.h"        // UButton의 OnClicked 이벤트를 사용하기 위해 필요
#include "Kismet/GameplayStatics.h"   // OpenLevel 함수를 사용하기 위해 필요
#include "Kismet/KismetSystemLibrary.h" // QuitGame 함수를 사용하기 위해 필요
#include "GameFramework/PlayerController.h" // GetOwningPlayerController()가 반환하는 APlayerController를 알기 위해 필요
#include "Blueprint/UserWidget.h" // <-- CreateWidget 함수를 사용하기 위해 추가
#include "SettingsGameInstance.h"

/**
 * NativeConstruct: 위젯이 생성될 때 호출됩니다.
 */
void UMainMenuWidget::NativeConstruct()
{
    // 부모 클래스의 NativeConstruct를 먼저 호출합니다.
    Super::NativeConstruct();

    // BindWidget으로 연결된 버튼들이 유효한지 확인하고 클릭 이벤트를 C++ 함수에 연결합니다.
    if (PlayButton)
    {
        PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayClicked);
    }

    if (OptionButton)
    {
        OptionButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnOptionClicked);
    }

    if (ExitButton)
    {
        ExitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnExitClicked);
    }
}

/**
 * "Play" 버튼 기능
 */
void UMainMenuWidget::OnPlayClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Play Button Clicked! Setting up Loading Screen..."));

    // 1. 로딩 위젯 생성 및 표시
    if (LoadingScreenWidgetClass)
    {
        UUserWidget* LoadingWidget = CreateWidget<UUserWidget>(GetWorld(), LoadingScreenWidgetClass);
        if (LoadingWidget)
        {
            LoadingWidget->AddToViewport();
            UE_LOG(LogTemp, Warning, TEXT("Loading Screen displayed."));
        }
    }
    // ...

    // 2. 메인 메뉴 숨기기
    this->SetVisibility(ESlateVisibility::Hidden);

    // [수정된 부분] 로딩 화면을 띄울 때 입력 모드를 명시적으로 설정합니다.
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        FInputModeUIOnly InputMode;
        // InputMode.SetWidgetToFocus(...) // <-- 로딩 화면은 포커스가 필요 없으므로 호출 안 함
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(false); // 로딩 중 커서 숨김
    }
    // [수정 끝]

    // 4. 약간의 지연 후 레벨 로드 (로딩 스크린이 보이도록)
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UMainMenuWidget::LoadTargetLevel, 2.0f, false);
}

/**
 * "Option" 버튼 기능
 */
void UMainMenuWidget::OnOptionClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Option Button Clicked!"));

    if (OptionsWidgetClass)
    {
        UUserWidget* OptionsWidget = CreateWidget<UUserWidget>(GetWorld(), OptionsWidgetClass);
        if (OptionsWidget)
        {
            FScriptDelegate BackDelegate;
            BackDelegate.BindUFunction(this, FName("OnBackFromOptions"));

            FProperty* DispatcherProperty = OptionsWidget->GetClass()->FindPropertyByName(FName("OnBackClicked"));
            if (FMulticastDelegateProperty* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(DispatcherProperty))
            {
                MulticastDelegateProperty->AddDelegate(BackDelegate, OptionsWidget);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("WBP_Options에 'OnBackClicked' Event Dispatcher None!"));
            }

            OptionsWidget->AddToViewport();
            this->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("OptionsWidgetClass is not set..."));
    }
}

/**
 * "Exit" 버튼 기능
 */
void UMainMenuWidget::OnExitClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Exit Button Clicked! Quitting Game..."));

    // [수정된 부분]
    // GetOwningPlayerController() 대신 UGameplayStatics::GetPlayerController를 사용합니다.
    // 'this'는 이 위젯이 속한 월드(UObject*)를 의미하고, '0'은 0번 플레이어를 의미합니다.
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

    // PlayerController가 유효한지 확인하는 것은 동일하게 중요합니다.
    if (PlayerController)
    {
        UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, true);
    }
    else
    {
        // 혹시 모르니 컨트롤러를 못 찾았을 때의 로그를 추가합니다.
        UE_LOG(LogTemp, Error, TEXT("OnExitClicked: Failed to get PlayerController!"));
    }
}

void UMainMenuWidget::OnBackFromOptions()
{
    UE_LOG(LogTemp, Warning, TEXT("Back from Options received. Showing Main Menu."));

    // [추가된 부분]
    // 1. SettingsGameInstance를 가져옵니다.
    USettingsGameInstance* SettingsGI = Cast<USettingsGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    // 2. 인스턴스가 유효하면 SaveSettings()를 호출합니다.
    if (SettingsGI)
    {
        SettingsGI->SaveSettings();
    }
    // [추가된 부분 끝]

    // WBP_Options는 스스로 'Remove from Parent'를 호출했으므로,
    // 우리는 이 메인 메뉴 위젯을 다시 '보이게'만 하면 됩니다.
    this->SetVisibility(ESlateVisibility::Visible);
}

void UMainMenuWidget::LoadTargetLevel()
{
    // 기존 OnPlayClicked에 있던 레벨 로드 로직
    UGameplayStatics::OpenLevel(this, FName("LowerSector_Mod"));
}
