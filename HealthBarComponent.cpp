#include "HealthBarComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Components/ProgressBar.h" 
#include "Blueprint/UserWidget.h"   

UHealthBarComponent::UHealthBarComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen); // 항상 스크린 공간에서 카메라를 바라보도록 설정
	SetVisibility(false); // 기본적으로 숨김
	SetRelativeLocation(FVector(0.f, 0.f, 100.f)); // 머리 에서 얼마나 위에서 표시 될지 Bp에서 수정가능
}

void UHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 매번 Cast하는 비용을 줄이기 위해 시작 시 캐싱
	HealthInterface = TScriptInterface<IHealthInterface>(Owner);

	if (HealthInterface)
	{
		// 데미지 전달받기 위해 델리게이트 바인딩
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthBarComponent::OnOwnerDamaged);

		TWeakObjectPtr<UHealthBarComponent> WeakThis(this); // 약참조 선언
		GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis]() // 다음 틱에
			{
				if (WeakThis.IsValid()) // 유효성 검사
				{
					WeakThis->UpdateHealthBar(); // 체력바 갱신
				}
			});
	}
}

void UHealthBarComponent::OnOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	UWorld* World = GetWorld();
	if (!World || !HealthInterface) return;

	// 사망했을때와 사망하지 않았을떄의 UI 연출시간을 다르게 함
	// Execute_IsEnemyDead로 사망을 BP로 처리했건 C++로 처리했건 상관없이 IsEnemyDead 함수 실행
	// Execute는 정적 함수이므로 매개변수로 이 함수를 실행할 HealthInterface를 가진 적 엑터를 반환
	bool bIsDead = HealthInterface->Execute_IsEnemyDead(HealthInterface.GetObject());

	// 체력바 갱신
	UpdateHealthBar();

	if (bIsDead) // 사망했을 경우
	{
		// 사망 시에는 짧게 보여준 후 제거 
		World->GetTimerManager().ClearTimer(HealthBarVisibleTimerHandle); // 타이머 클리어
		World->GetTimerManager().SetTimer(HealthBarVisibleTimerHandle, this, &UHealthBarComponent::HideHealthBar, HealthBarFadeTime, false); // 짧은 시간 보여줌
		return;
	}

	if (Damage <= 0.0f) return; // 데미지를 입지 않았더면 리턴

	// 데미지를 입으면 표시하고 타이머 갱신
	SetVisibility(true); // 표시
	World->GetTimerManager().ClearTimer(HealthBarVisibleTimerHandle); // 타이머 갱신

	TWeakObjectPtr<UHealthBarComponent> WeakThis(this); // 약참조 선언
	GetWorld()->GetTimerManager().SetTimer(HealthBarVisibleTimerHandle, [WeakThis]()
		{
			if (WeakThis.IsValid()) // 유효성 검사
			{
				WeakThis->HideHealthBar(); // 체력바 숨김
			}
		}, HealthBarVisibilityTime, false); // 체력바 보이는 시간 후에
}

void UHealthBarComponent::UpdateHealthBar()
{
	UUserWidget* HealthWidget = GetUserWidgetObject();
	if (!HealthWidget || !HealthInterface) return;

	if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar")))) // 체력바를 찾고
	{
		float HealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject()); // 체력 퍼센트를 가져와서 
		HealthProgressBar->SetPercent(HealthPercent); // 현재 인터페이스 값으로 설정
	}
}

void UHealthBarComponent::HideHealthBar()
{
	SetVisibility(false);
}

void UHealthBarComponent::InitWidget()
{
	Super::InitWidget(); 
	// 위젯 생성 즉시
	UpdateHealthBar(); // 헬스바 동기화
}
//void UHealthBarComponent::BeginPlay()
//{
//	Super::BeginPlay();
//
//	if (AActor* Owner = GetOwner())
//	{
//		HealthInterface = TScriptInterface<IHealthInterface>(Owner);
//
//		// TScriptInterface는 bool 연산자 사용
//		if (HealthInterface)
//		{
//			// OnTakeAnyDamage 이벤트 바인딩
//			Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthBarComponent::OnOwnerDamaged);
//
//			// 초기 체력바 설정 (위젯이 준비된 후)
//			GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
//				{
//					if (UUserWidget* HealthWidget = GetUserWidgetObject())
//					{
//						float InitialHealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());
//
//						if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
//						{
//							HealthProgressBar->SetPercent(InitialHealthPercent);
//							UE_LOG(LogTemp, Warning, TEXT("Health Bar initialized in BeginPlay to: %f"), InitialHealthPercent);
//						}
//					}
//				});
//		}
//		else
//		{
//			UE_LOG(LogTemp, Error, TEXT("Owner does not implement IHealthInterface!"));
//		}
//	}
//	// 처음에는 숨김
//	SetVisibility(false);
//}
//
//void UHealthBarComponent::OnOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
//{
//	UE_LOG(LogTemp, Warning, TEXT("OnOwnerDamaged called: Actor=%s, Damage=%f"),
//		*DamagedActor->GetName(), Damage);
//
//	// TScriptInterface는 nullptr 비교 또는 ! 연산자 사용
//	if (!HealthInterface)
//	{
//		UE_LOG(LogTemp, Error, TEXT("HealthInterface is NULL!"));
//		return;
//	}
//
//	// 사망 체크 - 사망했으면 체력바를 0으로 설정 후 숨김
//	if (HealthInterface->Execute_IsEnemyDead(HealthInterface.GetObject()))
//	{
//		// **핵심 수정**: 사망 시 체력바를 완전히 비우기
//		if (UUserWidget* HealthWidget = GetUserWidgetObject())
//		{
//			if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
//			{
//				HealthProgressBar->SetPercent(0.0f);  // 완전히 비우기
//				UE_LOG(LogTemp, Warning, TEXT("Enemy died - Progress Bar set to 0.0"));
//			}
//		}
//
//		// 잠깐 보여준 후 숨김 (사망 효과를 위해)
//		GetWorld()->GetTimerManager().SetTimer(
//			HealthBarVisibleTimerHandle,
//			this,
//			&UHealthBarComponent::HideHealthBar,
//			0.5f,  // 0.5초 후 숨김
//			false);
//		return;
//	}
//
//	// 2. 0 데미지(예: 상태이상)는 무시합니다.
//	if (Damage <= 0.0f) return;
//
//	// 3. 체력 바를 즉시 보이게 합니다.
//	SetVisibility(true);
//
//	// Progress Bar 직접 업데이트
//	if (UUserWidget* HealthWidget = GetUserWidgetObject())
//	{
//		// 체력 퍼센티지 계산
//		float HealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());
//		UE_LOG(LogTemp, Warning, TEXT("Calculated Health Percent: %f"), HealthPercent);
//
//		// Progress Bar 찾기 및 직접 업데이트
//		if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
//		{
//			HealthProgressBar->SetPercent(HealthPercent);
//			UE_LOG(LogTemp, Warning, TEXT("Progress Bar updated directly to: %f"), HealthPercent);
//		}
//		else
//		{
//			UE_LOG(LogTemp, Error, TEXT("HealthProgressBar not found in widget!"));
//		}
//	}
//	else
//	{
//		UE_LOG(LogTemp, Error, TEXT("Widget is NULL!"));
//	}
//
//	// 4. 기존 숨김 타이머를 취소하고, 새 타이머를 시작합니다.
//	GetWorld()->GetTimerManager().ClearTimer(HealthBarVisibleTimerHandle);
//	GetWorld()->GetTimerManager().SetTimer(
//		HealthBarVisibleTimerHandle,
//		this,
//		&UHealthBarComponent::HideHealthBar,
//		HealthBarVisibilityTime,
//		false);
//
//	// Progress Bar 직접 업데이트
//	if (UUserWidget* HealthWidget = GetUserWidgetObject())
//	{
//		float HealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());
//
//		// Progress Bar 찾기 및 업데이트
//		if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
//		{
//			HealthProgressBar->SetPercent(HealthPercent);
//		}
//	}
//}
//
//void UHealthBarComponent::HideHealthBar()
//{
//	SetVisibility(false);
//}
//
//void UHealthBarComponent::InitWidget()
//{
//	Super::InitWidget();
//
//	// 위젯이 생성되면 즉시 올바른 체력 값으로 초기화
//	if (UUserWidget* HealthWidget = GetUserWidgetObject())
//	{
//		// TScriptInterface는 bool 연산자 사용 (IsValid() 아님)
//		if (HealthInterface)
//		{
//			float InitialHealthPercent = HealthInterface->Execute_GetHealthPercent(HealthInterface.GetObject());
//
//			if (UProgressBar* HealthProgressBar = Cast<UProgressBar>(HealthWidget->GetWidgetFromName(TEXT("HealthProgressBar"))))
//			{
//				HealthProgressBar->SetPercent(InitialHealthPercent);
//				UE_LOG(LogTemp, Warning, TEXT("Health Bar initialized to: %f"), InitialHealthPercent);
//			}
//		}
//	}
//}
