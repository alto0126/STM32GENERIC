#define MINIMP3_ONLY_MP3
//#define MINIMP3_ONLY_SIMD
#define MINIMP3_NO_SIMD
//#define MINIMP3_NONSTANDARD_BUT_LOGICAL
//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_MAX_SAMPLES_PER_FRAME 1152*2
#define CODEC_ADDR 0x1F // i2c address
#include "minimp3.h"
#include "SdFat.h"
#include <Wire.h>
#include "I2S.h"

/*typedef struct
{
    int frame_bytes;
    int channels;
    int hz;
    int layer;
    int bitrate_kbps;
} mp3dec_frame_info_t;*/

mp3dec_frame_info_t info;
short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
uint8_t buf[0x600];
int16_t buff[8192*2];
static mp3dec_t mp3d;
SdFatSdio sd;
I2SClass I2S = I2SClass(SPI2, PB15 /*DIN*/ , PB9 /*LRC*/, PB13 /*SCLK*/, PC6 /*MCLK*/);

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
  codec_writeReg(0x03, 0x02);
  codec_writeReg(0x06, 0x320);
  codec_writeReg(0x07, 0x320);
}

char fname[100][30];
int fcount;

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("Start ARM-First MP3 Test Program");
  Wire.begin(); // start the I2C driver for codec register setup
  codec_reg_setup();
  I2S.begin(I2S_PHILIPS_MODE, 44100, 32);
  I2S.setBuffer(buff, 8192*2);
  
  sd.begin();
  File root = sd.open("/mp3");
  File file = root.openNextFile();
  for(fcount = 0; file; fcount++){
    file.getName(fname[fcount], 30);
    Serial.println(fname[fcount]);
    file = root.openNextFile();
  } 
}

void loop() {
  char path[30];
  int count, twice, half;
  int samples = 1;
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
  mp3dec_init(&mp3d);
  int d_on;
    
  Serial.println("Start Music");
  for(int m_no = 0; m_no < fcount; m_no++){
    d_on = 1;
    strcpy(path, "/mp3/");
    strcat(path, fname[ m_no]);
    Serial.println(fname[ m_no]);
    File file = sd.open(path);

    int pos = 0;
    int frame = sizeof(buf);
    mp3dec_init(&mp3d);  
    for(int fn = 1; file.available(); fn++){
      int pbyte = pos % 4;
      file.seek(pos - pbyte);
      file.read(buf, sizeof(buf));    
      samples = mp3dec_decode_frame(&mp3d, buf+pbyte, sizeof(buf), pcm, &info);
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
        //Serial.printf("%d-samples:%d\n", fn,samples);
        Serial.printf("channels:%d\n", info.channels);
        Serial.printf("frame:%dbyte\n", info.frame_bytes);
        Serial.printf("bitrate:%dkbps\n", info.bitrate_kbps);
        d_on = 0;
      }
    }
    file.close();
  }
}
