/***********************************************************************/
/*                                                                     */
/*  FILE        :net_clock.ino                                                */
/*  DATE        :2019/10/16                                              */
/*  DESCRIPTION :NTP Clock sample PROGRAM                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/

#include "RTC.h"
#include <time.h>

String atout(String data);
void initWiFi();
void setRTSpin();

void setup() {
  delay(1000);
  /* NTP時計 */
  String month = "JanFebMarAprMayJunJulAugSepOctNovDec"; 
   
  RTC_Init();                   //RTC初期化
  if(!check_backup()){          //RTC時刻データ有効フラグのチェック(0xdbd9）
    setRTSpin();                //RTS機能選択
    initWiFi();                 //WiFi初期化
    atout("AT+CWMODE=1");       //WiFiをStationモードに設定
    atout("AT+CIPSNTPCFG=1,9,\"pool.ntp.org\""); //NTPタイムゾーン設定
    set_backup();               //RTC時刻データ有効フラグ書き込み（0xdbd9)
    delay(2000);  
    String strtime = atout("AT+CIPSNTPTIME?");   //時刻の読出し
    strtime = strtime.substring(strtime.indexOf("+CIPSNTPTIME:"),100);
    int y = strtime.substring(33,37).toInt();
    int d = strtime.substring(21,23).toInt();
    int h = strtime.substring(24,26).toInt();
    int m = strtime.substring(27,29).toInt();
    int s = strtime.substring(30,32).toInt();
    int mo = month.indexOf(strtime.substring(17,20))/3 + 1;
    //Serial.printf("%d %d %d %d %d %d\n",y,mo,d,h,m,s);
    setstime(y,mo,d,h,m,s);
  }
  setsecint(clockcount);
  while(!Serial.available());
  RTC_deint();
}

void loop() {

}

/* NTP時計1秒割込みコールバック */
void clockcount(){
  struct tm stime;
  char day[][4] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
  char str[32];
  
  stime = getstime();
  Serial.printf("%4d/%02d/%02d(%s) %02d:%02d:%02d\n", 1900+stime.tm_year,stime.tm_mon+1,stime.tm_mday+1,day[stime.tm_wday],stime.tm_hour,stime.tm_min,stime.tm_sec);
}

/* AT命令出力 */
String atout(String data) {
  String resp = "";
  String res;

  SerialUART2.println(data);  //UART2からAT命令出力
  while (1) {
    while (!SerialUART2.available()); //レスポンス待ち
    res = SerialUART2.readStringUntil(10);
    resp += res;
    if(res.indexOf("OK") != -1) return resp;  //OKレスポンス
    if(res.indexOf("ERROR") != -1) return ""; //ERRORレスポンス
  }  
}

void setRTSpin(){
  GPIO_InitTypeDef GPIO_InitStruct;
  
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  
}

void initWiFi(){
    pinMode(PB1, OUTPUT);
    digitalWrite(PB1, LOW);
    delay(100);
    digitalWrite(PB1, HIGH);
    for(int n = 0; n < 5; n++){
       Serial.print("*");
       delay(1000);
    }
    Serial.println();
    Serial2.begin(115200);
    delay(100);
    while(Serial2.available()){
      Serial2.read();
    }
    String ssid = atout("AT+CWJAP_CUR?");
    String sid = ssid.substring(ssid.indexOf("CWJAP_CUR:")+10, ssid.indexOf(','));
    Serial.println("LAN SSID:" + sid);
    atout("AT+UART_CUR=921600,8,1,0,2");
    Serial2.begin(921600, 1); 
    delay(100);
}
