#ifndef AUDIO_PROCESS_H
#define AUDIO_PROCESS_H

#define AUDIO_PROCESS_PERIOD	1										/* ms */
#define AUDIO_I2S_FORMAT		32										/* bit */
#define AUDIO_I2S_LRCK			I2S_AUDIOFREQ_48K						/* Hz */
#define AUDIO_I2S_BCCK			(AUDIO_I2S_FORMAT*2*AUDIO_I2S_LRCK)		/* Hz */

#define AUDIO_MONO_BUFF_LEN		(AUDIO_I2S_LRCK/1000*AUDIO_PROCESS_PERIOD)
#define AUDIO_MONO_16B_BUFF_LEN	(AUDIO_MONO_BUFF_LEN*AUDIO_I2S_FORMAT/16)
#define AUDIO_ST_BUFF_LEN		(AUDIO_MONO_BUFF_LEN*2)
#define AUDIO_ST_16B_BUFF_LEN	(AUDIO_ST_BUFF_LEN*AUDIO_I2S_FORMAT/16)

#define PCM_BUFF_PERIOD			200										/* PCM_BUFF_PERIOD * AUDIO_PROCESS_PERIOD ms */

#define PCM_BUFF_STATE_NOTCPLT	0
#define PCM_BUFF_STATE_HALFCPLT	1
#define PCM_BUFF_STATE_CPLT		2

#define CH_SETTING_M1M2			0
#define CH_SETTING_M3M4			1

#include "stm32f4xx_hal.h"
#include "pdm2pcm.h"
#include <math.h>

uint32_t i2s2_input_buff[AUDIO_ST_BUFF_LEN*2];
uint32_t i2s3_input_buff[AUDIO_ST_BUFF_LEN*2];

int16_t temp_pcm_buff[AUDIO_ST_BUFF_LEN];
//int16_t pcm_buff[PCM_BUFF_PERIOD*AUDIO_ST_BUFF_LEN];	/* 200*48000/1000*1*2 = 19200 */
int16_t *pcm_buff;

uint8_t use_mic;
uint8_t pcm_buff_state;
uint16_t pcm_buff_cnt;

void setpcmbuf(int16_t *buf){
    pcm_buff = buf;
}

void InitProcessBuffer(void)
{
	uint16_t cnt;

	for(cnt = 0; cnt < (AUDIO_ST_BUFF_LEN*2); cnt++)
	{
		i2s2_input_buff[cnt] = 0;
		i2s3_input_buff[cnt] = 0;
	}

	for(cnt = 0; cnt < (PCM_BUFF_PERIOD*AUDIO_ST_BUFF_LEN); cnt++)
	{
		pcm_buff[cnt] = 0;
	}
}

void StartAudioProcess(uint8_t ch_setting)
{
	if(ch_setting == CH_SETTING_M1M2)
	{
		use_mic = CH_SETTING_M1M2;
	}
	else
	{
		use_mic = CH_SETTING_M3M4;
	}

	pcm_buff_state = PCM_BUFF_STATE_NOTCPLT;
	pcm_buff_cnt = 0;

	InitProcessBuffer();

	InitMic12();
	InitMic34();

  pinMode(PB10, OUTPUT);  /* SN74LVC1G74 _CLR Control port */
  digitalWrite(PB10, LOW);  /* SN74LVC1G74 _CLR Low */

	MX_DMA_Init();
	MX_I2S3_Init();
	MX_I2S2_Init();
	HAL_I2S_MspInit(&hi2s3);
	HAL_I2S_MspInit(&hi2s2);

  digitalWrite(PB10, HIGH);  /* SN74LVC1G74 _CLR High */

	HAL_I2S_Receive_DMA(&hi2s3, (uint16_t *)i2s3_input_buff, (AUDIO_ST_BUFF_LEN*2));
	HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)i2s2_input_buff, (AUDIO_ST_BUFF_LEN*2));
}

void FinishAudioProcess(void)
{
  HAL_I2S_DMAStop(&hi2s2);
  HAL_I2S_DMAStop(&hi2s3);

  HAL_I2S_MspDeInit(&hi2s2);
  HAL_I2S_MspDeInit(&hi2s3);

  MX_DMA_DeInit();
  HAL_I2S_DeInit(&hi2s2);
  HAL_I2S_DeInit(&hi2s3);

  digitalWrite(PB10, LOW);  /* SN74LVC1G74 _CLR Low */

  use_mic = CH_SETTING_M1M2;
  pcm_buff_state = PCM_BUFF_STATE_NOTCPLT;
  pcm_buff_cnt = 0;
}

void UpdatePcmBuffState(void)
{
	uint16_t cnt;

	for(cnt = 0; cnt < AUDIO_ST_BUFF_LEN; cnt++)
	{
		pcm_buff[pcm_buff_cnt*AUDIO_ST_BUFF_LEN+cnt] = temp_pcm_buff[cnt];
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
	if(i2sHandle->Instance == SPI2)
	{
		if(use_mic == CH_SETTING_M3M4)
		{
			Pdm2PcmM3M4(&i2s2_input_buff[AUDIO_ST_BUFF_LEN], &temp_pcm_buff[0]);
			UpdatePcmBuffState();
		}
	}
	else if(i2sHandle->Instance == SPI3)
	{
		if(use_mic == CH_SETTING_M1M2)
		{
			Pdm2PcmM1M2(&i2s3_input_buff[AUDIO_ST_BUFF_LEN], &temp_pcm_buff[0]);
			UpdatePcmBuffState();
		}
	}
	else
	{
		;
	}
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *i2sHandle)
{
	if(i2sHandle->Instance == SPI2)
	{
		if(use_mic == CH_SETTING_M3M4)
		{
			Pdm2PcmM3M4(&i2s2_input_buff[0], &temp_pcm_buff[0]);
			UpdatePcmBuffState();
		}
	}
	else if(i2sHandle->Instance == SPI3)
	{
		if(use_mic == CH_SETTING_M1M2)
		{
			Pdm2PcmM1M2(&i2s3_input_buff[0], &temp_pcm_buff[0]);
			UpdatePcmBuffState();
		}
	}
	else
	{
		;
	}
}

#endif /* AUDIO_PROCESS_H */

