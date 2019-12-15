/***********************************************************************/
/*                                                                     */
/*  FILE        :mp3_player.ino                                                */
/*  DATE        :2019/10/16                                              */
/*  DESCRIPTION :MP3 Player sample PROGRAM                                  */
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

I2SClass I2S(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

void setup() {
  delay(1000);
  Serial.begin(115200);
  Wire.begin(); // start the I2C driver for codec register setup 
  I2S.begin(I2S_PHILIPS_MODE, 44100, 32);
}

void loop() {
/* MP3音楽プレーヤ */
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
  sd.begin();
  I2S.setBuffer(buf, 8192*2);    
  File root = sd.open("/mp3");
  File file = root.openNextFile();
  for(fcount = 0; file; fcount++){
    file.getName(fname[fcount], 50);
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
  
  for(int m_no = 0; m_no < fcount; m_no++){//ファイルリストのファイルを順に再生
    d_on = 1;
    strcpy(path, "/mp3/");
    strcat(path, fname[m_no]);
    Serial.println(fname[m_no]);
    File file = sd.open(path); //指定したmp3フォルダ内のファイルをオープン

    int pos = 0;              //フレームポインタ     
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
