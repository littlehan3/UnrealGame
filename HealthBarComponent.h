#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "HealthInterface.h" 
#include "HealthBarComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCOMOTION_API UHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UHealthBarComponent();

protected:
	virtual void BeginPlay() override;
	virtual void InitWidget() override;

	// 피해를 입은 뒤 체력바 노출 지속시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HealthBar")
	float HealthBarVisibilityTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HealthBar")
	float HealthBarFadeTime = 0.5f;

	// 체력바를 다시 숨기기 위한 타이머핸들
	FTimerHandle HealthBarVisibleTimerHandle;

	// 오너인 적이 피해를 입었을 때 호출될 함수
	UFUNCTION()
	void OnOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	//체력 바를 숨기는 함수
	UFUNCTION()
	void HideHealthBar();

	void UpdateHealthBar();
	// 이 컴포넌트의 오너인 적이 IHealthInterface를 가졌는지 캐시
	UPROPERTY()
	TScriptInterface<IHealthInterface> HealthInterface;
};