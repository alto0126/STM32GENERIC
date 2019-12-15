
const stm32_af_pin_list_type chip_af_usart_rx [] = {
//UART4
    { UART4 , GPIOA, GPIO_PIN_1  , GPIO_AF8_UART4 }, 
//USART1
    { USART1, GPIOA, GPIO_PIN_10 , GPIO_AF7_USART1}, 
    { USART1, GPIOB, GPIO_PIN_7  , GPIO_AF7_USART1}, 
//USART2
    { USART2, GPIOA, GPIO_PIN_3  ,  }, 
    { USART2, GPIOA, GPIO_PIN_15 , GPIO_AF3_USART2}, 
//USART3
    { USART3, GPIOB, GPIO_PIN_11 , GPIO_AF7_USART3}, 
}; 

const stm32_af_pin_list_type chip_af_usart_tx [] = {
//UART4
    { UART4 , GPIOA, GPIO_PIN_0  , GPIO_AF8_UART4 }, 
//USART1
    { USART1, GPIOA, GPIO_PIN_9  , GPIO_AF7_USART1}, 
    { USART1, GPIOB, GPIO_PIN_6  , GPIO_AF7_USART1}, 
//USART2
    { USART2, GPIOA, GPIO_PIN_2  ,  }, 
//USART3
    { USART3, GPIOB, GPIO_PIN_10 , GPIO_AF7_USART3}, 
}; 
