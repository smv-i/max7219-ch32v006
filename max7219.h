/********************************** (C) COPYRIGHT *******************************
 * File Name          : max7219.h
 * Author             : Sirotkin Mikhail
 * Version            : V1.0.0
 * Date               : 23.09.2026
 * Description        : MAX7219 driver public API.
 *********************************************************************************
 * Copyright (c) 2026 Sirotkin Mikhail
 * Licensed under the MIT License. See the LICENSE file for details.
 *******************************************************************************/

#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>

/*
 * Bare-metal driver for a single MAX7219 LED display driver connected to a
 * 7-segment display and used in Code-B decode mode.
 *
 * The control lines are bit-banged through GPIO. Default wiring (CH32V006):
 *   PC1 -> MAX7219 DIN
 *   PC2 -> MAX7219 LOAD (CS)
 *   PC3 -> MAX7219 CLK
 * All hardware settings (GPIO port, port clock, pins, default intensity)
 * are collected in the "Configuration" section at the top of max7219.c.
 *
 * Usage:
 * 1. Call MAX7219_Init() once after system clock setup.
 * 2. Call the MAX7219_set_*() functions to update the display.
 *
 * Digit numbering: index 0..7 selects the DIG0..DIG7 registers of the
 * MAX7219. The physical position of a digit depends on the module wiring
 * (on most 4/8-digit modules DIG0 is the rightmost digit).
 *
 * Invalid arguments: functions check their arguments and return without
 * touching the bus when a value is out of range (no error reporting).
 *
 * Interrupts: the driver uses no dynamic memory and does not disable
 * interrupts, but one register frame must not be interleaved with frames
 * from another context. Do not call the driver concurrently from an ISR and
 * the main program; usage from a single context (including one ISR) is safe.
 * Every call sends one 16-bit frame and completes in tens of microseconds
 * at 48 MHz; nothing blocks.
 */

/* Code-B symbols for decoded digits (MAX7219 datasheet Table 5).
 * Codes 0x00..0x09 display the digits 0..9, the codes below display: */
#define MAX7219_DASH_CODE           0x0A    /* '-' */
#define MAX7219_E_CODE              0x0B    /* 'E' */
#define MAX7219_H_CODE              0x0C    /* 'H' */
#define MAX7219_L_CODE              0x0D    /* 'L' */
#define MAX7219_P_CODE              0x0E    /* 'P' */
#define MAX7219_BLANK_CODE          0x0F    /* all segments off */

/* Bit D7: OR it with a symbol code to light the decimal point. */
#define MAX7219_DP_BIT              0x80

/*
 * Leave shutdown mode: the display shows the digit registers again
 * (the chip leaves shutdown in under 250 us).
 */
void MAX7219_enable(void);

/*
 * Enter shutdown mode: display blank, digit and control registers are
 * retained, supply current drops to about 150 uA. The chip can still be
 * programmed while in shutdown.
 */
void MAX7219_shutdown(void);

/* Enable Code-B decode mode for all 8 digits. */
void MAX7219_set_decode_all(void);

/*
 * Set the scan limit: display digits 0..digit (valid range 0..7).
 * digit > 7 is ignored. Note (datasheet): with 3 or fewer scanned digits
 * the per-digit driver power grows; keep RSET within the datasheet limits
 * (see README.md).
 */
void MAX7219_set_scan_limit(uint8_t digit);

/*
 * Show a Code-B symbol on digit index (0..7). index > 7 is ignored.
 * symbol: 0x00..0x09 show digits '0'..'9'; use the MAX7219_*_CODE constants
 * above for '-', 'E', 'H', 'L', 'P' and blank. The decoder ignores bits
 * D6..D4; bit D7 (MAX7219_DP_BIT) lights the decimal point.
 */
void MAX7219_set_digit(uint8_t index, uint8_t symbol);

/* Same as MAX7219_set_digit(), but the decimal point is forced on. */
void MAX7219_set_digit_point(uint8_t index, uint8_t symbol);

/* Blank digit index (0..7); index > 7 is ignored. */
void MAX7219_set_blank(uint8_t index);

/* Leave display test mode (see MAX7219_enter_test_mode). */
void MAX7219_exit_test_mode(void);

/*
 * Display test mode: all segments of all digits on, independently of the
 * shutdown and intensity settings. Stays on until MAX7219_exit_test_mode.
 */
void MAX7219_enter_test_mode(void);

/*
 * Set display intensity: 0x00 (dimmest, 1/32 duty) .. 0x0F (brightest,
 * 31/32 duty). Values above 0x0F are ignored.
 */
void MAX7219_set_intensity(uint8_t intensity_value);

/*
 * Initialize the GPIO pins and the MAX7219 itself. Call once after system
 * clock setup, before any other function of this driver. Resulting state:
 * normal operation (not shutdown), Code-B decode for all 8 digits, scan
 * limit of 8 digits, intensity MAX7219_DEFAULT_INTENSITY (max7219.c),
 * all digits blank.
 */
void MAX7219_Init(void);

#endif /* MAX7219_H */
