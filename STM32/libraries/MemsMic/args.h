#ifndef ARGS_H
#define ARGS_H

#include <math.h>

typedef struct
{
	float freq;
	float phase;
}singen_t;

#define PI2							(2.0f * 3.14159265358979f)

void InitSinGen(singen_t *st_ptr, float freq, float fs)
{
	st_ptr->freq = PI2 * freq / fs;
	st_ptr->phase = 0.0f;
}

void SinGen(singen_t *st_ptr, float *output_ptr, uint16_t output_len)
{
	uint16_t cnt;

	for(cnt = 0; cnt < output_len; cnt++)
	{
		*output_ptr++ = sinf(st_ptr->phase);
		st_ptr->phase += st_ptr->freq;

		if(st_ptr->phase > PI2)
		{
			st_ptr->phase -= PI2;
		}
	}
}

void InitFirFilter(float *delay_ptr, uint16_t coef_len, uint16_t input_len)
{
	uint16_t cnt;

	for(cnt = 0; cnt < (coef_len + input_len - 1); cnt++)
	{
		*delay_ptr++ = 0.0f;
	}
}

void FirFilter(float *input_ptr, float *output_ptr, const float *coef_ptr, float *delay_ptr, uint16_t coef_len, uint16_t input_len)
{
	uint16_t i_cnt;
	uint16_t o_cnt;
	float *delay_tmp1_ptr;
	float *delay_tmp2_ptr;
	const float *coef_tmp_ptr;
	float fir_process;

	delay_tmp1_ptr = delay_ptr;
	delay_tmp2_ptr = delay_ptr + input_len;

	for(i_cnt = 0; i_cnt < (coef_len - 1); i_cnt++)
	{
		*delay_tmp1_ptr++ = *delay_tmp2_ptr++;
	}

	for(i_cnt = 0; i_cnt < input_len; i_cnt++)
	{
		*delay_tmp1_ptr++ = *input_ptr++;
	}

	for(i_cnt = 0; i_cnt < input_len; i_cnt++)
	{
		delay_tmp1_ptr = delay_ptr + i_cnt;
		coef_tmp_ptr = coef_ptr;

		fir_process = 0.0f;
		for(o_cnt = 0; o_cnt < coef_len; o_cnt++)
		{
			fir_process += *delay_tmp1_ptr++ * *coef_tmp_ptr++;
		}

		*output_ptr++ = fir_process;
	}
}

void InitDccut(float *delay_ptr)
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

#endif /* ARGS_H */

