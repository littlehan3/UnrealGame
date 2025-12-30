#include "WaveRecordSaveGame.h"

// 스태틱 변수 초기화
FString UWaveRecordSaveGame::SaveSlotName = TEXT("BestWaveRecordSlot");
const int32 UWaveRecordSaveGame::UserIndex = 0;

UWaveRecordSaveGame::UWaveRecordSaveGame()
{
    // 기본값 설정
    BestClearedWaveIndex = -1;
    BestClearedWaveName = TEXT("");
}
