/*
 * Test double for the WCH CH32V00X SDK GPIO header (ch32v00X_gpio.h).
 * It declares only what max7219.c needs. The implementations live in
 * test_max7219.c and record every call, so no SDK files are required to
 * build and run the host tests.
 */

#ifndef MOCK_CH32V00X_GPIO_H
#define MOCK_CH32V00X_GPIO_H

#include <stdint.h>

typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;

typedef enum {
    GPIO_Mode_AIN = 0,
    GPIO_Mode_IN_FLOATING,
    GPIO_Mode_IPD,
    GPIO_Mode_IPU,
    GPIO_Mode_Out_OD,
    GPIO_Mode_Out_PP
} GPIOMode_TypeDef;

typedef enum {
    GPIO_Speed_10MHz = 1,
    GPIO_Speed_2MHz,
    GPIO_Speed_30MHz
} GPIOSpeed_TypeDef;

typedef struct {
    uint16_t GPIO_Pin;
    GPIOSpeed_TypeDef GPIO_Speed;
    GPIOMode_TypeDef GPIO_Mode;
} GPIO_InitTypeDef;

#define GPIO_Pin_0 ((uint16_t)0x0001)
#define GPIO_Pin_1 ((uint16_t)0x0002)
#define GPIO_Pin_2 ((uint16_t)0x0004)
#define GPIO_Pin_3 ((uint16_t)0x0008)
#define GPIO_Pin_4 ((uint16_t)0x0010)
#define GPIO_Pin_5 ((uint16_t)0x0020)
#define GPIO_Pin_6 ((uint16_t)0x0040)
#define GPIO_Pin_7 ((uint16_t)0x0080)

/* Peripheral clock bits: distinct values, only GPIOC is used by the driver. */
#define RCC_PB2Periph_GPIOA ((uint32_t)0x00000004)
#define RCC_PB2Periph_GPIOC ((uint32_t)0x00000010)
#define RCC_PB2Periph_GPIOD ((uint32_t)0x00000020)

typedef struct { uint32_t odr; } GPIO_TypeDef;

extern GPIO_TypeDef mock_GPIOC;
#define GPIOC (&mock_GPIOC)

/* Implemented by the test (test_max7219.c). */
void GPIO_SetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void GPIO_ResetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct);
void RCC_PB2PeriphClockCmd(uint32_t RCC_PB2Periph, FunctionalState NewState);

#endif /* MOCK_CH32V00X_GPIO_H */
