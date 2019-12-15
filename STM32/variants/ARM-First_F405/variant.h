#ifndef VARIANT_H
#define VARIANT_H

//On-board LED pin number           PIN  // Arduino Pin Number         
#define LED_BUILTIN                 PB4 // 18
#define LED_GREEN                   LED_BUILTIN
#define LED_BLUE                    LED_BUILTIN
#define LED_RED                     PA15 // 19
#define LED_ORANGE                  LED_RED
#define LED_RED_OTG_OVERCURRENT     LED_RED // be careful with this led. When using it configure the output as PULL_UP.

//On-board user button
//#define USER_BTN                    PC13  // 2


// Connected to on board SPI1
#define MOSI                        PA7
#define MISO                        PA6
#define SCK                         PA5
#define SS                          PA4

// Connected to on board I2C
#define SDA                         PB7
#define SCL                         PB6

#endif
