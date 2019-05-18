/*
  I2S sine wave

  This example produces a digital sine wave on the I2S interface of the
  microcontroller.

  Hardware:
   microcontroller board: STM32F407VE Black
   amplifier:             Adafruit I2S amplifier with the MAX98357a chip
                          https://www.adafruit.com/product/3006

  Pin description

  sd  = DIN  // I2S data
  ws  = LRC  // left/right channel
  ck  = SCLK // serial data clock
  mck = MCLK // Master clock

  MAY 2017 ChrisMicro
  Modified MAY 2019 Alto 

*/
#include <Wire.h>
#include "I2S.h"
#include "SdFat.h"

SdFatSdio sd;
uint8_t buff[1536*4];
int16_t buf[8192*2];

// SPI2 pins are used for I2S.
// SPI2 is avalailable on the BalckF407VE and does not conflict with peripherals like SDIO
//I2SClass I2S(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);
I2SClass I2S = I2SClass(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

#define CODEC_ADDR 0x1F // i2c address

uint8_t codec_writeReg(uint8_t reg, uint16_t data)
{
  uint8_t error;
  
  Wire.beginTransmission(CODEC_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(data >> 8));
  Wire.write((uint8_t)(data & 0xff));
  error = Wire.endTransmission();
  return error;
}

uint16_t codec_readReg(uint8_t reg)
{
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

void codec_reg_setup(void)
{
  Serial.printf("%x", codec_readReg(0x00));
  codec_writeReg(0x02, 0x00);
  Serial.println(codec_readReg(0x02));
  codec_writeReg(0x02, 0x03);
  Serial.println(codec_readReg(0x02));
  codec_writeReg(0x03, 0x1a);
  codec_writeReg(0x06, 0x350);
  codec_writeReg(0x07, 0x350);
}

int search(File file, String str="abc"){
  int n, m;  
  Serial.println(str); 
  file.seek(0);
  for(n = 0, m = 0; m < str.length(); n++){
    if(str[m] == file.read()) m++;
  }
  return n;
}

char fname[100][30];
int fcount;

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("Start");
  Wire.begin(); // start the I2C driver for codec register setup
  codec_reg_setup();
  I2S.begin(I2S_PHILIPS_MODE, 96000, 32);
  I2S.setBuffer(buf, 8192*2);
  Serial.printf("%d\n",I2S.getBufferSize()); 
  
  sd.begin();
  File root = sd.open("/hr");
  File file = root.openNextFile();
  for(fcount = 0; file; fcount++){
    file.getName(fname[fcount], 30);
    Serial.println(fname[fcount]);
    file = root.openNextFile();
  }
}

void loop()
{
    char path[30];
    int count, twice, half;
    
    Serial.println("Start Music");
    for(int n = 0; n < fcount; n++){
      strcpy(path, "/hr/");
      strcat(path, fname[n]);
      Serial.println(fname[n]);
      File file = sd.open(path);
      file.seek(8);
      file.read(buff, 4);
      buff[4] = 0;
      if(strcmp((char *)buff, "WAVE") != 0){
        Serial.println("Not WAVE File");
        Serial.println((char *)buff); 
        continue;
      }
      file.seek(search(file, "fmt "));
      file.read(buff, 4);
      int chunk_len = *(int32_t *)buff;
      Serial.printf("Chunk length:   %dB\n",chunk_len);
      file.read(buff, chunk_len);
      int bitrate = *(uint32_t *)&buff[4];
      int sample = *(uint16_t *)&buff[14];
      Serial.printf("Bit Rate:   %dHz\n",bitrate);
      Serial.printf("Bit Density:%dBit\n",sample);
      count = sample / 8;
      int pos = search(file, "data");
      Serial.printf("Data Pos:%d\n", pos);
      file.seek(pos);     
      file.read(buff, 4);
      uint32_t d;
      while(file.available()){
          file.read(buff, 1536*4);
            for(int n=0, h=0; n < 1536*4; n += count*((h%2)*2+1), h++){
                d = *(uint32_t *)&buff[n] << (4 - count) * 8;
                I2S.write(d >> 16);
                I2S.write(d);
            }
      }
      file.close();
      Serial.println("Done");
    }
    delay(2000);
}
