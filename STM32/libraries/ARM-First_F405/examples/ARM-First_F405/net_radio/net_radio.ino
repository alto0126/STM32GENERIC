/***********************************************************************/
/*                                                                     */
/*  FILE        :net_radio.ino                                                */
/*  DATE        :2019/10/16                                              */
/*  DESCRIPTION :Internet Radio sample PROGRAM                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/
#define CODEC_ADDR 31 // DAC i2c slave address
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_MAX_SAMPLES_PER_FRAME 1152*2

#include <Wire.h>   //I2Cライブラリのヘッダファイル
#include "I2S.h"    //I2Sライブラリのヘッダファイル
#include "SdFat.h"  //MicroSDのヘッダファイル
#include "minimp3.h"

void volume(int num);
void codec_reg_setup();
void setRTSpin();  
String atout(String data);

unsigned int rp = 0, wp = 0, quant = 0;
I2SClass I2S(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

void setup() {
  delay(1000);
  Serial.begin(115200);
  Wire.begin(); // start the I2C driver for codec register setup 
  I2S.begin(I2S_PHILIPS_MODE, 44100, 32);
}

void loop() {
/* インターネットラジオ*/
  int comm;
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
  mp3dec_frame_info_t info;
  mp3dec_t mp3d;
  int samples = 1;
  mp3dec_init(&mp3d);
  int frame = 1536;
  uint8_t inbuf[0x600];
  int16_t buf[8192*2];
  uint8_t uartbuf[8192*3];  //UARTリングバッファ
 
  struct url {              //ラジオ局URLリスト
    String host;
    String file;
    uint16_t port;
  }urllist[] = {
    {"wbgo.streamguys.net","/thejazzstream",80},
    {"wbgo.streamguys.net","/wbgo128",80},
    {"ice1.somafm.com","/defcon-128-mp3",80},
    {"ice1.somafm.com","/folkfwd-128-mp3",80},
    {"ice1.somafm.com","/illstreet-128-mp3",80},
    {"ice1.somafm.com","/secretagent-128-mp3",80},
    {"ice1.somafm.com","/bootliquor-128-mp3",80},
    {"tokyofmworld.leanstream.co","/JOAUFM-MP3",80},
  };

  Serial.begin(115200);
  codec_reg_setup();            //DAC初期化
  setRTSpin();                  //RTS機能選択
  I2S.setBuffer(buf, 8192*2);   //I2Sバッファ初期化 
  I2S.setsample(44100);         //I2Sサンプリング周波数設定
  Serial.println("ラジオメニュー");
  Serial.println("++++++++++++++++++++++++");
  Serial.println("音量(u:大きく d:小さく)");
  Serial.println("選局(n:次局   p:前局)");
  Serial.println("終了(e:終了)");
  Serial.println("++++++++++++++++++++++++");
  
  for(int num = 0;num < 8; num++){
    initWiFi();                 //WiFi初期化
    Serial.println("Site:" + urllist[num].host + urllist[num].file);
    atout("AT+CWMODE=1");       //WiFiをStationモードに設定
    //TCPクライアントとして相手先のドメイン＋Portを指定してコネクションを要求
    if(atout("AT+CIPSTART=\"TCP\",\""+urllist[num].host+"\","+urllist[num].port) == "") return;
    atout("AT+CIPMODE=1");      //WiFi、UART間を透過モードに設定
    String str = "GET "+urllist[num].file+" HTTP/1.1\r\n" + "Host: "+urllist[num].host+"\r\n\r\n";
    atout("AT+CIPSEND");        //透過モードの開始
    //Serial.println(str);
    Serial2.println(str);
    Serial.print(">");
    //ラジオ音声データバッファリング：8192×3Byte
    Serial2.readBytes(uartbuf,8192*3);
    wp = 8192*3;
    rp = 0;
    while(1){
      if((quant = Serial2.rxbufferhalf()) > 4096){ //UART受信バッファが4096byte以上ならリングバッファに書き込む
        if(8192*3 - (wp - rp) > 1024){  
          Serial2.readBytes(uartbuf + (wp % (8192*3)), 1024);
          wp += 1024;                               //リングバッファの書き込みポインタを更新
          //Serial.printf("w_wp:%d rp:%d qt:%d ", wp, rp, quant);
        }
      }
      if(wp - rp > 0x600){                          //リングバッファのデータが0x600以上あれば読出し
        for(int n = 0; n < 0x600; n++){             //MP3デコーダ入力バッファにデータをコピー
           inbuf[n] = uartbuf[(rp + n) % (8192*3)];
        }
        samples = mp3dec_decode_frame(&mp3d, inbuf, 0x600, pcm, &info);//MP3デコード
        frame = info.frame_bytes;
        rp += frame;             //MP3デコード後のフレームバイト数分リングバッファの読出しポインタを更新
        uint32_t d;
        for (int n = 0; n < 1152 * 2; n++) {
          d = *(uint32_t *)&pcm[n] << 16;
          I2S.write(d >> 16);
          I2S.write(d);
        }
        //Serial.printf("r_p:%d qt:%d\n ", wp - rp, quant);
        //Serial.printf("frame:%d\n", frame); 
        //Serial.printf("Layer:%d\n", info.layer);
        //Serial.printf("Sample Rate:%dHz\n", info.hz);
        //Serial.printf("Bit rate:%dkbps\n", info.bitrate_kbps); 
      }
      if(Serial.available()){
        comm = Serial.read();
        Serial.printf("%c:", comm);
        volume(comm);                  
        if(comm == 'n'){
          break;
        }else if(comm == 'p'){
          num -= 2;
          break;
        }else if(comm == 'e'){
                 Serial2.print("+++");
                 while(1);
              }
              Serial.print(">");
        }          
    }
    Serial2.begin(921600, 0);   
    Serial2.print("+++");
    delay(1000);
  }
}

/* 文字検索　*/
int search(File file, String str){
  int n, m;  
  //Serial.println(str); 
  file.seek(0);
  for(n = 0, m = 0; m < str.length(); n++){
    if(str[m] == file.read()) m++;
  }
  return n;
}

/* 音量調整 */
void volume(int num){
  int vol;

  if(num == 'u') {      //音量UP
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol + 10));//左音量設定　+2.5dB
    codec_writeReg(0x07, 0x200 | (vol + 10));//右音量設定
    Serial.println(codec_readReg(0x06));     //設定値読出し
  }else if(num == 'd') {//音量Down
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol - 10));//左音量設定　-2.5dB
    codec_writeReg(0x07, 0x200 | (vol - 10));//右音量設定
    Serial.println(codec_readReg(0x06));     //設定値読出し
  }
}

uint8_t codec_writeReg(uint8_t reg, uint16_t data){
  uint8_t error;
  
  Wire.beginTransmission(CODEC_ADDR); //output Slave address
  Wire.write(reg);                    //output slave mem address
  Wire.write((uint8_t)(data >> 8));   //output data MSB
  Wire.write((uint8_t)(data & 0xff)); //output data LSB
  error = Wire.endTransmission();     //terminate transmission
  return error;
}

uint16_t codec_readReg(uint8_t reg){
  uint8_t error;
  uint16_t data;
  
  Wire.beginTransmission(CODEC_ADDR); //output Slave address
  Wire.write(reg);                    //output slave mem address
  Wire.endTransmission();             //terminate transmission
  Wire.requestFrom(CODEC_ADDR, 2);    //output Slave address + 2byte read
  delay(1);
  data = Wire.read();                 //read MSB
  data = data << 8 | Wire.read();     //read LSB
  error = Wire.endTransmission();
  return data;                        //data:word data
}

void codec_reg_setup(){
  //Serial.printf("%x\n", codec_readReg(0x00));
  codec_writeReg(0x02, 0x00);       //DAC Power Down
  //Serial.println(codec_readReg(0x02));
  codec_writeReg(0x02, 0x03);       //DAC Power UP
  //Serial.println(codec_readReg(0x02));
  codec_writeReg(0x03, 0x1a);       //32bit I2S Format
  codec_writeReg(0x06, 0x200 | 300);//Volume left=300
  //Serial.println(codec_readReg(0x06));
  codec_writeReg(0x07, 0x200 | 300);//Volume right=300
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
    Serial.println("LAN SSID:" + ssid.substring(ssid.indexOf("CWJAP_CUR:")+10, ssid.indexOf(',')));
    atout("AT+UART_CUR=921600,8,1,0,2");
    Serial2.begin(921600, 1); 
    delay(100);
}
