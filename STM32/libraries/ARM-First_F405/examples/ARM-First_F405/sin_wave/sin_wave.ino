/***********************************************************************/
/*                                                                     */
/*  FILE        :sin_wave.ino                                                */
/*  DATE        :2019/10/16                                             */
/*  DESCRIPTION :Sin Wave Generator sample                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/
#define CODEC_ADDR 31 // DAC i2c slave address

#include <Wire.h>   //I2Cライブラリのヘッダファイル
#include "I2S.h"    //I2Sライブラリのヘッダファイル
#include "math.h"

void codec_reg_setup();
void getstr(char *buf);
I2SClass I2S(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

void setup() {
  Serial.begin(115200);
  Wire.begin(); // start the I2C driver for codec register setup
  I2S.setsample(44100);
  codec_reg_setup(); 
}

void loop() {
/* サイン波出力　*/
  double frequency;
  double amplitude = 30000;
  char buf[8];
  int16_t buff[4410*2];
  
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
        I2S.write(buff[n]);   //left MSB
        I2S.write(0);         //left LSB
        I2S.write(buff[n]);   //right MSB
        I2S.write(0);         //right LSB
      }
      if(Serial.available()) break;
    }
    Serial.read();
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
