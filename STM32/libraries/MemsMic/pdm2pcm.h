/* Copyright 2020 CQ Publishing Co.,Ltd. All Rights Reserved.                 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CQ_PDM2PCM
#define __CQ_PDM2PCM

/* define --------------------------------------------------------------------*/
#define PDM_CLK                 (AUDIO_I2S_BCCK / 2)

#define PDM2PCM_CIC_R           32
#define PDM2PCM_CIC_N           3
#define PDM2PCM_CIC_DELAY_LEN   (PDM2PCM_CIC_N * 2)

#define PDM2PCM_LPF_COEF_LEN    12
#define PDM2PCM_LPF_DELAY_LEN   (PDM2PCM_LPF_COEF_LEN + AUDIO_MONO_BUFF_LEN - 1)

#define PDM2PCM_DCCUT_COEFB     0.99f
#define PDM2PCM_DCCUT_COEFA     (1.0f - PDM2PCM_DCCUT_COEFB)

#define OUTPUT_GAIN             100.0f                                  /* +40dB */

/* Includes ------------------------------------------------------------------*/
/* typedef -------------------------------------------------------------------*/
typedef struct
{
  uint32_t cic_delay[PDM2PCM_CIC_DELAY_LEN];
  float cic_norm_gain;
  float lpf_delay[PDM2PCM_LPF_DELAY_LEN];
  float dccut_delay;
}pdm2pcm_t;

typedef struct
{
  pdm2pcm_t lch;
  pdm2pcm_t rch;
}pdm2pcm_st_t;

/* variables -----------------------------------------------------------------*/
const float pdm2pcm_lpf_coef[PDM2PCM_LPF_COEF_LEN] =
{
   0.0077880110890349748,
  -0.0043650639698822272,
  -0.050381202041919641,
  -0.028608884750919869,
   0.16785130307859031,
   0.40063478900964855,
   0.40063478900964855,
   0.16785130307859031,
  -0.028608884750919869,
  -0.050381202041919641,
  -0.0043650639698822272,
   0.0077880110890349748
};

const float pdm2pcm_dccut_coef[2] =
{
  PDM2PCM_DCCUT_COEFB,
  PDM2PCM_DCCUT_COEFA
};

pdm2pcm_st_t m1m2;

/* functions -----------------------------------------------------------------*/
void InitPdm2Pcm(pdm2pcm_t *st_ptr)
{
  uint16_t cnt;

  st_ptr->cic_norm_gain = 1.0f / (powf(2.0f, (float)PDM2PCM_CIC_N * log2f((float)PDM2PCM_CIC_R) + 1.0f));

  for(cnt = 0; cnt < PDM2PCM_CIC_DELAY_LEN; cnt++)
  {
    st_ptr->cic_delay[cnt] = 0;
  }

  InitFirFilter(&st_ptr->lpf_delay[0], PDM2PCM_LPF_COEF_LEN, AUDIO_MONO_BUFF_LEN);

  InitDcCut(&st_ptr->dccut_delay);
}

void InitPdm2PcmSt(pdm2pcm_st_t *st_ptr)
{
  InitPdm2Pcm(&st_ptr->lch);
  InitPdm2Pcm(&st_ptr->rch);
}

void InitMic12(void)
{
  InitPdm2PcmSt(&m1m2);
}

void PdmCicFilter(pdm2pcm_t *st_ptr, uint16_t *pdm_input_ptr, float *pcm_output_ptr, uint16_t input_len)
{
  uint16_t input_cnt;
  uint16_t bit_cnt;
  uint16_t cic_cnt;
  uint32_t pdm_temp;
  uint32_t cic_process;
  uint32_t cic_temp;

  for(input_cnt = 0; input_cnt < input_len; input_cnt++)
  {
    pdm_temp = (uint32_t)*pdm_input_ptr++;

    for(bit_cnt = 0; bit_cnt < 16; bit_cnt++)
    {
      cic_process = (pdm_temp & (uint32_t)0x8000) >> 15;
      pdm_temp = pdm_temp << 1;

      for(cic_cnt = 0; cic_cnt < PDM2PCM_CIC_N; cic_cnt++)
      {
        cic_process = cic_process + st_ptr->cic_delay[cic_cnt];
        st_ptr->cic_delay[cic_cnt] = cic_process;
      }
    }

    if(((input_cnt + 1) % (PDM2PCM_CIC_R / 16)) == 0)
    {
      cic_temp = cic_process;
      for(cic_cnt = 0; cic_cnt < PDM2PCM_CIC_N; cic_cnt++)
      {
        cic_process = cic_process - st_ptr->cic_delay[cic_cnt + PDM2PCM_CIC_N];
        st_ptr->cic_delay[cic_cnt + PDM2PCM_CIC_N] = cic_temp;
        cic_temp = cic_process;
      }

      *pcm_output_ptr++ = (float)cic_process * st_ptr->cic_norm_gain * OUTPUT_GAIN;
    }
  }
}

void Pdm2PcmSt(pdm2pcm_st_t *st_ptr, uint32_t *pdm_input_ptr, int16_t *pcm_output_ptr)
{
  uint16_t lch_pdm_buff[AUDIO_MONO_16B_BUFF_LEN];
  uint16_t rch_pdm_buff[AUDIO_MONO_16B_BUFF_LEN];
  float process_buff1[AUDIO_MONO_BUFF_LEN];
  float process_buff2[AUDIO_MONO_BUFF_LEN];
  uint16_t h16_mux_pdm;
  uint16_t l16_mux_pdm;
  uint16_t input_cnt;
  uint16_t bit_cnt;
  uint16_t output_cnt;

  for(input_cnt = 0; input_cnt < AUDIO_ST_BUFF_LEN; input_cnt++)
  {
    h16_mux_pdm = (uint16_t)(((uint32_t)0xFFFF0000 & *pdm_input_ptr) >> 16);
    l16_mux_pdm = (uint16_t)((uint32_t)0x0000FFFF & *pdm_input_ptr++);

    lch_pdm_buff[input_cnt] = 0;
    rch_pdm_buff[input_cnt] = 0;

    for(bit_cnt = 0; bit_cnt < 8; bit_cnt++)
    {
      lch_pdm_buff[input_cnt] = (lch_pdm_buff[input_cnt] << 1) | ((l16_mux_pdm & 0x8000) >> 15);
      l16_mux_pdm = l16_mux_pdm << 1;

      rch_pdm_buff[input_cnt] = (rch_pdm_buff[input_cnt] << 1) | ((l16_mux_pdm & 0x8000) >> 15);
      l16_mux_pdm = l16_mux_pdm << 1;
    }

    for(bit_cnt = 0; bit_cnt < 8; bit_cnt++)
    {
      lch_pdm_buff[input_cnt] = (lch_pdm_buff[input_cnt] << 1) | ((h16_mux_pdm & 0x8000) >> 15);
      h16_mux_pdm = h16_mux_pdm << 1;

      rch_pdm_buff[input_cnt] = (rch_pdm_buff[input_cnt] << 1) | ((h16_mux_pdm & 0x8000) >> 15);
      h16_mux_pdm = h16_mux_pdm << 1;
    }
  }

  PdmCicFilter(&st_ptr->lch, &lch_pdm_buff[0], &process_buff1[0], AUDIO_MONO_16B_BUFF_LEN);
  FirFilter(&process_buff1[0], &process_buff2[0], &pdm2pcm_lpf_coef[0], &st_ptr->lch.lpf_delay[0], PDM2PCM_LPF_COEF_LEN, AUDIO_MONO_BUFF_LEN, 1);
  DcCut(&process_buff2[0], &process_buff1[0], &pdm2pcm_dccut_coef[0], &st_ptr->lch.dccut_delay, AUDIO_MONO_BUFF_LEN);

  for(output_cnt = 0; output_cnt < AUDIO_MONO_BUFF_LEN; output_cnt++)
  {
    pcm_output_ptr[2 * output_cnt] = (int16_t)(process_buff1[output_cnt] * 32738.0f);
  }

  PdmCicFilter(&st_ptr->rch, &rch_pdm_buff[0], &process_buff1[0], AUDIO_MONO_16B_BUFF_LEN);
  FirFilter(&process_buff1[0], &process_buff2[0], &pdm2pcm_lpf_coef[0], &st_ptr->rch.lpf_delay[0], PDM2PCM_LPF_COEF_LEN, AUDIO_MONO_BUFF_LEN, 1);
  DcCut(&process_buff2[0], &process_buff1[0], &pdm2pcm_dccut_coef[0], &st_ptr->rch.dccut_delay, AUDIO_MONO_BUFF_LEN);

  for(output_cnt = 0; output_cnt < AUDIO_MONO_BUFF_LEN; output_cnt++)
  {
    pcm_output_ptr[2 * output_cnt + 1] = (int16_t)(process_buff1[output_cnt] * 32738.0f);
  }
}

void Pdm2PcmM1M2(uint32_t *pdm_input_ptr, int16_t *pcm_output_ptr)
{
  Pdm2PcmSt(&m1m2, pdm_input_ptr, pcm_output_ptr);
}

#endif /* __CQ_PDM2PCM */

