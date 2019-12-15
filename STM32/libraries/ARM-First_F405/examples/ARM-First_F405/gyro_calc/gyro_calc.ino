/***********************************************************************/
/*                                                                     */
/*  FILE        :gyro_calc.ino                                                */
/*  DATE        :2019/10/16                                             */
/*  DESCRIPTION :Gyro sensor sample                                  */
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
/* ジャイロセンサデータ取得 */
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
