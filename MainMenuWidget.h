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

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> OptionsWidgetClass; // bp로 옵션 위젯클래스 등록

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> LoadingScreenWidgetClass; // bp로 로딩 위젯클래스 등록

private:
    // WBP 디자이너에서 만든 위젯과 C++ 변수를 연결
    // meta = (BindWidget)으로 WBP의 디자이너 탭에서 Variable Name이 "PlayButton"인 위젯을 찾아 이 C++ 변수에 자동으로 할당
    UPROPERTY(meta = (BindWidget, AllowPrivateAccess = "true"))
    UButton* PlayButton; // 플레이 버튼

    UPROPERTY(meta = (BindWidget, AllowPrivateAccess = "true"))
    UButton* OptionButton; // 옵션 버튼

    UPROPERTY(meta = (BindWidget, AllowPrivateAccess = "true"))
    UButton* ExitButton; // 나가기 버튼

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    float LoadingDisplayTime = 2.0f; // 로딩화면을 재생할 시간

    UFUNCTION()
    void OnPlayClicked(); // 플레이 버튼 이벤트 바인딩 함수

    UFUNCTION()
    void OnOptionClicked(); // 옵션 버튼 이벤트 바인딩 함수

    UFUNCTION()
    void OnExitClicked(); // 나가기 버튼 이벤트 바인딩 함수

    UFUNCTION()
    void OnBackFromOptions(); // 이벤트 디스패처 응답 함수
	
     UFUNCTION()
    void LoadTargetLevel(); // 실제 레벨 로드 함수
};
