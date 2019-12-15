#include <Arduino.h>
extern "C" int RTC_Init(void);
extern "C" void RTC_deint();
extern "C" struct tm getstime();
extern "C" int setstime(int year, int mon, int day, int hour, int min, int sec);
extern "C" int setsecint(void (*callback)());
extern "C" void RTC_WKUP_IRQHandler(void);
extern "C" int check_backup();
extern "C" int read_backup();
extern "C" void set_backup();
extern "C" void dateTime(uint16_t* date, uint16_t* time);
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc);

