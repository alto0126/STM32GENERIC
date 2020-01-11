/* Copyright 2020 CQ Publishing Co.,Ltd. All Rights Reserved.                 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CQ_AUDIO_PROCESS
#define __CQ_AUDIO_PROCESS

/* define --------------------------------------------------------------------*/
#define AUDIO_PROCESS_PERIOD    1                                             /* ms */
#define AUDIO_I2S_FORMAT        32                                            /* bit */
#define AUDIO_I2S_LRCK          I2S_AUDIOFREQ_48K                             /* Hz */
#define AUDIO_I2S_BCCK          (AUDIO_I2S_FORMAT * 2 * AUDIO_I2S_LRCK)

#define AUDIO_MONO_BUFF_LEN     (AUDIO_I2S_LRCK / 1000 * AUDIO_PROCESS_PERIOD)
#define AUDIO_MONO_16B_BUFF_LEN (AUDIO_MONO_BUFF_LEN * AUDIO_I2S_FORMAT / 16)
#define AUDIO_ST_BUFF_LEN       (AUDIO_MONO_BUFF_LEN * 2)
#define AUDIO_ST_16B_BUFF_LEN   (AUDIO_ST_BUFF_LEN * AUDIO_I2S_FORMAT / 16)

#define DMA_BUFF_LEN            (AUDIO_ST_BUFF_LEN * 2)

#define PCM_BUFF_PERIOD         200                                           /* PCM_BUFF_PERIOD * AUDIO_PROCESS_PERIOD ms */
#define PCM_BUFF_LEN            (PCM_BUFF_PERIOD * AUDIO_ST_BUFF_LEN)         /* 200 * 48000 / 1000 * 1 * 2 = 19200 */

#define PCM_BUFF_STATE_NOTCPLT  0
#define PCM_BUFF_STATE_HALFCPLT 1
#define PCM_BUFF_STATE_CPLT     2

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "pdm2pcm.h"
#include <math.h>

/* typedef -------------------------------------------------------------------*/
/* variables -----------------------------------------------------------------*/
uint32_t i2s3_input_buff[DMA_BUFF_LEN];

int16_t temp_pcm_buff[AUDIO_ST_BUFF_LEN];
//int16_t pcm_buff[PCM_BUFF_LEN];
int16_t *pcm_buff;

uint8_t pcm_buff_state;
uint16_t pcm_buff_cnt;

/* functions -----------------------------------------------------------------*/
void setpcmbuf(int16_t *buf){
  pcm_buff = buf;
}

void InitProcessBuffer(void)
{
  uint16_t cnt;

  for(cnt = 0; cnt < DMA_BUFF_LEN; cnt++)
  {
    i2s3_input_buff[cnt] = 0;
  }

  for(cnt = 0; cnt < PCM_BUFF_LEN; cnt++)
  {
    pcm_buff[cnt] = 0;
  }
}

void StartAudioProcess(void)
{
  pcm_buff_state = PCM_BUFF_STATE_NOTCPLT;
  pcm_buff_cnt = 0;

  InitProcessBuffer();

  InitMic12();

  /* SN74LVC1G74 _CLR Low */
  pinMode(PB10, OUTPUT);
  digitalWrite(PB10, LOW);

  MX_DMA_Init();
  MX_I2S3_Init();
  HAL_I2S_MspInit(&hi2s3);

  /* SN74LVC1G74 _CLR High */
  digitalWrite(PB10, HIGH);

  HAL_I2S_Receive_DMA(&hi2s3, (uint16_t *)i2s3_input_buff, DMA_BUFF_LEN);
}

void FinishAudioProcess(void)
{
  HAL_I2S_DMAStop(&hi2s3);
}

void UpdatePcmBuffState(void)
{
  uint16_t cnt;

  for(cnt = 0; cnt < AUDIO_ST_BUFF_LEN; cnt++)
  {
    pcm_buff[pcm_buff_cnt * AUDIO_ST_BUFF_LEN + cnt] = temp_pcm_buff[cnt];
  }

  pcm_buff_cnt++;
  if(pcm_buff_cnt == PCM_BUFF_PERIOD/2)
  {
    pcm_buff_state = PCM_BUFF_STATE_HALFCPLT;
  }
  else if(pcm_buff_cnt == PCM_BUFF_PERIOD)
  {
    pcm_buff_state = PCM_BUFF_STATE_CPLT;
    pcm_buff_cnt = 0;
  }
  else
  {
    ;
  }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *i2sHandle)
{
  if(i2sHandle->Instance == SPI3)
  {
    Pdm2PcmM1M2(&i2s3_input_buff[AUDIO_ST_BUFF_LEN], &temp_pcm_buff[0]);
    UpdatePcmBuffState();
  }
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *i2sHandle)
{
  if(i2sHandle->Instance == SPI3)
  {
    Pdm2PcmM1M2(&i2s3_input_buff[0], &temp_pcm_buff[0]);
    UpdatePcmBuffState();
  }
}

#endif /* __CQ_AUDIO_PROCESS */

