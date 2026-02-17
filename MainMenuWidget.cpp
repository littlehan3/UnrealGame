#include "MainMenuWidget.h" 
#include "Components/Button.h"        // UButton의 OnClicked 이벤트를 사용하기 위해 
#include "Kismet/GameplayStatics.h"   // OpenLevel 함수를 사용하기 위해
#include "Kismet/KismetSystemLibrary.h" // QuitGame 함수를 사용하기 위해
#include "GameFramework/PlayerController.h" // GetOwningPlayerController()가 반환하는 APlayerController를 알기 위해
#include "Blueprint/UserWidget.h" // CreateWidget 함수를 사용하기 위해
#include "SettingsGameInstance.h" 
#include "TimerManager.h"


// NativeConstruct: 위젯이 생성될 때 호출
void UMainMenuWidget::NativeConstruct()
{
    // 부모 클래스의 NativeConstruct를 먼저 호출
    Super::NativeConstruct();

    // BindWidget으로 연결된 버튼들이 유효한지 확인하고 클릭 이벤트를 C++ 함수에 연결
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

void UMainMenuWidget::OnPlayClicked()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. 로딩 위젯 생성 및 표시
    if (IsValid(LoadingScreenWidgetClass))
    {
        UUserWidget* LoadingWidget = CreateWidget<UUserWidget>(World, LoadingScreenWidgetClass);
        if (IsValid(LoadingWidget))
        {
            LoadingWidget->AddToViewport(); // 뷰포트에 추가
        }
    }
    // 2. 메인 메뉴 숨김
    this->SetVisibility(ESlateVisibility::Hidden);

    // 3. 입력모드 설정
    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (IsValid(PC))
    {
        FInputModeUIOnly InputMode; // 키보드 마우스 입력 잠금
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways); // 마우스 커서 게임 창 밖으로 나가지 않게 잠금 (듀얼모니터)
        PC->SetInputMode(InputMode); // 입력 목적지를 게임 월드가 아닌 UI로 고정 (UI 우선권 부여)
        PC->SetShowMouseCursor(false); // 로딩 중 커서 숨김
    }
    // 게임이 불러오는 중인 걸 시각적으로 보여주기 위해 지연 타이머 설정
    TWeakObjectPtr<UMainMenuWidget> WeakThis(this); // 약참조 선언
    FTimerHandle TimerHandle; 
    World->GetTimerManager().SetTimer(TimerHandle, [WeakThis]()
        {
            if (WeakThis.IsValid()) // 유효성 검사
            {
                WeakThis->LoadTargetLevel(); // 레벨 로드
            }
        }, LoadingDisplayTime, false); // 로딩화면 재생시간만큼, 단발성
}

void UMainMenuWidget::OnOptionClicked()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!OptionsWidgetClass) return;

    // 옵션 위젯을 생성하고 리플렉션을 통해 뒤로가기 이벤트를 바인딩
    if (UUserWidget* OptionsWidget = CreateWidget<UUserWidget>(GetWorld(), OptionsWidgetClass))
    {
        // 콜백 함수 바인딩
        FScriptDelegate BackDelegate;
        BackDelegate.BindUFunction(this, FName("OnBackFromOptions")); // 문자열로 함수를 찾음

        // BP에서 정의된 OnBackClicked 이벤트 디스패처를 찾음
        FProperty* DispatcherProperty = OptionsWidget->GetClass()->FindPropertyByName(FName("OnBackClicked"));
        // 찾은 것이 이벤트 디스패처 형식이 맞는지 확인 (아니라면 nullptr 반환)
        if (FMulticastDelegateProperty* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(DispatcherProperty))
        {
            MulticastDelegateProperty->AddDelegate(BackDelegate, OptionsWidget); // BP에서 OnBackClicked 호출 시 c++ 함수 OnBackedFromOptions 실행
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("WBP_Options 'OnBackClicked' Event Dispatcher None!"));
        }
        OptionsWidget->AddToViewport(); // 뷰포트에 옵션 위젯 표시
        this->SetVisibility(ESlateVisibility::Hidden); // 메인메뉴 숨김
    }
}

void UMainMenuWidget::OnExitClicked()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 플레이어 컨트롤러 유효성 검사 후
    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (IsValid(PC))
    {
        UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true); // 게임 종료
    }
}

void UMainMenuWidget::OnBackFromOptions()
{
    // 옵션창을 닫을때 GameInstance를 통해 현재 설정을 디스크에 저장
    USettingsGameInstance* SettingsGI = Cast<USettingsGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (IsValid(SettingsGI)) // 유효성 검사
    {
        SettingsGI->SaveSettings(); // 세이브 세팅스 함수 호출
    }
    this->SetVisibility(ESlateVisibility::Visible); // 메인메뉴 활성화
}

void UMainMenuWidget::LoadTargetLevel()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (World) // 유효성 검사 후
    {
        UGameplayStatics::OpenLevel(this, FName("LowerSector_Mod")); // 레벨 로드
    }
}
