/********************************** (C) COPYRIGHT *******************************
 * File Name          : max7219.h
 * Author             : Sirotkin Mikhail
 * Version            : V1.0.0
 * Date               : 23.09.2026
 * Description        : MAX7219 library.
 *********************************************************************************
 * Copyright (c) 2026

 *******************************************************************************/

#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>

#define MAX7219_DASH_CODE           0x0A
#define MAX7219_E_CODE              0x0B
#define MAX7219_H_CODE              0x0C
#define MAX7219_L_CODE              0x0D
#define MAX7219_P_CODE              0x0E
#define MAX7219_BLANK_CODE          0x0F

#define MAX7219_DP_BIT              0x80
#define MAX7219_DEFAULT_INTENSITY   0x00

void MAX7219_enable(void);

void MAX7219_shutdown(void);

void MAX7219_set_decode_all(void);

void MAX7219_set_scan_limit(uint8_t digit);

void MAX7219_set_digit(uint8_t index, uint8_t symbol);

void MAX7219_set_digit_point(uint8_t index, uint8_t symbol);

void MAX7219_set_blank(uint8_t index);

void MAX7219_exit_test_mode(void);

void MAX7219_enter_test_mode(void);

void MAX7219_set_intensity(uint8_t intensity_value);

void MAX7219_Init(void);

#endif /* MAX7219_H */