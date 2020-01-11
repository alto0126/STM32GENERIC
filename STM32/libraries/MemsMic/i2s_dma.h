/* Copyright 2020 CQ Publishing Co.,Ltd. All Rights Reserved.                 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CQ_I2S_DMA
#define __CQ_I2S_DMA

/* define --------------------------------------------------------------------*/
//#define ENABLE_I2S3_WS                                                        /* WS出力を有効にする */

/* Includes ------------------------------------------------------------------*/
/* typedef -------------------------------------------------------------------*/
/* variables -----------------------------------------------------------------*/
I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_rx;

extern DMA_HandleTypeDef *dmaHandles[];
DMA_HandleTypeDef *dma1_stream0;

/* functions -----------------------------------------------------------------*/
void Error_Handler(void)
{
  while(1)
  {
    ;
  }
}

void MX_DMA_Init(void) 
{
  dma1_stream0 = dmaHandles[0];
  dmaHandles[0] = &hdma_spi3_rx;

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

void MX_I2S3_Init(void)
{
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s3.Init.Standard = I2S_STANDARD_MSB;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_32B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

  if(HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_I2S_MspInit(I2S_HandleTypeDef* i2sHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if(i2sHandle->Instance==SPI3)
  {
    /* I2S3 clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();

#ifdef ENABLE_I2S3_WS
    __HAL_RCC_GPIOA_CLK_ENABLE();
#endif  /* ENABLE_I2S3_WS */

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /**I2S3 GPIO Configuration
    PA4     ------> I2S3_WS
    PB3     ------> I2S3_CK
    PB5     ------> I2S3_SD
    */
#ifdef ENABLE_I2S3_WS
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif  /* ENABLE_I2S3_WS */

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2S3 DMA Init */
    /* SPI3_RX Init */
    hdma_spi3_rx.Instance = DMA1_Stream0;
    hdma_spi3_rx.Init.Channel = DMA_CHANNEL_0;
    hdma_spi3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi3_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi3_rx.Init.Mode = DMA_CIRCULAR;
    hdma_spi3_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_spi3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if(HAL_DMA_Init(&hdma_spi3_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2sHandle,hdmarx,hdma_spi3_rx);
  }
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef* i2sHandle)
{
  if(i2sHandle->Instance==SPI3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();

    /**I2S3 GPIO Configuration
    PA4     ------> I2S3_WS
    PB3     ------> I2S3_CK
    PB5     ------> I2S3_SD
    */
#ifdef ENABLE_I2S3_WS
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);
#endif  /* ENABLE_I2S3_WS */

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_5);

    /* I2S3 DMA DeInit */
    HAL_DMA_DeInit(i2sHandle->hdmarx);
  }
}

#endif /* __CQ_I2S_DMA */

