#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;

UCLASS()
class LOCOMOTION_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

protected:
	virtual void NativeConstruct() override;

    /**
     * 메인 메뉴(WBP_MainMenu) 블루프린트에서
     * 우리가 띄울 '옵션 위젯' 블루프린트(WBP_Options)를
     * 지정할 수 있도록 변수를 노출시킵니다.
     */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> OptionsWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

private:
    /**
     * WBP 디자이너에서 만든 위젯과 C++ 변수를 연결합니다.
     * * [매우 중요]
     * 'meta = (BindWidget)'은 WBP의 디자이너 탭에서
     * 'Variable Name'이 "PlayButton"인 위젯을 찾아
     * 이 C++ 변수에 자동으로 할당하라는 뜻입니다.
     */
    UPROPERTY(meta = (BindWidget))
    UButton* PlayButton;

    UPROPERTY(meta = (BindWidget))
    UButton* OptionButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ExitButton;

    /** 버튼 클릭 시 호출될 실제 C++ 함수들 */
    // 델리게이트(이벤트)에 연결하려면 UFUNCTION() 매크로가 필수입니다.
    UFUNCTION()
    void OnPlayClicked();

    UFUNCTION()
    void OnOptionClicked();

    UFUNCTION()
    void OnExitClicked();

    UFUNCTION()
    void OnBackFromOptions();
	
    // [추가] 실제 레벨 로드를 담당할 프라이빗 함수
    void LoadTargetLevel();
};
