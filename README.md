# MAX7219 driver for CH32V006

Minimal bare-metal C driver for a single MAX7219 LED display driver connected to a
7-segment display and used in Code-B decode mode. The control lines are bit-banged
through three GPIO pins. No dynamic memory, no HAL, no interrupts: one .c file and
one .h file to drop into a MounRiver Studio CH32V006 project.

## Features

- single MAX7219, 7-segment display, Code-B decode (digits 0-9, `-`, `E`, `H`, `L`, `P`, blank)
- decimal point control (per digit)
- brightness (intensity 0x00-0x0F), scan limit, shutdown mode, display test mode
- bit-banged 3-wire interface (DIN, LOAD/CS, CLK), no hardware SPI required
- all hardware settings collected in one "Configuration" section of max7219.c
- host tests with a mocked GPIO layer (no hardware needed)

## Scope and limitations

- one chip only: no daisy-chaining, no LED matrix support, no no-decode (raw segment)
 mode helpers, no number formatting, no hardware SPI backend
- blocking bit-bang: every call sends one 16-bit register frame (tens of microseconds
 at 48 MHz system clock)
- not reentrant: do not call the driver concurrently from an ISR and the main program
 (usage from one single context, including one ISR, is safe)
- target platform: CH32V006 / CH32V00x family with the WCH ch32v00X_* SDK headers

## Wiring (default configuration)

| MAX7219 signal | CH32V006 pin | QFN20 (CH32V006F8U7) | Function       |
|---------------|------------|---------------------|---------------|
| DIN           | PC1        | pin 8               | serial data   |
| LOAD (CS)     | PC2        | pin 9               | frame latch   |
| CLK           | PC3        | pin 10              | serial clock  |
| GND           | GND        |                     | common ground |
| V+            | 5V (see below) |                 | supply        |

Pin numbers differ per package; verify with the CH32V006 datasheet for your exact
part. To use other pins or another port, edit the "Configuration" section at the top
of max7219.c (port, port clock enable, pins, default intensity).

## Electrical notes

Verified against the MAX7219/MAX7221 datasheet (Analog Devices/Maxim, 19-4452,
Rev 6) and the WCH CH32V006 datasheet:

- Supply: the MAX7219 operates from 4.0 V to 5.5 V. It is not specified at 3.3 V.
- Logic levels: VIH min = 3.5 V, VIL max = 0.8 V. A 3.3 V MCU output high is below
 the guaranteed VIH of the MAX7219. Recommended: power the CH32V006 from 5 V (its
 VDD range is 2.0-5.5 V); then all logic levels match directly. If the MCU must run
 at 3.3 V, use a level shifter for DIN/LOAD/CLK - a direct connection is out of
 specification even if it sometimes appears to work.
- ISET: a resistor R_SET between V+ and ISET sets the peak segment current (about
 100x the ISET pin current). R_SET must be at least 9.53 kOhm (about 40 mA per
 segment); most modules ship 10 kOhm. For lower currents see datasheet Table 11
 (example for V_LED = 2.0 V: 20 mA -> 28 kOhm, 10 mA -> 63.7 kOhm). If you scan
 3 or fewer digits, derate per datasheet Table 9 (10/20/30 mA for 1/2/3 digits).
- Decoupling: place a 0.1 uF ceramic capacitor and a 10 uF electrolytic capacitor
 between V+ and GND as close to the MAX7219 as possible; keep the wires to the
 display short. On DIP/SO packages both GND pins must be connected.
- Common ground between the MCU board and the display module is mandatory.
- Serial timing: the MAX7219 accepts a 10 MHz clock maximum; a GPIO bit-bang at
 48 MHz system clock is far below that, so no timing constraints are violated.

## Integrating into a MounRiver Studio project

1. Create or open a CH32V006 project in MounRiver Studio (CH32V00x template).
2. Copy max7219.c and max7219.h into the project (for example next to User/main.c
 or into a Driver/ folder) and add max7219.c to the build.
3. If your wiring differs, edit the "Configuration" section at the top of max7219.c.
4. Include "max7219.h" where needed and call MAX7219_Init() once after system clock
 setup.

The driver includes "ch32v00X_gpio.h" from the WCH SDK that ships with the MounRiver
CH32V00x project template; no other dependencies.

## Example

See [example/main.c](example/main.c): after MAX7219_Init() it counts from 0 to 9999
on a 4-digit module with the decimal point lit on digit 2 (reads as "12.34"):

```c
#include "debug.h"
#include "max7219.h"

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    MAX7219_Init();
    MAX7219_set_intensity(0x08);

    while (1) {
        show_number(counter++);
        Delay_Ms(100);
    }
}
```

debug.h and Delay_Ms() come with the MounRiver Studio project template.

## API reference

Full documentation is in [max7219.h](max7219.h). Summary:

| Function | Description |
|----------|-------------|
| MAX7219_Init() | Configure the pins and the chip; leaves the display blank and on |
| MAX7219_set_digit(index, symbol) | Show a Code-B symbol on digit index (0..7) |
| MAX7219_set_digit_point(index, symbol) | Same, with the decimal point forced on |
| MAX7219_set_blank(index) | Blank one digit |
| MAX7219_set_intensity(v) | Brightness 0x00 (dim) .. 0x0F (bright); other values ignored |
| MAX7219_set_scan_limit(d) | Scan digits 0..d (0..7) |
| MAX7219_set_decode_all() | Enable Code-B decode for all digits |
| MAX7219_shutdown() / MAX7219_enable() | Blank the display / show it again (registers kept) |
| MAX7219_enter_test_mode() / MAX7219_exit_test_mode() | All segments on / back to normal |

Code-B symbol constants: 0x00-0x09 are the digits themselves; MAX7219_DASH_CODE,
MAX7219_E_CODE, MAX7219_H_CODE, MAX7219_L_CODE, MAX7219_P_CODE,
MAX7219_BLANK_CODE; OR MAX7219_DP_BIT (0x80) to light the decimal point.

Digit index 0..7 maps to the DIG0..DIG7 registers; on most modules DIG0 is the
rightmost digit - verify on your module. Invalid arguments are ignored silently
without any bus traffic.

## Tests

tests/test_max7219.c contains host tests: the GPIO layer is replaced by a mock
(tests/mock/ch32v00X_gpio.h), and a small simulator reconstructs the MAX7219
receiver (DIN sampled on the CLK rising edge, frames latched on the LOAD rising
edge). The tests check the register traffic of every public function, the init
sequence, handling of invalid arguments, and bus invariants (exactly 16 clocks per
frame, no clocks outside a frame, DIN stable while CLK is high).

Build and run from the tests/ directory with any host C compiler:

```sh
cc -I mock -I .. -Wall -Wextra -o test_max7219 test_max7219.c ../max7219.c
./test_max7219
```

Verified: compiles warning-free with MSVC (cl /W4 /WX) and with
riscv-wch-elf-gcc (GCC 12, -Wall -Wextra, rv32ec); the test suite passes. These are
logic tests only - they are not a hardware test.

## Hardware bring-up checklist

Things to verify on the real board (no hardware was tested during development of
this release):

- [ ] Wiring matches the table above; common ground connected; both GND pins of the
 MAX7219 tied to GND (DIP/SO packages)
- [ ] V+ = 5 V; MCU powered from 5 V (recommended) or DIN/LOAD/CLK level-shifted
- [ ] R_SET on the module is at least 9.53 kOhm (typical modules: 10 kOhm)
- [ ] 0.1 uF + 10 uF decoupling near the MAX7219 V+/GND
- [ ] After MAX7219_Init() the display is blank; MAX7219_enter_test_mode() lights
 all segments of all digits
- [ ] MAX7219_set_digit(0, 1) - find out which physical position digit 0 is on your
 module
- [ ] Sweep MAX7219_set_intensity() from 0x00 to 0x0F and check the brightness range
- [ ] Run the example counter for a while; check the supply current stays within the
 R_SET-derived budget and the display does not flicker

## License

MIT - see [LICENSE](LICENSE). Copyright (c) 2026 Sirotkin Mikhail.

## References

- MAX7219/MAX7221 datasheet, Analog Devices (19-4452, Rev 6):
 https://www.analog.com/media/en/technical-documentation/data-sheets/MAX7219-MAX7221.pdf
- CH32V006 datasheet, WCH: https://www.wch-ic.com/products/CH32V006.html
