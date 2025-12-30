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

	/** 피해를 입은 후 체력 바가 표시될 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HealthBar")
	float HealthBarVisibilityTime = 3.0f;

	/** 체력 바를 다시 숨기기 위한 타이머 핸들 */
	FTimerHandle HealthBarVisibleTimerHandle;

	/** 오너(적)가 피해를 입었을 때 호출될 함수 */
	UFUNCTION()
	void OnOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	/** 체력 바를 숨기는 함수 */
	UFUNCTION()
	void HideHealthBar();

	/** 이 컴포넌트의 오너(적)가 IHealthInterface를 가졌는지 캐시합니다. */
	UPROPERTY()
	TScriptInterface<IHealthInterface> HealthInterface;

	virtual void InitWidget() override;
};