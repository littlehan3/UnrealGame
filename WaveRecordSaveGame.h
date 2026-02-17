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
    static FString SaveSlotName; // 세이브 슬롯의 이름을 고정 시키기 위한 정적변수
    static const int32 UserIndex; // 로컬 사용자 인덱스를 정의하기 위한 정적 상수
    static UWaveRecordSaveGame* LoadOrCreate(); // 데이터를 로드하거나 파일이 없다면 새로 생성하는 정적함수

    UWaveRecordSaveGame();

    // 저장할 최고 기록 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Best Record")
    int32 BestClearedWaveIndex = -1; // 클리어한 웨이브의 인덱스 (0부터 시작, -1은 기록 없음)

    // 저장될 최고 기록의 이름 데이터
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Best Record")
    FString BestClearedWaveName = TEXT(""); // 클리어한 웨이브의 이름

};
