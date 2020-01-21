/***********************************************************************/
/*                                                                     */
/*  FILE        :Radio_client.c                                                */
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
#define FLEN 1024
#define BLEN (FLEN*32)

#include <Wire.h>   //I2Cライブラリのヘッダファイル
#include "I2S.h"    //I2Sライブラリのヘッダファイル
#include "SdFat.h"  //MicroSDのヘッダファイル
#include "math.h"
#include "minimp3.h"
#include <LiquidCrystal.h>
#include <time.h>

const int rs = PA4, en = PB11, d4 = PC13, d5 = PB8, d6 = PB12, d7 = PC7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
String lcddata[2] = {"",""};
unsigned int rp = 0, wp = 0, quant = 0;
int starttime;

uint8_t i2c_mem_write(uint8_t reg, uint8_t addr, uint8_t data);
uint8_t i2c_mem_read(uint8_t count, uint8_t addr, uint8_t reg, uint8_t * buf);
uint16_t codec_readReg(uint8_t reg);
uint8_t codec_writeReg(uint8_t reg, uint16_t data);
void music_play();
void mp3music_play();
void codec_reg_setup();
void lcdprint(String data);
char keyin();
char select();
void clockcount();
void scrollbuf(char *buf);
void bflush();

I2SClass I2S(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

void menu(void){  
  char num;

  Serial.println("\n\nAudio Jack");
  Serial.println("----------------------------------");
  Serial.println("1: WAV Player");
  Serial.println("  (SD /wav下を再生～192KHz～32bit)");
  Serial.println("2: MP3 Player");
  Serial.println("  (SD /mp3下を再生～320Kbps 16bit)");
  Serial.println("3: Internet Radio(Higher BitRate)");
  Serial.println("4: WiFi接続設定");
  Serial.println("----------------------------------");
  Serial.print("No.?:");
  lcdprint("Audio Jack");
  lcdprint("1234 <-SEL No.?");
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
    case '1':
      music_play();      
      break;
    case '2':
      mp3music_play();
      break;    
    case '3':Serial.println("インターネットラジオ");
      lcdprint("Internet Radio");
      netradio();
      break;
    case '4':Serial.println("WiFi接続設定");
      lcdprint("WiFi Seting");
      wifisetup();
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
  char str[16];

  if(num == 'u') {      //音量UP
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol + 10));//左音量設定　+2.5dB
    codec_writeReg(0x07, 0x200 | (vol + 10));//右音量設定
    Serial.println(codec_readReg(0x06));     //設定値読出し
    lcd.setCursor(9, 1);
    sprintf(str,"VOL:%3d", codec_readReg(0x06));
    lcd.print(str);
    starttime = millis();
  }
  if(num == 'd') {//音量Down
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol - 10));//左音量設定　-2.5dB
    codec_writeReg(0x07, 0x200 | (vol - 10));//右音量設定
    Serial.println(codec_readReg(0x06));     //設定値読出し
    lcd.setCursor(9, 1);
    sprintf(str,"VOL:%3d", codec_readReg(0x06));
    lcd.print(str);
    starttime = millis();
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
  int frame = 1152;
  uint8_t inbuf[0x600];
  int16_t buf[8192*2];
  uint8_t uartbuf[BLEN];  //UARTリングバッファ
  char urlbuf[64], strbuf[17];

  /* URL LIST : "URL", "W"   TCP/IP Window size = W * MSS */ 
  struct ul {
    String url;
    String wnd;
  }urllist[]= {
    "http://wbgo.streamguys.net/wbgo128","6",
    "http://ice2.somafm.com/bootliquor-320-mp3","7",
    "http://icecast.omroep.nl/radio4-bb-mp3","7",
    "http://66.70.187.44:9069/j1","6",
    "http://18443.live.streamtheworld.com:80/KDFCFM_SC","6",
    "http://mediaserv30.live-streams.nl:8086/stream","12",
    "http://89.16.185.174:8004/stream","13",
    "http://109.123.116.202:8008/mp3","6",
    "http://ice2.somafm.com/u80s-256-mp3","6",
    "http://ice2.somafm.com/christmas-256-mp3","7",    
    "http://ice1.somafm.com/defcon-128-mp3","6",
    "http://ice1.somafm.com/folkfwd-128-mp3","6",
    "http://ice1.somafm.com/illstreet-128-mp3","6",
    "http://ice1.somafm.com/secretagent-128-mp3","6",
    "http://ice1.somafm.com/bootliquor-128-mp3","6"};

  codec_reg_setup();            //DAC初期化
  I2S.setBuffer(buf, 8192*2);   //I2Sバッファ初期化 
  //I2S.setsample(44100);         //I2Sサンプリング周波数設定
  Serial.println("ラジオメニュー");
  Serial.println("++++++++++++++++++++++++");
  Serial.println("音量(u:大きく d:小さく)");
  Serial.println("選局(n:次局   p:前局)");
  Serial.println("終了(e:終了)");
  Serial.println("++++++++++++++++++++++++");

  while(1){
  for(int num = 0;num < 15; num++){    
    while (!Serial2.available()); //レスポンス待ち
    String res = Serial2.readStringUntil(10);
    Serial.println("Site:" + urllist[num].url);
    strcpy(urlbuf, (urllist[num].url + "   ").c_str());
    String str = "c," + urllist[num].url + "," + urllist[num].wnd;
    Serial2.write(str.c_str());
    //ラジオ音声データバッファリング：BLEN Byte
    delay(500);
    wp = Serial2.readBytes(uartbuf, 8192);
    delay(500);
    Serial.printf("Buffer filled:%dByte\n", wp);
    Serial.print(">");
    wp = 8192;
    rp = 0;
    int prt = 0, first = 0;
    starttime = millis();
    while(1){
      if((BLEN - (wp - rp) > FLEN) && ((quant = Serial2.rxbufferhalf()) >= FLEN)){
          Serial2.readBytes((uint8_t*)(uartbuf + (wp % BLEN)), FLEN);
         wp += FLEN;
      //Serial.printf("w_wp:%d rp:%d qt:%d \n", wp, rp, quant);
      }
      if(wp - rp >= 0x600){                          //リングバッファのデータが0x600以上あれば読出し
        for(int n = 0; n < 0x600; n++){             //MP3デコーダ入力バッファにデータをコピー
           inbuf[n] = uartbuf[(rp + n) % BLEN];
        }
        samples = mp3dec_decode_frame(&mp3d, inbuf, 0x600, pcm, &info);//MP3デコード
        frame = info.frame_bytes;
        rp += frame;             //MP3デコード後のフレームバイト数分リングバッファの読出しポインタを更新
        if(first == 0){
          I2S.setsample(info.hz);         //I2Sサンプリング周波数設定
          first = 1;
        }
        uint32_t d;
        for (int n = 0; n < 1152 * 2; n++) {
          d = (*(uint32_t *)&pcm[n]) << 16;
          I2S.write(d >> 16);
          I2S.write(d);
        }
        if(prt % 20 == 0){
          lcd.setCursor(0, 0);
          strncpy(strbuf, urlbuf, 17);
          strbuf[16] = '\0';
          lcd.print(strbuf);
          scrollbuf(urlbuf);          
        }
        if(prt % 100 == 0){
          Serial.printf("DoubleBuffer:%5dB IputBuffer:%4dB ", wp - rp, quant);
          //Serial.printf("frame:%d ", frame); 
          //Serial.printf("Layer:%d\n", info.layer);
          Serial.printf("Sample Rate:%dHz ", info.hz);
          Serial.printf("Bit rate:%dkbps\n", info.bitrate_kbps);
          lcd.setCursor(0, 1);
          int bf = (wp - rp)/100;
          sprintf(strbuf,"B:%5dB BR:%3dK", wp - rp, info.bitrate_kbps);
          lcd.print(strbuf);
        }
        if(millis()-starttime > 100){//100ms読出しが出来ないときは再接続する
          Serial2.write('e'); //Client disconnect command
          num--;
          break; 
        }        
        starttime = millis();
        prt++; 
      }
      if(Serial.available() | (comm = keyin())){ //キー入力のチェック
        if(Serial.available()){
          comm = Serial.read();            //指示を読み出し
        }
        Serial.printf("%c:", comm);
        volume(comm);                  
        if(comm == 'n'){      //next
          Serial2.write('e'); //Client disconnect command
          break;
        }else if(comm == 'p'){//previous 
          Serial2.write('e'); //Client disconnect command
          num -= 2;
          break;
        }else if(comm == 'e'){           //終了
          Serial2.write('e'); //Client disconnect command
          return;
        }
        Serial.print(">");
      }          
    }
  }
 }
}

/* 命令出力 */
String commandout(String data) {
  String res = "";
  Serial2.write(data.c_str());
  while(1){
    while (!Serial2.available()); 
    char c = Serial2.read();
    res += c;
    if(c == '>') break;
  }
  return res;  
}

void bflush(){
  while (Serial2.available()){
    Serial2.read();
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
/* Reset First Bee */ 
void initWiFi(){
    delay(100);
    pinMode(PB1, OUTPUT);
    digitalWrite(PB1, LOW);
    delay(100);
    digitalWrite(PB1, HIGH);
    delay(1000);
 }
 

/* WiFi 設定 */
void wifisetup(){
  char linebuf[50];
  
  initWiFi();
  bflush();
  Serial.println();
   Serial.print("SSID?:"); 
  getstr(linebuf);        //SSID入力
  String ssid = linebuf;
  Serial.print("PASS?:");
  getstr(linebuf);        //パスワード入力
  String passwd = linebuf; 
  Serial.println("Connecting!!");
  String str = (String)"a," + ssid + "," + passwd;  //WiFi AP connect
  Serial.println(commandout(str));
  Serial.print("Push Enter key:");
  getstr(linebuf);
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

/* scroll buffer */
void scrollbuf(char *buf){
  int tmp, n;

  tmp = buf[0];
  for(n = 0; n < strlen(buf) - 1; n++){
    buf[n] = buf[n+1];
  }
  buf[n] = tmp;
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
      if(cur > 3) cur = 0;
       break;
    case 'p':
      lcd.setCursor(--cur,1);
      if(cur < 0) cur = 3;
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
  lcdprint("Audio Jack");
  digitalWrite(PA15, HIGH);
  analogReadResolution(12);
  Serial.begin(115200); //Set USB Serial baudrate
  Serial2.begin(921600, 1); //Set UART2 baudrate & RTS ON
  setRTSpin();  //RTS機能選択
  lcdprint("Wi-Fi Setup");
  initWiFi();
  bflush();
  Serial.println(commandout(" "));
  Wire.begin(); // start the I2C driver for codec register setup 
  I2S.begin(I2S_PHILIPS_MODE, 44100, 32);
}

void loop() {
  menu();
}
