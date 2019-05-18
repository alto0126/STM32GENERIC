/***********************************************************************/
/*                                                                     */
/*  FILE        :Arm_First_Test.c                                                */
/*  DATE        :2019/3/24                                              */
/*  DESCRIPTION :Arm-First DEMO PROGRAM                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/
#define CODEC_ADDR 31 // i2c address
#define PRESS_ADR 93
#define ACC_ADR 107

#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_MAX_SAMPLES_PER_FRAME 1152*2

#include <Wire.h>
#include "I2S.h"
#include "SdFat.h"
#include "math.h"
#include "minimp3.h"

uint8_t buff[1536*4];
int16_t buf[8192*2];
char fname[100][30];
int fcount;

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
void temp_calc();
void codec_reg_setup();

I2SClass I2S(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

void menu(void){  
  char num;
  
  Serial.println("\n\n   ARM-First デモプログラム");
  Serial.println("          　　  　      CQ出版社");
  Serial.println("----------------------------------");
  Serial.println("1: 気圧温度センサ表示");
  Serial.println("2: 加速度センサ表示");
  Serial.println("3: ジャイロセンサ表示");
  Serial.println("4: サイン波出力 (30Hz～)");
  Serial.println("5: WAVプレーヤ");
  Serial.println("  (SD /wav下を再生～192KHz～32bit)");
  Serial.println("6: MP3プレーヤ");
  Serial.println("  (SD /mp3下を再生～320Kbps 16bit)");
  Serial.println("7: LEDテスト");
  Serial.println("8: ESP ATコマンド入力実行");
  Serial.println("----------------------------------");
  Serial.print("No.?:");
  while(!Serial.available());
  num = Serial.read();
  Serial.write(num);
  Serial.println();
  switch(num){
    case '1':Serial.println("気圧、気温センサの値を表示");
      exfunc1(press_calc);break;
    case '2':Serial.println("加速度センサの値を表示");
      Serial.println("ボードを傾けると値が変化します");
      exfunc1(acc_calc);break;
    case '3':Serial.println("ジャイロセンサの値を表示");
      Serial.println("ボードを傾けると値が変化します");
      exfunc1(gyro_calc);break;
    case '4': Serial.println("サイン波出力");
      wave();
      break;
    case '5':
      music_play();      
      break;
    case '6':
      mp3music_play();
      break;    
    case '7':Serial.println("LED テスト");
      exfunc1(led_onoff);break;
      break;    
    case '8':Serial.println("ATコマンド入力　終了はESC");
      esp_flash();
      //exfunc1(temp_calc);
      break;
  }
}

void getstr(char *buf){
  int n;  
  for(n = 0;; n++){
    while(!Serial.available());
    buf[n] = Serial.read();
    Serial.write(buf[n]);
    if(buf[n] == 0x0a) break;
  }
  buf[n] = '\0';  
}

void exfunc1(void (*fcp)()){
  while(!Serial.available()){
    fcp();
    delay(100);
  }
  Serial.read();
}

void exfunc2(void (*fcp)(char), char mode){
  while(!Serial.available()){
    fcp(mode);
    delay(100);
  }
  Serial.read();
}

/* 気圧温度データの取得 */
void press_calc(){
  static int hpa_s = 0, tmp_s = 0, measure_count = 0;
  int hpa, tmp;
  uint8_t buf[5];
  
  i2c_mem_write(0x20, PRESS_ADR, 0x10);
  i2c_mem_read(5, PRESS_ADR, 0x28, buf);
  hpa_s += (buf[2]*65536+buf[1]*256+buf[0])/4096;
  tmp_s += (buf[4]*256+buf[3])/128;
  measure_count += 1;
  if(measure_count == 5){
    Serial.printf("PRESS:%4dhpa TEMP:%2d℃\n", hpa_s/5, tmp_s/5);
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
    measure_count = 0;
    gyrox_s = gyroy_s = gyroz_s = 0;
  }
}

/* LED ON/OFF */
void led_onoff(){
  pinMode(PA15, OUTPUT);
  pinMode(PB4, OUTPUT);

  digitalWrite(PA15, HIGH);    // turn the YELLOW LED on
  digitalWrite(PB4, LOW);      // turn the BLUE LED off
  delay(300);                  // wait for a second
  digitalWrite(PA15, LOW);     // turn the YELLOW LED off
  digitalWrite(PB4, HIGH);     // turn the BLUE LED on
  delay(300);                  // wait for a second  
}

/* サイン波出力　*/
void wave(){
  double frequency;
  double amplitude = 30000;
  char buf[8];
  int16_t buff[4410*2];
  
  I2S.setsample(44100);
  codec_reg_setup();
  while(1){
    Serial.print("Frequency:");
    getstr(buf);
    frequency = (double)atoi(buf);
    if(frequency == 0) break;
    for (int n = 0; n < 4410*2; n++) {
        buff[n] = (sin(2 * PI * frequency * n / 44100)) * amplitude;
    }
    while(1){
      for (int n = 0; n < 4410*2; n++) {
        I2S.write(buff[n]); //left
        I2S.write(0); //right 
        I2S.write(buff[n]); //left
        I2S.write(0); //right 
      }
      if(Serial.available()) break;
    }
    Serial.read();
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

  if(num == 'u') {
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol + 10));
    codec_writeReg(0x07, 0x200 | (vol + 10));
    Serial.println(codec_readReg(0x06));
  }else if(num == 'd') {
    delay(100);
    vol = codec_readReg(0x06);
    codec_writeReg(0x06, 0x200 | (vol - 10));
    codec_writeReg(0x07, 0x200 | (vol - 10));
    Serial.println(codec_readReg(0x06));
  }
}

/* WAV音楽プレーヤ */
void music_play(){
  char path[30];
  int count, twice, half, num;
  SdFatSdio sd;
  sd.begin();   
  File root = sd.open("/wav");
  File file = root.openNextFile();
  for(fcount = 0; file; fcount++){
    file.getName(fname[fcount], 30);
    //Serial.println(fname[fcount]);
    file = root.openNextFile();
  }
    Serial.println("音楽プレーヤメニュー");
    Serial.println("++++++++++++++++++++++++");
    Serial.println("音量(u:大きく d:小さく)");
    Serial.println("選曲(n:次曲   p:前曲)");
    Serial.println("終了(e:プレーヤ終了)");
    Serial.println("++++++++++++++++++++++++");
    for(int n = 0; n < fcount; n++){
      strcpy(path, "/wav/");
      strcat(path, fname[n]);
      Serial.println(fname[n]);
      File file = sd.open(path);
      file.seek(8);
      file.read(buff, 4);
      buff[4] = 0;
      if(strcmp((char *)buff, "WAVE") != 0){
        Serial.println("Not WAVE File");
        //Serial.println((char *)buff); 
        continue;
      }
      file.seek(search(file, "fmt "));
      file.read(buff, 4);
      int chunk_len = *(int32_t *)buff;
      //Serial.printf("Chunk length:   %dB\n",chunk_len);
      file.read(buff, chunk_len);
      int bitrate = *(uint32_t *)&buff[4];
      int sample = *(uint16_t *)&buff[14];
      Serial.printf("Sample rate:%dHz\n",bitrate);
      Serial.printf("Bit accuracy:%dBit\n",sample);
      int btr192 = 0;
      if(bitrate == 192000){
        bitrate = 96000;
        btr192 = 1;
      }
      I2S.setsample(bitrate);
      switch(sample){
        case 16: count = 2; break;
        case 24: count = 3; break;
        case 32: count = 4; break;
      }
      int pos = search(file, "data");
      //Serial.printf("Data Pos:%d\n", pos);
      file.seek(pos);     
      file.read(buff, 4);
      uint32_t d;
      Serial.print(">");
      while(file.available()){
          file.read(buff, 1536*4);
            for(int n=0, h=0; n < 1536*4; n += count*((h%2)*2+1)){
                d = *(uint32_t *)&buff[n] << (4 - count) * 8;
                I2S.write(d >> 16);
                I2S.write(d);
                if(btr192) h++;
            }
            if(Serial.available()){
                num = Serial.read();
                Serial.printf("%c:", num);
                volume(num);                  
                if(num == 'n'){
                  break;
                }else if(num == 'p'){
                  n -= 2;
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

/* MP3音楽プレーヤ */
void mp3music_play(){
  char path[30];
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
  mp3dec_frame_info_t info;
  mp3dec_t mp3d;
  uint8_t fbuf[0x600];
  int count, twice, half, num;
  SdFatSdio sd;
  
  sd.begin();
  File root = sd.open("/mp3");
  File file = root.openNextFile();
  for(fcount = 0; file; fcount++){
    file.getName(fname[fcount], 30);
    //Serial.println(fname[fcount]);
    file = root.openNextFile();
  }
  Serial.println("音楽プレーヤメニュー");
  Serial.println("++++++++++++++++++++++++");
  Serial.println("音量(u:大きく d:小さく)");
  Serial.println("選曲(n:次曲   p:前曲)");
  Serial.println("終了(e:プレーヤ終了)");
  Serial.println("++++++++++++++++++++++++");
  
  int samples = 1;
  int d_on;
  mp3dec_init(&mp3d);
  
  for(int m_no = 0; m_no < fcount; m_no++){
    d_on = 1;
    strcpy(path, "/mp3/");
    strcat(path, fname[m_no]);
    Serial.println(fname[m_no]);
    File file = sd.open(path);

    int pos = 0;
    int frame = sizeof(fbuf);
    I2S.setsample(44100);
    
    if(!d_on) Serial.print(">");
    for(int fn = 1; file.available(); fn++){
      int pbyte = pos % 4;
      file.seek(pos - pbyte);
      file.read(fbuf, sizeof(fbuf));    
      samples = mp3dec_decode_frame(&mp3d, fbuf+pbyte, sizeof(fbuf), pcm, &info);
      pos += info.frame_bytes;
      frame = info.frame_bytes;
      uint32_t d;
      for(int n=0; n < 1152*2; n++){
        d = *(uint32_t *)&pcm[n] << 16;
        I2S.write(d >> 16);
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
        d_on = 0;
        Serial.print(">");
      }
      if(Serial.available()){
        num = Serial.read();
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

void temp_calc(){
  static int tmp_s = 0, measure_count = 0;
  int tmp;
  uint8_t buf[2];
  
  i2c_mem_write(0x30, 107, 0x10);
  i2c_mem_read(2, 107, 0x20, buf);
  tmp_s += (buf[1]*256+buf[0])/128;
  measure_count += 1;
  if(measure_count == 5){
    Serial.printf("TEMP:%2d℃\n", tmp_s/5);
    measure_count = 0;
    tmp_s = 0;    
  }
}

/* ATコマンド出力 */
void esp_flash(){
  int data1, data2;
  int f1 = 0, f2 = 0;
 
  Serial2.begin(115200);
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

uint8_t codec_writeReg(uint8_t reg, uint16_t data){
  uint8_t error;
  
  Wire.beginTransmission(CODEC_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(data >> 8));
  Wire.write((uint8_t)(data & 0xff));
  error = Wire.endTransmission();
  return error;
}

uint16_t codec_readReg(uint8_t reg){
  uint8_t error;
  uint16_t data;
  
  Wire.beginTransmission(CODEC_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(CODEC_ADDR, 2);
  delay(1);
  data = Wire.read();
  data = data << 8 | Wire.read();
  error = Wire.endTransmission();
  return data;
}

void codec_reg_setup(){
  //Serial.printf("%x\n", codec_readReg(0x00));
  codec_writeReg(0x02, 0x00);
  //Serial.println(codec_readReg(0x02));
  codec_writeReg(0x02, 0x03);
  //Serial.println(codec_readReg(0x02));
  codec_writeReg(0x03, 0x1a);
  codec_writeReg(0x06, 0x200 | 300);
  //Serial.println(codec_readReg(0x06));
  codec_writeReg(0x07, 0x200 | 300);
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

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("Start ARM-First Test Program");
  Wire.begin(); // start the I2C driver for codec register setup 
  codec_reg_setup();
  I2S.begin(I2S_PHILIPS_MODE, 44100, 32);
  I2S.setBuffer(buf, 8192*2); 
  }

void loop() {
  menu();
}
