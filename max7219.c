/********************************** (C) COPYRIGHT *******************************
 * File Name          : max7219.c
 * Author             : Sirotkin Mikhail
 * Version            : V1.0.0
 * Date               : 23.09.2026
 * Description        : MAX7219 library.
 *********************************************************************************
 * Copyright (c) 2026

 *******************************************************************************/


#include "max7219.h"
#include "ch32v00X_gpio.h"

#define MAX7219_DIN                 GPIO_Pin_1
#define MAX7219_CS                  GPIO_Pin_2
#define MAX7219_CLK                 GPIO_Pin_3

#define MAX7219_SHUTDOWN_REG        0x0C
#define MAX7219_DECODE_MODE_REG     0x09
#define MAX7219_SCAN_LIMIT_REG      0x0B
#define MAX7219_DISPLAY_TEST_REG    0x0F
#define MAX7219_DIGIT0_REG          0x01
#define MAX7219_INTENSITY_REG       0x0A

/*********************************************************************
 * @fn      MAX7219_GPIO_Init
 *
 * @brief   Initializes the GPIO pins for interact with MAX7219 chip on board.
 *
 * @return  none
 */
static void MAX7219_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC, ENABLE);

    /* MAX7219 DIN-->C.1   CS-->C.2    CLK-->C.3 */
    GPIO_InitStructure.GPIO_Pin = MAX7219_DIN | MAX7219_CS | MAX7219_CLK;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

    /* CS = 1, DIN = 0, CLK = 0*/
    GPIO_SetBits(GPIOC, MAX7219_CS);
    GPIO_ResetBits(GPIOC, MAX7219_CLK | MAX7219_DIN);

    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      MAX_7219_byte_transfer
 *
 * @brief   Byte transfer function.
 *
 * @return  none
 */
static void MAX7219_byte_transfer(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (byte & 0x80) {
            /* MAX7219 DIN = 1 */
            GPIO_SetBits(GPIOC, MAX7219_DIN);
        } else {
            /* MAX7219 DIN = 0 */
            GPIO_ResetBits(GPIOC, MAX7219_DIN);
        }

        /* MAX7219 CLK = 1 */
        GPIO_SetBits(GPIOC, MAX7219_CLK);

        byte <<= 1;

        /* MAX7219 CLK = 0 */
        GPIO_ResetBits(GPIOC, MAX7219_CLK);
    }   
}

/*********************************************************************
 * @fn      MAX7219_write
 *
 * @brief   Register write function.
 *
 * @return  none
 */
static void MAX7219_write(uint8_t address, uint8_t data)
{
    /* MAX7219 CS = 0 */
    GPIO_ResetBits(GPIOC, MAX7219_CS);

    MAX7219_byte_transfer(address);
    MAX7219_byte_transfer(data);

    /* MAX7219 CS = 1 */
    GPIO_SetBits(GPIOC, MAX7219_CS);
}

/*********************************************************************
 * @fn      MAX7219_enable
 *
 * @brief   Set shutdown register value to normal operation.
 *
 * @return  none
 */
void MAX7219_enable(void)
{
    MAX7219_write(MAX7219_SHUTDOWN_REG, (uint8_t) 1);
}

/*********************************************************************
 * @fn      MAX7219_shutdown
 *
 * @brief   Set shutdown register value to shutdown mode.
 *
 * @return  none
 */
void MAX7219_shutdown(void)
{
    MAX7219_write(MAX7219_SHUTDOWN_REG, (uint8_t) 0);
}

/*********************************************************************
 * @fn      MAX7219_set_decode_all
 *
 * @brief   Set decode mode register value to B decode mode for all digits.
 *
 * @return  none
 */
void MAX7219_set_decode_all(void)
{
    MAX7219_write(MAX7219_DECODE_MODE_REG, 0xFF);
}

/*********************************************************************
 * @fn      MAX7219_set_scan_limit
 *
 * @brief   Set scan limit register value for digit (and digits under it).
 *
 * @return  none
 */
void MAX7219_set_scan_limit(uint8_t digit)
{
    if (digit > 7) return;

    MAX7219_write(MAX7219_SCAN_LIMIT_REG, digit);
}

/*********************************************************************
 * @fn      MAX7219_set_digit
 *
 * @brief   Set digit (0-7) to specific symbol in decode mode.
 *
 * @return  none
 */
void MAX7219_set_digit(uint8_t index, uint8_t symbol)
{
    if (index > 7) return;

    MAX7219_write(MAX7219_DIGIT0_REG + index, symbol);
}

/*********************************************************************
 * @fn      MAX7219_set_digit_point
 *
 * @brief   Set digit (0-7) to specific symbol in decode mode with digit point.
 *
 * @return  none
 */
void MAX7219_set_digit_point(uint8_t index, uint8_t symbol)
{
    if (index > 7) return;

    MAX7219_write(MAX7219_DIGIT0_REG + index, symbol | MAX7219_DP_BIT);
}

/*********************************************************************
 * @fn      MAX7219_set_blank
 *
 * @brief   Set digit (0-7) to blank symbol in decode mode.
 *
 * @return  none
 */
void MAX7219_set_blank(uint8_t index)
{
    if (index > 7) return;

    MAX7219_write(MAX7219_DIGIT0_REG + index, MAX7219_BLANK_CODE);
}

/*********************************************************************
 * @fn      MAX7219_exit_test_mode
 *
 * @brief   Exit test mode (all LEDs on).
 *
 * @return  none
 */
void MAX7219_exit_test_mode(void)
{
    MAX7219_write(MAX7219_DISPLAY_TEST_REG, (uint8_t) 0);
}

/*********************************************************************
 * @fn      MAX7219_enter_test_mode
 *
 * @brief   Enter test mode (all LEDs on).
 *
 * @return  none
 */
void MAX7219_enter_test_mode(void)
{
    MAX7219_write(MAX7219_DISPLAY_TEST_REG, (uint8_t) 1);
}

/*********************************************************************
 * @fn      MAX7219_set_intensity
 *
 * @brief   Set brightness by value 0h-Fh.
 *
 * @return  none
 */
void MAX7219_set_intensity(uint8_t intensity_value)
{
    if (intensity_value > 0x0F) return;

    MAX7219_write(MAX7219_INTENSITY_REG, intensity_value);
}

/*********************************************************************
 * @fn      MAX7219_Init
 *
 * @brief   Initialize GPIO pins and MAX7219 display so its ready for work.
 *
 * @return  none
 */
void MAX7219_Init(void)
{
    MAX7219_GPIO_Init();

    MAX7219_shutdown();
    MAX7219_exit_test_mode();
    MAX7219_set_decode_all();
    MAX7219_set_scan_limit(7);
    MAX7219_set_intensity(MAX7219_DEFAULT_INTENSITY);

    for (uint8_t i = 0; i < 8; i++) {
        MAX7219_set_blank(i);
    }

    MAX7219_enable();
}