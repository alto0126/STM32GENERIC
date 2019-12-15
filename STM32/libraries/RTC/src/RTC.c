/* RTCライブラリ */
#include "stm32_def.h"
#include <time.h>
//#include "FatStructs.h"
RTC_HandleTypeDef hrtc;

void (*fcp)() = 0;

/* RTC初期設定 */
int RTC_Init(void){
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)  {
    return !HAL_OK;
  }
  return HAL_OK;
}

/* RTC割込み設定 */
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc){
  if(hrtc->Instance==RTC){
    __HAL_RCC_RTC_ENABLE();
    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(hrtc, RTC_FLAG_WUTF);
  }
}

/* RTC割込み解除 */
void RTC_deint(){
  if(hrtc.Instance==RTC){
    HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
  }
}

/* 曜日計算　*/
int week(int y, int m, int d){
    if(m == 1 || m == 2 ){
        y--;
        m += 12;
    }
    return (y+y/4-y/100+y/400+(13*m+8)/5+d)%7;
}

/* RTC読出し */
struct tm getstime(){
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
  struct tm stime;
  
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
  
  stime.tm_sec = time.Seconds;
  stime.tm_min = time.Minutes;
  stime.tm_hour = time.Hours;
  stime.tm_year = date.Year + 100;
  stime.tm_mon = date.Month - 1;
  stime.tm_mday = date.Date - 1;
  stime.tm_wday = date.WeekDay;
  if(date.WeekDay == 7) stime.tm_wday = 0;
  return stime;
}

/* RTC時刻設定 */
int setstime(int year, int mon, int day, int hour, int min, int sec){
  RTC_DateTypeDef sDate = {0};
  RTC_TimeTypeDef sTime = {0};
  
  sTime.Hours = hour;
  sTime.Minutes = min;
  sTime.Seconds = sec;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK){
        return !HAL_OK;
  }
  sDate.WeekDay = week(year, mon, day);
  sDate.Month = mon;
  sDate.Date = day;
  sDate.Year = year - 2000;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK){
    return !HAL_OK;
  }
  return HAL_OK;
}

/* RTC_BKP_DR0をチェック */
int check_backup(){
    if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 0xdbd9){
        return 1;
    }
    return 0;
}

/* RTC_BKP_DR0を読出し*/
int read_backup(){
    return HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
}

/* RTC_BKP_DR0に書き込み　*/
void set_backup(){
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0xdbd9);
}

/* RTC　１S割込みハンドラ */
void RTC_WKUP_IRQHandler(void){
  HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}

/* RTC 1秒割込みコールバック処理 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc){
    fcp();
}

/* 1秒割込みコールバックルーチン登録 */
int setsecint(void (*callback)()){
    fcp = callback;
    HAL_RTC_MspInit(&hrtc);
    /** Enable the WakeUp */
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK){
        return !HAL_OK;
    }
    return HAL_OK;
}

void dateTime(uint16_t* date, uint16_t* time){
  struct tm stime;

  stime = getstime();
  int16_t year = 1900+stime.tm_year;
  int8_t month = stime.tm_mon+1;
  int8_t day = stime.tm_mday+1;
  int8_t hour = stime.tm_hour;
  int8_t minute = stime.tm_min;
  int8_t second = stime.tm_sec;
  *date = (year - 1980) << 9 | month << 5 | day;
  *time = hour << 11 | minute << 5 | second >> 1;
}

