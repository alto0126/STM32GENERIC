/***********************************************************************/
/*                                                                     */
/*  FILE        :wave_player.ino                                                */
/*  DATE        :2019/10/16                                              */
/*  DESCRIPTION :Wave Player sample PROGRAM                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/
#define CODEC_ADDR 31 // DAC i2c slave address
#define WAV_HEADER_SIZE   44

#include <Wire.h>   //I2Cライブラリのヘッダファイル
#include "I2S.h"    //I2Sライブラリのヘッダファイル
#include "SdFat.h"  //MicroSDのヘッダファイル

int search(File file, String str);
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
/* WAV音楽プレーヤ */
  int16_t buf[8192*2];      //DMAリングバッファ
  uint8_t buff[1536*4 + 4]; //SDカード読出しバッファ
  char path[30];            //フォルダ名（wav）
  char fname[100][50];      //ファイル名リスト
  int count, num, fcount;
  SdFatSdio sd;             //sdioオブジェクト生成
  codec_reg_setup();        //DACの初期化
  sd.begin();               //sdカードの初期化
  I2S.setBuffer(buf, 8192*2);    //DMAバッファの設定
  File root = sd.open("/wav");      //wavフォルダをオープン
  File file = root.openNextFile();  //wavフォルダ内のファイルをオープン
  for(fcount = 0; file; fcount++){  //全てのファイル名が取得できるまで繰り返し
    file.getName(fname[fcount], 50);    //オープンできたファイル名を取得
    //Serial.println(fname[fcount]);
    file = root.openNextFile();     //次のファイルをオープン
  }
    Serial.println("音楽プレーヤメニュー");       //サブメニューのコンソールへの表示
    Serial.println("++++++++++++++++++++++++");
    Serial.println("音量(u:大きく d:小さく)");
    Serial.println("選曲(n:次曲   p:前曲)");
    Serial.println("終了(e:プレーヤ終了)");
    Serial.println("++++++++++++++++++++++++");
    for(int n = 0; n < fcount; n++){    //取得したファイルを順に再生
      strcpy(path, "/wav/");
      strcat(path, fname[n]);
      Serial.println(fname[n]);
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
            if(Serial.available()){             //コンソールから指示があるかチェック
                num = Serial.read();            //指示を読み出し
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

