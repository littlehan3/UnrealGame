#include "HealthBarComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/ProgressBar.h"  // 추가
#include "Blueprint/UserWidget.h"    // 추가

UHealthBarComponent::UHealthBarComponent()
{
	// 기본 설정: 항상 스크린 공간에서 카메라를 바라보도록 설정
	SetWidgetSpace(EWidgetSpace::Screen);
	SetVisibility(false); // 기본적으로 숨김
	SetRelativeLocation(FVector(0.f, 0.f, 100.f)); // 머리 위 100cm (BP에서 조절 가능)
}

void UHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	// Owner 설정
	if (AActor* Owner = GetOwner())
	{
		HealthInterface = TScriptInterface<IHealthInterface>(Owner);

		// TScriptInterface는 bool 연산자 사용
		if (HealthInterface)
		{
			// OnTakeAnyDamage 이벤트 바인딩
			Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthBarComponent::OnOwnerDamaged);

			// 초기 체력바 설정 (위젯이 준비된 후)
			GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
				{
					if (UUserWidget* HealthWidget = GetUserWidgetObject())
					{
						float InitialHealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());

						if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
						{
							HealthProgressBar->SetPercent(InitialHealthPercent);
							UE_LOG(LogTemp, Warning, TEXT("Health Bar initialized in BeginPlay to: %f"), InitialHealthPercent);
						}
					}
				});
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Owner does not implement IHealthInterface!"));
		}
	}
	// 처음에는 숨김
	SetVisibility(false);
}

void UHealthBarComponent::OnOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("OnOwnerDamaged called: Actor=%s, Damage=%f"),
		*DamagedActor->GetName(), Damage);

	// TScriptInterface는 nullptr 비교 또는 ! 연산자 사용
	if (!HealthInterface)
	{
		UE_LOG(LogTemp, Error, TEXT("HealthInterface is NULL!"));
		return;
	}

	// 사망 체크 - 사망했으면 체력바를 0으로 설정 후 숨김
	if (HealthInterface->Execute_IsEnemyDead(HealthInterface.GetObject()))
	{
		// **핵심 수정**: 사망 시 체력바를 완전히 비우기
		if (UUserWidget* HealthWidget = GetUserWidgetObject())
		{
			if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
			{
				HealthProgressBar->SetPercent(0.0f);  // 완전히 비우기
				UE_LOG(LogTemp, Warning, TEXT("Enemy died - Progress Bar set to 0.0"));
			}
		}

		// 잠깐 보여준 후 숨김 (사망 효과를 위해)
		GetWorld()->GetTimerManager().SetTimer(
			HealthBarVisibleTimerHandle,
			this,
			&UHealthBarComponent::HideHealthBar,
			0.5f,  // 0.5초 후 숨김
			false);
		return;
	}

	// 2. 0 데미지(예: 상태이상)는 무시합니다.
	if (Damage <= 0.0f) return;

	// 3. 체력 바를 즉시 보이게 합니다.
	SetVisibility(true);

	// Progress Bar 직접 업데이트
	if (UUserWidget* HealthWidget = GetUserWidgetObject())
	{
		// 체력 퍼센티지 계산
		float HealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());
		UE_LOG(LogTemp, Warning, TEXT("Calculated Health Percent: %f"), HealthPercent);

		// Progress Bar 찾기 및 직접 업데이트
		if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
		{
			HealthProgressBar->SetPercent(HealthPercent);
			UE_LOG(LogTemp, Warning, TEXT("Progress Bar updated directly to: %f"), HealthPercent);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("HealthProgressBar not found in widget!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Widget is NULL!"));
	}

	// 4. 기존 숨김 타이머를 취소하고, 새 타이머를 시작합니다.
	GetWorld()->GetTimerManager().ClearTimer(HealthBarVisibleTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		HealthBarVisibleTimerHandle,
		this,
		&UHealthBarComponent::HideHealthBar,
		HealthBarVisibilityTime,
		false);

	// Progress Bar 직접 업데이트
	if (UUserWidget* HealthWidget = GetUserWidgetObject())
	{
		float HealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());

		// Progress Bar 찾기 및 업데이트
		if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
		{
			HealthProgressBar->SetPercent(HealthPercent);
		}
	}
}

void UHealthBarComponent::HideHealthBar()
{
	SetVisibility(false);
}

void UHealthBarComponent::InitWidget()
{
	Super::InitWidget();

	// 위젯이 생성되면 즉시 올바른 체력 값으로 초기화
	if (UUserWidget* HealthWidget = GetUserWidgetObject())
	{
		// TScriptInterface는 bool 연산자 사용 (IsValid() 아님)
		if (HealthInterface)
		{
			float InitialHealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());

			if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
			{
				HealthProgressBar->SetPercent(InitialHealthPercent);
				UE_LOG(LogTemp, Warning, TEXT("Health Bar initialized to: %f"), InitialHealthPercent);
			}
		}
	}
}
