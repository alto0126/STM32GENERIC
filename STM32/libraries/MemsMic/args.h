/* Copyright 2020 CQ Publishing Co.,Ltd. All Rights Reserved.                 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CQ_ARGS
#define __CQ_ARGS

/* define --------------------------------------------------------------------*/
/* Includes ------------------------------------------------------------------*/
/* typedef -------------------------------------------------------------------*/
/* variables -----------------------------------------------------------------*/
/* functions -----------------------------------------------------------------*/
void InitFirFilter(float *delay_ptr, uint16_t coef_len, uint16_t input_len)
{
  uint16_t cnt;

  for(cnt = 0; cnt < (coef_len + input_len - 1); cnt++)
  {
    *delay_ptr++ = 0.0f;
  }
}

void FirFilter(float *input_ptr, float *output_ptr, const float *coef_ptr, float *delay_ptr, uint16_t coef_len, uint16_t input_len, uint16_t deci_factor)
{
  float *delay_tmp1_ptr;
  float *delay_tmp2_ptr;
  const float *coef_tmp_ptr;
  float fir_process;
  uint16_t cnt1;
  uint16_t cnt2;

  delay_tmp1_ptr = delay_ptr;
  delay_tmp2_ptr = delay_ptr + input_len;

  for(cnt1 = 0; cnt1 < (coef_len - 1); cnt1++)
  {
    *delay_tmp1_ptr++ = *delay_tmp2_ptr++;
  }

  for(cnt1 = 0; cnt1 < input_len; cnt1++)
  {
    *delay_tmp1_ptr++ = *input_ptr++;
  }

  for(cnt1 = 0; cnt1 < input_len; cnt1 += deci_factor)
  {
    delay_tmp1_ptr = delay_ptr + cnt1;
    coef_tmp_ptr = coef_ptr;

    fir_process = 0.0f;
    for(cnt2 = 0; cnt2 < coef_len; cnt2++)
    {
      fir_process += *delay_tmp1_ptr++ * *coef_tmp_ptr++;
    }

    *output_ptr++ = fir_process;
  }
}

void InitDcCut(float *delay_ptr)
{
  *delay_ptr = 0.0f;
}

void DcCut(float *input_ptr, float *output_ptr, const float *coef_ptr, float *delay_ptr, uint16_t input_len)
{
  uint16_t cnt;

  for(cnt = 0; cnt < input_len; cnt++)
  {
    *delay_ptr = coef_ptr[0] * *delay_ptr + coef_ptr[1] * *input_ptr;
    *output_ptr++ = *input_ptr++ - *delay_ptr;
  }
}

#endif /* __CQ_ARGS */

