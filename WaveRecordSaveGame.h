#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WaveRecordSaveGame.generated.h"

UCLASS()
class LOCOMOTION_API UWaveRecordSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    // 인스턴스 생성을 쉽게 하기 위한 스태틱 변수
    static FString SaveSlotName;
    static const int32 UserIndex;

public:
    UWaveRecordSaveGame();

    // 저장할 최고 기록 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Best Record")
    int32 BestClearedWaveIndex = -1; // 클리어한 웨이브의 인덱스 (0부터 시작, -1은 기록 없음)

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Best Record")
    FString BestClearedWaveName = TEXT(""); // 클리어한 웨이브의 이름

};
