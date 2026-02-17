#include "WaveRecordSaveGame.h"
#include "Kismet/GameplayStatics.h"

// 스태틱 변수 초기화
FString UWaveRecordSaveGame::SaveSlotName = TEXT("BestWaveRecordSlot"); // 정적 변수 초기화
const int32 UWaveRecordSaveGame::UserIndex = 0; // 정적 상수 초기화 단일 사용자 이므로 0

UWaveRecordSaveGame::UWaveRecordSaveGame()
{
    BestClearedWaveIndex = -1; // 클리어 인덱스의 초기값을 -1로 설정하여 아무 기록이 없게함
    BestClearedWaveName = TEXT(""); // 웨이브 이름의 초기값을 빈 텍스트로 설정
}

UWaveRecordSaveGame* UWaveRecordSaveGame::LoadOrCreate()
{
    // 기존 세이브 파일이 있는지 검사
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
    {
        // 파일이 있다면 해당 슬롯에서 데이터를 읽어와 UWaveRecordSaveGame 타입으로 캐스팅하여 반환
        return Cast<UWaveRecordSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));
    }
    // 없으면 새로운 세이브 객체를 메모리에 생성하여 반환
    return Cast<UWaveRecordSaveGame>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
}
