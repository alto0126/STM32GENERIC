
const stm32_af_pin_list_type chip_af_usart_rx [] = {
//UART4
    { UART4 , GPIOA, GPIO_PIN_1  , GPIO_AF8_UART4 }, 
    { UART4 , GPIOC, GPIO_PIN_11 , GPIO_AF8_UART4 }, 
//UART5
    { UART5 , GPIOD, GPIO_PIN_2  , GPIO_AF8_UART5 }, 
//USART1
    { USART1, GPIOA, GPIO_PIN_10 , GPIO_AF7_USART1}, 
    { USART1, GPIOB, GPIO_PIN_7  , GPIO_AF7_USART1}, 
//USART2
    { USART2, GPIOA, GPIO_PIN_3  ,  }, 
    { USART2, GPIOD, GPIO_PIN_6  ,  }, 
//USART3
    { USART3, GPIOB, GPIO_PIN_11 , GPIO_AF7_USART3}, 
    { USART3, GPIOC, GPIO_PIN_11 , GPIO_AF7_USART3}, 
    { USART3, GPIOD, GPIO_PIN_9  , GPIO_AF7_USART3}, 
//USART6
    { USART6, GPIOC, GPIO_PIN_7  , GPIO_AF8_USART6}, 
    { USART6, GPIOG, GPIO_PIN_9  , GPIO_AF8_USART6}, 
}; 

const stm32_af_pin_list_type chip_af_usart_tx [] = {
//UART4
    { UART4 , GPIOA, GPIO_PIN_0  , GPIO_AF8_UART4 }, 
    { UART4 , GPIOC, GPIO_PIN_10 , GPIO_AF8_UART4 }, 
//UART5
    { UART5 , GPIOC, GPIO_PIN_12 , GPIO_AF8_UART5 }, 
//USART1
    { USART1, GPIOA, GPIO_PIN_9  , GPIO_AF7_USART1}, 
    { USART1, GPIOB, GPIO_PIN_6  , GPIO_AF7_USART1}, 
//USART2
    { USART2, GPIOA, GPIO_PIN_2  ,  }, 
    { USART2, GPIOD, GPIO_PIN_5  ,  }, 
//USART3
    { USART3, GPIOB, GPIO_PIN_10 , GPIO_AF7_USART3}, 
    { USART3, GPIOC, GPIO_PIN_10 , GPIO_AF7_USART3}, 
    { USART3, GPIOD, GPIO_PIN_8  , GPIO_AF7_USART3}, 
//USART6
    { USART6, GPIOC, GPIO_PIN_6  , GPIO_AF8_USART6}, 
    { USART6, GPIOG, GPIO_PIN_14 , GPIO_AF8_USART6}, 
}; 
