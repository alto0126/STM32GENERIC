/***********************************************************************/
/*                                                                     */
/*  FILE        :press_calc.ino                                                */
/*  DATE        :2019/10/16                                             */
/*  DESCRIPTION :Press Temp sensor sample                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/
#define CODEC_ADDR 31 // DAC i2c slave address
#define PRESS_ADR 93
#define ACC_ADR 107

#include <Wire.h>   //I2Cライブラリのヘッダファイル

uint8_t i2c_mem_write(uint8_t reg, uint8_t addr, uint8_t data);
uint8_t i2c_mem_read(uint8_t count, uint8_t addr, uint8_t reg, uint8_t * buf);

void setup() {
  Serial.begin(115200);
  Wire.begin(); // start the I2C driver for codec register setup 
  }

void loop() {
/* 気圧温度データの取得 */
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
  delay(100);
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
