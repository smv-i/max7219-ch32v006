/*
 * Minimal CH32V006 example for the MAX7219 driver.
 *
 * Wiring (default driver configuration, see max7219.c):
 *   PC1 -> MAX7219 DIN,  PC2 -> MAX7219 LOAD (CS),  PC3 -> MAX7219 CLK,
 *   common ground between the board and the module.
 *
 * The example counts from 0 to 9999 on a 4-digit module and lights the
 * decimal point on digit 2 (reads as "12.34"). debug.h and Delay_Ms()
 * come with the MounRiver Studio CH32V00x project template.
 */

#include "debug.h"
#include "max7219.h"

#define EXAMPLE_DIGITS 4    /* digits actually mounted on the module */

static void show_number(uint16_t value)
{
    for (uint8_t digit = 0; digit < EXAMPLE_DIGITS; digit++) {
        uint8_t symbol = (uint8_t) (value % 10);

        if (digit == 2) {
            MAX7219_set_digit_point(digit, symbol);
        } else {
            MAX7219_set_digit(digit, symbol);
        }

        value /= 10;
    }
}

int main(void)
{
    uint16_t counter = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    MAX7219_Init();
    MAX7219_set_intensity(0x08);

    while (1) {
        show_number(counter);

        counter++;
        if (counter > 9999) {
            counter = 0;
        }

        Delay_Ms(100);
    }
}
