/***********************************************************************/
/*                                                                     */
/*  FILE        :Arm_First_Test_lcd.c                                                */
/*  DATE        :2019/5/18                                              */
/*  DESCRIPTION :Arm-First DEMO PROGRAM                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/
#define CODEC_ADDR 31 // DAC i2c slave address
#define PRESS_ADR 93
#define ACC_ADR 107

#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
//#define MINIMP3_MAX_SAMPLES_PER_FRAME 1152*2
#define RECORDING_PERIOD  3000    /* xPCM_BUFF_PERIOD ms */
#define WAV_HEADER_SIZE   44

#include <Wire.h>   //I2Cライブラリのヘッダファイル
#include "I2S.h"    //I2Sライブラリのヘッダファイル
#include "SdFat.h"  //MicroSDのヘッダファイル
#include "math.h"
#include "minimp3.h"
#include "i2s_dma.h"
#include "args.h"
#include "audio_process.h"
#include "pdm2pcm.h"
#include <LiquidCrystal.h>
#include "RTC.h"
#include <time.h>

const int rs = PA4, en = PB11, d4 = PC13, d5 = PB8, d6 = PB12, d7 = PC7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
String lcddata[2] = {"",""};
unsigned int rp = 0, wp = 0, quant = 0;

void exfunc1(void (*fp)());
void exfunc2(void (*fp)(char), char mode);
uint8_t i2c_mem_write(uint8_t reg, uint8_t addr, uint8_t data);
uint8_t i2c_mem_read(uint8_t count, uint8_t addr, uint8_t reg, uint8_t * buf);
void press_calc();
void acc_calc();
void gyro_calc();
void led_onoff();
void wave();
void music_play();
void mp3music_play();
void codec_reg_setup();
void lcdprint(String data);
char keyin();
char select();
void clockcount();

I2SClass I2S(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

void menu(void){  
  char num;

  Serial.println("\n\nARM-First サンプルプログラム");
  Serial.println("          　　  　      CQ出版社");
  Serial.println("----------------------------------");
  Serial.println("1: 気圧温度センサ表示(ESCで中止)");
  Serial.println("2: 加速度センサ表示  (ESCで中止)");
  Serial.println("3: ジャイロセンサ表示(ESCで中止)");
  Serial.println("4: ボイスレコーダ(ESCで中止)");
  Serial.println("  (SD/wav下に録音)");
  Serial.println("5: サイン波出力 (30Hz～ ESCで中止)");
  Serial.println("6: WAVプレーヤ");
  Serial.println("  (SD /wav下を再生～192KHz～32bit)");
  Serial.println("7: MP3プレーヤ");
  Serial.println("  (SD /mp3下を再生～320Kbps 16bit)");
  Serial.println("8: LEDテスト(ESCで中止)");
  Serial.println("9: インターネットラジオ");
  Serial.println("a: NTP時計");   
  Serial.println("b: WiFi接続設定");
  Serial.println("c: ATコマンド実行");
  Serial.println("----------------------------------");
  Serial.print("No.?:");
  lcdprint("SELECT No.?");
  lcdprint("123456789ABCDEFG");
  char d;
  while(1){
    if(Serial.available()){
      num = Serial.read();
      break;  
    }
    num = select();    
    if(num != 0) break;
  } 
  Serial.write(num);
  Serial.println();
  switch(num){
    case '1':Serial.println("気圧、気温センサの値を表示");
      lcdprint("Press&Temp");
      exfunc1(press_calc);break;
    case '2':Serial.println("加速度センサの値を表示");
      lcdprint("Acceralator");
      Serial.println("ボードを傾けると値が変化します");
      exfunc1(acc_calc);break;
    case '3':Serial.println("ジャイロセンサの値を表示");
      lcdprint("Gyroscope");
      Serial.println("ボードを傾けると値が変化します");
      exfunc1(gyro_calc);break;
    case '4':
      recording();
      break;
    case '5': Serial.println("サイン波出力");
      lcdprint("Sine wave");
      wave();
      break;
    case '6':
      music_play();      
      break;
    case '7':
      mp3music_play();
      break;    
    case '8':Serial.println("LED テスト");
      lcdprint("LED Test");
      exfunc1(led_onoff);break;
      break;
    case '9':Serial.println("インターネットラジオ");
      lcdprint("Internet Radio");
      netradio();
      break;
    case 'a':Serial.println("NTP時計");
      lcdprint("NTP CLOCK");
      ntpclock();
      break;    
    case 'b':Serial.println("WiFi接続設定");
      lcdprint("WiFi Seting");
      wifisetup();
      break;
    case 'c':Serial.println("ATコマンド実行");
      lcdprint("AT Command");
      esp_flash();
      break;      
  }
}

void getstr(char *buf){
  int n;  
  for(n = 0;; n++){
    while(!Serial.available());
    buf[n] = Serial.read();
    Serial.write(buf[n]);
    if(buf[n] == 0x08) n -= 2;
    if(buf[n] == '\r') break;
  }
  buf[n] = '\0';
  Serial.read();  
}

void exfunc1(void (*fcp)()){
  while(!Serial.available()){
    fcp();
    delay(100);
    if(keyin() == 'e'){
      digitalWrite(PA15, HIGH);
      return;      
    }
  }
  Serial.read();
}

void exfunc2(void (*fcp)(char), char mode){
  while(!Serial.available()){
    fcp(mode);
    delay(100);
    if(keyin() == 'e') return;  
  }
  Serial.read();
}

/* 気圧温度データの取得 */
void press_calc(){
  static int hpa_s = 0, tmp_s = 0, measure_count = 0;
  double hpa, tmp;
  uint8_t buf[5];
  
  i2c_mem_write(0x20, PRESS_ADR, 0x10);
  i2c_mem_read(5, PRESS_ADR, 0x28, buf);
  hpa_s += (double)(buf[2]*65536+buf[1]*256+buf[0])/4096;
  tmp_s += (double)(buf[4]*256+buf[3])/128;
  measure_count += 1;
  if(measure_count == 10){
    hpa_s = int(hpa_s);
    tmp_s = int(tmp_s);
    Serial.printf("PRESS:%4dhpa TEMP:%2d.%1d℃\n", hpa_s/10, tmp_s/10, tmp_s % 10);
    char str[32];
    sprintf(str, "PRESS:%4dhpa", hpa_s/10);
    lcdprint(str);
    sprintf(str, "TEMP :%2d.%1d%cC", tmp_s/10, tmp_s % 10, 0xdf);
    lcdprint(str);
    measure_count = 0;
    hpa_s = tmp_s = 0;    
  }
}

/* 加速度データの取得 */
void acc_calc(){
  static int accex_s = 0, accey_s = 0, accez_s = 0, measure_count = 0;
  uint8_t buf[6];
  
  i2c_mem_write(0x30,ACC_ADR,0x10);
  i2c_mem_read(6,ACC_ADR,0x28, buf);
  accex_s += buf[1];
  accey_s += buf[3];
  accez_s += buf[5];
  measure_count += 1;
  if(measure_count == 5){
    Serial.printf("ACCX:%3d ACCY:%3d ACCZ:%3d\n", (int8_t)(accex_s/5), (int8_t)(accey_s/5), (int8_t)(accez_s/5));
    char str[32];
    sprintf(str, "X:%3d Y:%3d", (int8_t)(accex_s/5), (int8_t)(accey_s/5));
    lcdprint(str);
    sprintf(str, "Z:%3d", (int8_t)(accez_s/5));
    lcdprint(str); 
    measure_count = 0;
    accex_s = accey_s = accez_s = 0;
  }
}

/* ジャイロセンサデータ取得 */
void gyro_calc(){
  static int gyrox_s = 0, gyroy_s = 0, gyroz_s = 0, measure_count = 0;
  uint8_t buf[6];
  
  i2c_mem_write(0x30,ACC_ADR,0x11);
  i2c_mem_read(6,ACC_ADR,0x22, buf);
  gyrox_s += buf[1];
  gyroy_s += buf[3];
  gyroz_s += buf[5];
  measure_count += 1;
  if(measure_count == 5){
    Serial.printf("GYROX:%3d GYROY:%3d GYROZ:%3d\n", (int8_t)(gyrox_s/5), (int8_t)(gyroy_s/5), (int8_t)(gyroz_s/5));
    char str[32];
    sprintf(str, "X:%3d Y:%3d", (int8_t)(gyrox_s/5), (int8_t)(gyroy_s/5));
    lcdprint(str);
    sprintf(str, "Z:%3d", (int8_t)(gyroz_s/5));
    lcdprint(str); 
    measure_count = 0;
    gyrox_s = gyroy_s = gyroz_s = 0;
  }
}

/* LED ON/OFF */
void led_onoff(){
  digitalWrite(PA15, HIGH);    // turn the YELLOW LED on
  digitalWrite(PB4, LOW);      // turn the BLUE LED off
  delay(300);                  // wait for a second
  digitalWrite(PA15, LOW);     // turn the YELLOW LED off
  digitalWrite(PB4, HIGH);     // turn the BLUE LED on
  delay(300);                  // wait for a second  
}

/* サイン波出力　*/
void wave(){
  int16_t buf[8192*2];      //DMAリングバッファ
  double frequency;
  double amplitude = 30000;
  int16_t buff[4410*2];

  codec_reg_setup();        //DACの初期化 
  I2S.setsample(44100);
  codec_reg_setup();
  Serial.print("Frequency:880Hz");
  lcdprint("Frequency:880Hz");
  frequency = 880;
  for (int n = 0; n < 4410*2; n++) {
      buff[n] = (sin(2 * PI * frequency * n / 44100)) * amplitude;
  }
  while(1){
    for (int n = 0; n < 4410*2; n++) {
      I2S.write(buff[n]);   //left MSB
      I2S.write(0);         //left LSB
      I2S.write(buff[n]);   //right MSB
      I2S.write(0);         //right LSB
    }
    if(Serial.available()|| keyin()) break;
  }
  Serial.read();
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
  static int vol = 300;

  if(num == 'u') {      //音量UP
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol + 10));//左音量設定　+2.5dB
    codec_writeReg(0x07, 0x200 | (vol + 10));//右音量設定
    Serial.println(codec_readReg(0x06));     //設定値読出し
    lcd.setCursor(13, 1);
    lcd.print(codec_readReg(0x06));
  }
  if(num == 'd') {//音量Down
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol - 10));//左音量設定　-2.5dB
    codec_writeReg(0x07, 0x200 | (vol - 10));//右音量設定
    Serial.println(codec_readReg(0x06));     //設定値読出し
    lcd.setCursor(13, 1);
    lcd.print(codec_readReg(0x06));
  }
}

/* WAV音楽プレーヤ */
void music_play(){
  int16_t buf[8192*2];      //DMAリングバッファ
  uint8_t buff[1536*4 + 4]; //SDカード読出しバッファ
  char path[30];            //フォルダ名（wav）
  char fname[100][50];      //ファイル名リスト
  int count, num, fcount;
  SdFatSdio sd;             //sdioオブジェクト生成
  codec_reg_setup();        //DACの初期化      
  if(!sd.begin())           //sdカードの初期化
  {
    Serial.println("SDカードを認識できません。");
    return;
  }
  I2S.setBuffer(buf, 8192*2);    //DMAバッファの設定
  File root = sd.open("/wav");      //wavフォルダをオープン
  File file = root.openNextFile();  //wavフォルダ内のファイルをオープン
  for(fcount = 0; file; fcount++){  //全てのファイル名が取得できるまで繰り返し
    file.getName(fname[fcount], 50);    //オープンできたファイル名を取得
    //Serial.println(fname[fcount]);
    file = root.openNextFile();     //次のファイルをオープン
  }
    Serial.println("音楽プレーヤメニュー");       //サブメニューのコンソールへの表示
    lcdprint("Wave Player");
    Serial.println("++++++++++++++++++++++++");
    Serial.println("音量(u:大きく d:小さく)");
    Serial.println("選曲(n:次曲   p:前曲)");
    Serial.println("終了(e:プレーヤ終了)");
    Serial.println("++++++++++++++++++++++++");
    for(int n = 0; n < fcount; n++){    //取得したファイルを順に再生
      strcpy(path, "/wav/");
      strcat(path, fname[n]);
      Serial.println(fname[n]);
      lcdprint(fname[n]);
      File file = sd.open(path);        //再生するファイルのオープン
      file.seek(8);                     //ファイルポインタを8バイト目に設定
      file.read(buff, 4);               //
      buff[4] = 0;
      if(strcmp((char *)buff, "WAVE") != 0){//"WAVE"の文字があるかチェック
        Serial.println("Not WAVE File");    //WAVファイルでないことを表示
        //Serial.println((char *)buff); 
        continue;
      }
      file.seek(search(file, "fmt "));  //ヘッダ内に"fmt"の文字を探してファイルポインタをセット
      file.read(buff, 4);
      int chunk_len = *(int32_t *)buff; //WAV形式データのサイズの取得
      //Serial.printf("Chunk length:   %dB\n",chunk_len);
      file.read(buff, chunk_len);       //WAV形式データの読出し
      int bitrate = *(uint32_t *)&buff[4];//ビットレートの取得
      int sample = *(uint16_t *)&buff[14];//サンプルサイズの取得
      Serial.printf("Sample rate:%dHz\n",bitrate);
      Serial.printf("Bit accuracy:%dBit\n",sample);
      char str[32];
      sprintf(str,"%3dKHz %dbit", bitrate/1000,sample);
      lcdprint(str);
      int btr192 = 0;
      if(bitrate == 192000){  //192KHzサンプルファイルは96KHzで再生するためにサンプル周波数を変更
        bitrate = 96000;
        btr192 = 1;
      }
      I2S.setsample(bitrate);   //I2Sのビットレートを設定
      switch(sample){
        case 16: count = 2; break;  //sample size:16bit
        case 24: count = 3; break;  //sample size:24bit
        case 32: count = 4; break;  //sample size:32bit
      }
      int pos = search(file, "data") + 4; //音楽データエリアの先頭位置を取得
 
      //Serial.printf("Data Pos:%d\n", pos);
      uint32_t d;
      Serial.print(">");
      while(file.available()){
          int pbyte = pos % 4;
          file.seek(pos - pbyte);   //SDカードの読出しは４バイト境界からしかできないので読出しのアライメントを４バイト境界に合わせる
          pos += 1536*4;     
          file.read(buff, 1536*4 + 4);//SDカードの読出しバッファ分読出し
            for(int n=pbyte, h=0; n < 1536*4+pbyte; n += count*((h%2)*2+1)){
                d = *(uint32_t *)&buff[n] << (4 - count) * 8;//サンプルサイズに合わせてを32ビットデータに拡張
                I2S.write(d >> 16);             //16ビットずつDMAバッファに転送
                I2S.write(d);
                if(btr192) h++;
            }
            if(Serial.available() || (num = keyin())){ //キー入力のチェック
                if(Serial.available()){
                  num = Serial.read();            //指示を読み出し
                }
                Serial.printf("%c:", num);
                volume(num);                    //ボリュームコントロール
                if(num == 'n'){                 //次の曲へ
                  break;
                }else if(num == 'p'){
                  n -= 2;                       //前の曲へ
                  break;
                }else if(num == 'e'){           //終了
                  file.close();
                  return;
                }
                Serial.print(">");
            }
      }
      file.close();         //ファイルクローズ
      delay(500);
    }
}

/* MP3音楽プレーヤ */
void mp3music_play(){
  int16_t buf[8192*2];
  char path[30];
  char fname[100][50];
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];//MP3デコード出力バッファの設定
  mp3dec_frame_info_t info;        //MP3デコーダ情報構造体の定義
  mp3dec_t mp3d;                   //MP3デコーダ構造体
  uint8_t fbuf[0x600];             //MP3フレームバッファ
  int count, num, fcount;
  SdFatSdio sd;
  codec_reg_setup();
  if(!sd.begin())
  {
    Serial.println("SDカードを認識できません。");
    return;
  }
  I2S.setBuffer(buf, 8192*2);    
  File root = sd.open("/mp3");
  File file = root.openNextFile();
  for(fcount = 0; file; fcount++){
    file.getName(fname[fcount], 50);
    //Serial.println(fname[fcount]);
    file = root.openNextFile();
  }
  Serial.println("音楽プレーヤメニュー");
  lcdprint("MP3 Player");
  Serial.println("++++++++++++++++++++++++");
  Serial.println("音量(u:大きく d:小さく)");
  Serial.println("選曲(n:次曲   p:前曲)");
  Serial.println("終了(e:プレーヤ終了)");
  Serial.println("++++++++++++++++++++++++");
  
  int samples = 1;
  int d_on;
  mp3dec_init(&mp3d);
  
  for(int m_no = 0; m_no < fcount; m_no++){//ファイルリストのファイルを順に再生
    d_on = 1;
    strcpy(path, "/mp3/");
    strcat(path, fname[m_no]);
    Serial.println(fname[m_no]);
    lcdprint(fname[m_no]);
    File file = sd.open(path); //指定したmp3フォルダ内のファイルをオープン

    int pos = 0;              //フレームポインタ
    I2S.setsample(44100);     //サンプリング周波数を44100Hzに設定
    
    if(!d_on) Serial.print(">");
    for(int fn = 1; file.available(); fn++){
      int pbyte = pos % 4;    
      file.seek(pos - pbyte); //4バイト境界でフレーム位置をファイルポインタに設定
      file.read(fbuf, sizeof(fbuf));  //最大のフレームサイズでフレームを読出し   
      samples = mp3dec_decode_frame(&mp3d, fbuf+pbyte, sizeof(fbuf), pcm, &info);//MP3デコード　結果PCMに格納
      pos += info.frame_bytes;  //実際のフレームサイズでフレームポインタを更新
      uint32_t d;
      for(int n=0; n < 1152*2; n++){
        d = *(uint32_t *)&pcm[n] << 16;
        I2S.write(d >> 16);         //16ビットずつDMAバッファに転送
        I2S.write(d);
      }
      if(samples != 0 && d_on && samples > info.frame_bytes){
        //Serial.printf("%n-Pos:%d ", fn, file.position());
        //Serial.printf("%d-Samples:%d\n", fn,samples);
        Serial.printf("Layer:%d\n", info.layer);
        Serial.printf("Sample Rate:%dHz\n", info.hz);
        //Serial.printf("Channels:%d\n", info.channels);
        //Serial.printf("Frame:%dbyte\n", info.frame_bytes);
        Serial.printf("Bit rate:%dkbps\n", info.bitrate_kbps);
        char str[32];
        sprintf(str,"%3dKBPS %2dK", info.bitrate_kbps, info.hz/1000);
        lcdprint(str);
        d_on = 0;
        Serial.print(">");
      }
      if(Serial.available() || (num = keyin())){ //キー入力のチェック
        if(Serial.available()){
          num = Serial.read();            //指示を読み出し
        }
        Serial.printf("%c:", num);
        volume(num);                  
        if(num == 'n'){
          break;
        }else if(num == 'p'){
          m_no -= 2;
          break;
        }else if(num == 'e'){
          file.close();
          return;
        }
        Serial.print(">");
      }
    }
    file.close();
    delay(500);
  }
}

/* インターネットラジオ*/
void netradio(){
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
  
  for(int num = 0;num < 7; num++){
    initWiFi();                 //WiFi初期化
    Serial.println("Site:" + urllist[num].host + urllist[num].file);
    lcdprint(urllist[num].file);
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
    wp = Serial2.readBytes(uartbuf,8192*3);
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
      if(Serial.available() || (comm = keyin())){ //キー入力のチェック
        if(Serial.available()){
          comm = Serial.read();            //指示を読み出し
        }
        Serial.printf("%c:", comm);
        volume(comm);                  
        if(comm == 'n'){
          break;
        }else if(comm == 'p'){
          num -= 2;
          break;
        }else if(comm == 'e'){
                 Serial2.print("+++");
                 return;
              }
              Serial.print(">");
        }
    }
    Serial2.begin(921600, 0);   
    Serial2.print("+++");
    delay(1000);
  }
}

/* NTP時計 */
void ntpclock(){
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
  while(!Serial.available() && !(keyin() == 'e'));
  RTC_deint();
}
/* NTP時計1秒割込みコールバック */
void clockcount(){
  struct tm stime;
  char day[][4] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
  char str[32];
  
  stime = getstime();
  Serial.printf("%4d/%02d/%02d(%s) %02d:%02d:%02d\n", 1900+stime.tm_year,stime.tm_mon+1,stime.tm_mday+1,day[stime.tm_wday],stime.tm_hour,stime.tm_min,stime.tm_sec);
  sprintf(str,"%4d/%02d/%02d(%s) ",1900+stime.tm_year,stime.tm_mon+1,stime.tm_mday+1,day[stime.tm_wday]);
  lcd.setCursor(0, 0);
  lcd.print(str);
  sprintf(str,"%02d:%02d:%02d        ",stime.tm_hour,stime.tm_min,stime.tm_sec); 
  lcd.setCursor(0, 1);
  lcd.print(str);  
}

void recording(void){
  uint8_t wav_header[WAV_HEADER_SIZE] = {
  0x52, 0x49, 0x46, 0x46, /* RIFF */
  0x00, 0x00, 0x00, 0x00, /* ファイルサイズ - 8 */
  0x57, 0x41, 0x56, 0x45, /* WAVE */
  0x66, 0x6D, 0x74, 0x20, /* fmt */
  0x10, 0x00, 0x00, 0x00, /* 16 */
  0x01, 0x00, 0x02, 0x00, /* 1 2 */
  0x80, 0xBB, 0x00, 0x00, /* 48000 */
  0x00, 0xEE, 0x02, 0x00, /* 192000 */
  0x04, 0x00, 0x10, 0x00, /* 4 16 */
  0x64, 0x61, 0x74, 0x61, /* data */
  0x00, 0x00, 0x00, 0x00  /* データサイズ */
  };
  uint32_t total_wr_size;
  uint32_t rec_period_cnt;
  uint32_t wav_buff_cnt;
  uint8_t end_req;
  int16_t pcm_buff[PCM_BUFF_LEN];
  int comm;

  digitalWrite(PB4, LOW);   // turn the BLUE LED off
  SdFatSdio sd;
  codec_writeReg(0x02, 0x00);
  if(!sd.begin())
  {
    Serial.println("SDカードを認識できません。");
    lcdprint("No SD CARD     ");
    return;
  }

  File file;
  setpcmbuf(pcm_buff);
  file = sd.open("/wav/rec_m1m2.wav", (O_WRITE | O_CREAT | O_TRUNC));
  if(file){
    Serial.println("'e'押下もしくは10分間の録音で終了します。");
    lcd.setCursor(0, 0);
    lcdprint("Start Record");
    lcdprint("Stop -> SEL");

    file.write(wav_header, WAV_HEADER_SIZE);

    StartAudioProcess();

    total_wr_size = 0;
    rec_period_cnt = 0;
    wav_buff_cnt = 0;
    end_req = 0;

    while((rec_period_cnt < RECORDING_PERIOD) && (end_req == 0)){
      if(Serial.available()|| (comm = keyin())){
        if(Serial.available()){
          comm = Serial.read();            //指示を読み出し
        }
        Serial.printf("%c:", comm);
        if(comm == 'e'){
          end_req = 1;
        }
      }
      if(pcm_buff_state == PCM_BUFF_STATE_HALFCPLT){
        digitalWrite(PB4, LOW);   // turn the BLUE LED off
        lcd.setCursor(14, 0);
        lcd.print(" *");
        file.write(&pcm_buff[0], (PCM_BUFF_PERIOD/2*AUDIO_ST_BUFF_LEN)*2);
        pcm_buff_state = PCM_BUFF_STATE_NOTCPLT;
      }
      else if(pcm_buff_state == PCM_BUFF_STATE_CPLT){
        digitalWrite(PB4, HIGH);  // turn the BLUE LED on
        lcd.setCursor(14, 0);
        lcd.print("* ");
        file.write(&pcm_buff[PCM_BUFF_PERIOD/2*AUDIO_ST_BUFF_LEN], (PCM_BUFF_PERIOD/2*AUDIO_ST_BUFF_LEN)*2);
        pcm_buff_state = PCM_BUFF_STATE_NOTCPLT;
        rec_period_cnt++;
      }
      else{
        ;
      }
    }

    FinishAudioProcess();

    digitalWrite(PB4, HIGH);  // turn the BLUE LED on

    total_wr_size = rec_period_cnt*(PCM_BUFF_PERIOD*AUDIO_ST_BUFF_LEN)*2;
    file.seek(40);
    file.write(&total_wr_size, 4);

    total_wr_size += (44-8);
    file.seek(4);
    file.write(&total_wr_size, 4);
    file.close();
    Serial.printf("録音時間: %dsec\n", (rec_period_cnt*PCM_BUFF_PERIOD*AUDIO_PROCESS_PERIOD/1000));

    digitalWrite(PB4, LOW);  // turn the BLUE LED off
  }
  else{
    Serial.println("ファイルを生成できません。");
  }
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

/* RTSピン設定 */
void setRTSpin(){
  GPIO_InitTypeDef GPIO_InitStruct;
  
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  
}

/* WiFiの初期化 */
void initWiFi(){
    pinMode(PB1, OUTPUT);   //RESET信号を出力設定
    digitalWrite(PB1, LOW); //RESET信号をLow
    delay(100);
    digitalWrite(PB1, HIGH);//RESET信号をHigh
    lcdprint("");
    lcd.setCursor(0,1);
    for(int n = 0; n < 5; n++){ //初期化処理の終了待ち
      Serial.print("*");
      lcd.print("*");
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
    lcdprint(sid);
    atout("AT+UART_CUR=921600,8,1,0,2");
    Serial2.begin(921600, 1); 
    delay(100);
}

/* WiFi 設定 */
void wifisetup(){
  char linebuf[50];  
  pinMode(PB1, OUTPUT);   //RESET信号を出力設定
  digitalWrite(PB1, LOW); //RESET信号をLow
  delay(100);
  digitalWrite(PB1, HIGH);//RESET信号をHigh
  for(int n = 0; n < 5; n++){ //初期化処理の終了待ち
     Serial.print("*");
     delay(1000);
  }
  while(Serial2.available()){
    Serial2.read();
  }
  Serial.println();
  Serial2.begin(115200);
  Serial.print("SSID?:"); 
  getstr(linebuf);        //SSID入力
  String ssid = linebuf;
  Serial.print("PASS?:");
  getstr(linebuf);        //パスワード入力
  String passwd = linebuf; 
  Serial.println("Connecting!!");
  /* AT命令　アクセスポイントへの接続 */
  Serial.println(atout("AT+CWMODE=1"));
  Serial.println(atout("AT+CWJAP_DEF=\""+ssid+"\",\""+passwd+"\""));
  Serial.print("Push Enter key:");
  getstr(linebuf);
}

/* ATコマンド出力 */
void esp_flash(){
  int data1, data2;
  int f1 = 0, f2 = 0;
  
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
  while(1){
    if(Serial.available()){
        data1 = Serial.read();
        f1 = 1;
        if(data1 == 0x1b) return;
    }
    if(f2){    
       Serial.write(data2);
       f2 = 0;
    }
    if(Serial2.available()){ 
      data2 = Serial2.read();
      f2 = 1;
    }
    if(f1){
        Serial2.write(data1);
        f1 = 0;
    }
  }
}

/* lcd print routine */
void lcdprint(String data){
   lcddata[0] = lcddata[1];
   lcddata[1] = data;
   lcd.setCursor(0, 0);
   lcd.print(lcddata[0]+"                ");
   lcd.setCursor(0, 1);
   lcd.print(lcddata[1]+"                ");
}

/* Keypad keyinput */
char keyin(){
  static int state = 0;
  int keyout;
  int res;
  
  if(state == 0){
    keyout = analogRead(PC0);
    if(keyout < 3700){
      res  = 'e';
      if(keyout < 2800) res = 'p';
      if(keyout < 2000) res = 'd';
      if(keyout < 1200) res = 'u';
      if(keyout < 400)  res = 'n';
      state = 1;
      return res;    
    }
    else return 0;
  }
  else{
    if(analogRead(PC0) > 4000){
      state = 0;
      return 0; 
    }
    else return 0;
  }
}

char select(){
  static int cur = 0;
  char conv[] = "123456789abcdefg";
  lcd.setCursor(cur,1);
  lcd.blink();
  switch(keyin()){
    case 'n':
      lcd.setCursor(++cur,1);
      if(cur > 15) cur = 15;
      break;
    case 'p':
      lcd.setCursor(--cur,1);
      if(cur < 0) cur = 0;
      break;
    case 'e':
      lcd.noCursor();
      return conv[cur];
  }
  return 0;
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

uint8_t i2c_mem_write(uint8_t data, uint8_t addr, uint8_t reg){
  uint8_t error;
  
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(data);
  error = Wire.endTransmission();
  return error;
}

uint8_t i2c_mem_read(uint8_t count, uint8_t addr, uint8_t reg, uint8_t * buf){
  uint8_t error;
  uint8_t n;
  
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(addr, count);
  delay(1);
  for(n = 0; n < count; n++){
    buf[n] = Wire.read();
  }
  error = Wire.endTransmission();
  return error;
}

/* main */
void setup() {
  delay(1000);
  pinMode(PA15, OUTPUT);
  pinMode(PB4, OUTPUT);
  pinMode(PC0, INPUT);
  lcd.begin(16, 2);
  digitalWrite(PA15, HIGH);
  analogReadResolution(12);
  Serial.begin(115200);
  Wire.begin(); // start the I2C driver for codec register setup 
  I2S.begin(I2S_PHILIPS_MODE, 44100, 32);
  RTC_Init();                   //RTC初期化
  SdFile::dateTimeCallback(dateTime);
}

void loop() {
  menu();
}
