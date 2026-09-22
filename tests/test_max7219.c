/*
 * Host-side tests for the MAX7219 driver.
 *
 * The GPIO layer is replaced by the test double in mock/ch32v00X_gpio.h.
 * The tests simulate the MAX7219 serial interface (DIN is sampled on the CLK
 * rising edge, a frame is latched on the LOAD rising edge) and check the
 * register traffic produced by the driver, plus bus invariants (frame length,
 * CLK only inside a frame, DIN stable while CLK is high).
 *
 * Build and run from the tests/ directory (any host C compiler):
 *   cc -I mock -I .. -Wall -Wextra -o test_max7219 test_max7219.c ../max7219.c
 *   ./test_max7219
 *
 * These tests verify driver logic only; they are not a hardware test.
 */

#include <stdio.h>
#include <stdbool.h>
#include "ch32v00X_gpio.h"    /* mock */
#include "../max7219.h"

/* ----------------  event log  ---------------- */

typedef enum { EV_SET, EV_RESET, EV_INIT, EV_RCC } ev_kind;

typedef struct {
    ev_kind kind;
    const GPIO_TypeDef *port;
    uint32_t mask; /* pin mask or peripheral clock bit */
} event;

#define MAX_EVENTS 8192
static event events[MAX_EVENTS];
static int event_count;

static GPIOMode_TypeDef init_mode;

GPIO_TypeDef mock_GPIOC;

#define M_DIN  GPIO_Pin_1
#define M_CS   GPIO_Pin_2
#define M_CLK  GPIO_Pin_3

void GPIO_SetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    events[event_count].kind = EV_SET;
    events[event_count].port = GPIOx;
    events[event_count].mask = GPIO_Pin;
    event_count++;
}

void GPIO_ResetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    events[event_count].kind = EV_RESET;
    events[event_count].port = GPIOx;
    events[event_count].mask = GPIO_Pin;
    event_count++;
}

void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct)
{
    events[event_count].kind = EV_INIT;
    events[event_count].port = GPIOx;
    events[event_count].mask = GPIO_InitStruct->GPIO_Pin;
    init_mode = GPIO_InitStruct->GPIO_Mode;
    event_count++;
}

void RCC_PB2PeriphClockCmd(uint32_t RCC_PB2Periph, FunctionalState NewState)
{
    events[event_count].kind = EV_RCC;
    events[event_count].port = 0;
    events[event_count].mask = (NewState == ENABLE) ? RCC_PB2Periph : 0u;
    event_count++;
}

/* ----------------  MAX7219 simulator  ---------------- */

typedef struct { uint8_t reg, data; } reg_write;

#define MAX_WRITES 128
static reg_write writes[MAX_WRITES];
static int write_count;

static bool cs, clk, din, armed;
static uint32_t shift;
static int bits;
static int processed;

static int clk_while_cs_high;       /* CLK rising edges outside a frame */
static int din_changes_while_clk_high;
static int bad_frames;              /* LOAD rising without exactly 16 bits */

static void sim_tick(void)
{
    for (; processed < event_count; processed++) {
        event *e = &events[processed];
        bool old_cs = cs, old_clk = clk, old_din = din;

        switch (e->kind) {
        case EV_SET:
        case EV_RESET: {
            uint32_t level = ((din ? M_DIN : 0u) | (cs ? M_CS : 0u)
                             | (clk ? M_CLK : 0u));
            if (e->kind == EV_SET) {
                level |= e->mask;
            } else {
                level &= ~e->mask;
            }
            cs  = (level & M_CS)  != 0;
            clk = (level & M_CLK) != 0;
            din = (level & M_DIN) != 0;
            break;
        }
        case EV_INIT:
            armed = true; /* transport is live after the pins are set up */
            break;
        case EV_RCC:
            break;
        }

        if (!armed) {
            continue;
        }

        if (clk && !old_clk) {       /* CLK rising edge: sample DIN */
            if (cs) {
                clk_while_cs_high++;
            } else {
                shift = (shift << 1) | (din ? 1u : 0u);
                bits++;
            }
        }

        if (din != old_din && clk) { /* DIN must be stable while CLK is high */
            din_changes_while_clk_high++;
        }

        if (cs && !old_cs) {       /* LOAD rising edge: latch the frame */
            if (bits == 16 && write_count < MAX_WRITES) {
                writes[write_count].reg  = (uint8_t)((shift >> 8) & 0x0Fu);
                writes[write_count].data = (uint8_t)(shift & 0xFFu);
                write_count++;
            } else {
                bad_frames++;
            }
            shift = 0;
            bits = 0;
        }
    }
}

/* ----------------  assertions  ---------------- */

static int failures;

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

static void reset_state(void)
{
    event_count = 0;
    processed = 0;
    write_count = 0;
    cs = clk = din = false;
    armed = false;
    shift = 0;
    bits = 0;
    clk_while_cs_high = 0;
    din_changes_while_clk_high = 0;
    bad_frames = 0;
}

static void clear_writes(void)
{
    write_count = 0;
}

static void check_invariants(void)
{
    CHECK(clk_while_cs_high == 0);
    CHECK(din_changes_while_clk_high == 0);
    CHECK(bad_frames == 0);
    CHECK(cs);   /* LOAD idles high */
    CHECK(!clk); /* CLK returns low */
}

/* ----------------  tests  ---------------- */

static void test_init_sequence(void)
{
    reset_state();
    MAX7219_Init();
    sim_tick();

    /* GPIO setup: port clock first, CS high, CLK/DIN low, then configure */
    CHECK(event_count >= 4);
    CHECK(events[0].kind == EV_RCC);
    CHECK(events[0].mask == RCC_PB2Periph_GPIOC);
    CHECK(events[1].kind == EV_SET);
    CHECK(events[1].mask == M_CS);
    CHECK(events[1].port == GPIOC);
    CHECK(events[2].kind == EV_RESET);
    CHECK(events[2].mask == (M_CLK | M_DIN));
    CHECK(events[3].kind == EV_INIT);
    CHECK(events[3].mask == (M_DIN | M_CS | M_CLK));
    CHECK(init_mode == GPIO_Mode_Out_PP);

    /* register traffic: shutdown, test off, decode, scan, intensity, */
    /* 8x blank, enable */
    CHECK(write_count == 14);
    CHECK(writes[0].reg == 0x0C && writes[0].data == 0x00);
    CHECK(writes[1].reg == 0x0F && writes[1].data == 0x00);
    CHECK(writes[2].reg == 0x09 && writes[2].data == 0xFF);
    CHECK(writes[3].reg == 0x0B && writes[3].data == 0x07);
    CHECK(writes[4].reg == 0x0A && writes[4].data == 0x00); /* default */
    for (int i = 0; i < 8; i++) {
        CHECK(writes[5 + i].reg == (uint8_t)(0x01 + i));
        CHECK(writes[5 + i].data == MAX7219_BLANK_CODE);
    }
    CHECK(writes[13].reg == 0x0C && writes[13].data == 0x01);

    check_invariants();
}

static void test_set_digit_mapping(void)
{
    for (uint8_t index = 0; index <= 7; index++) {
        clear_writes();
        MAX7219_set_digit(index, 0x05);
        sim_tick();
        CHECK(write_count == 1);
        CHECK(writes[0].reg == (uint8_t)(0x01 + index));
        CHECK(writes[0].data == 0x05);
    }
    check_invariants();
}

static void test_set_digit_point(void)
{
    clear_writes();
    MAX7219_set_digit_point(2, 0x07);
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x03);
    CHECK(writes[0].data == (0x07 | MAX7219_DP_BIT));

    clear_writes();
    MAX7219_set_digit_point(1, 0x85); /* DP bit already set: idempotent */
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].data == 0x85);
    check_invariants();
}

static void test_invalid_args_are_ignored(void)
{
    int ec;

    clear_writes();
    ec = event_count;
    MAX7219_set_digit(8, 1);
    MAX7219_set_digit_point(200, 2);
    MAX7219_set_blank(8);
    MAX7219_set_scan_limit(8);
    MAX7219_set_intensity(0x10);
    sim_tick();
    CHECK(write_count == 0);
    CHECK(event_count == ec); /* no bus activity at all */
    check_invariants();
}

static void test_control_registers(void)
{
    clear_writes();
    MAX7219_set_blank(0);
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x01 && writes[0].data == MAX7219_BLANK_CODE);

    clear_writes();
    MAX7219_set_intensity(0x0F);
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x0A && writes[0].data == 0x0F);

    clear_writes();
    MAX7219_set_scan_limit(3);
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x0B && writes[0].data == 0x03);

    clear_writes();
    MAX7219_enter_test_mode();
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x0F && writes[0].data == 0x01);

    clear_writes();
    MAX7219_exit_test_mode();
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x0F && writes[0].data == 0x00);

    clear_writes();
    MAX7219_shutdown();
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x0C && writes[0].data == 0x00);

    clear_writes();
    MAX7219_enable();
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x0C && writes[0].data == 0x01);

    clear_writes();
    MAX7219_set_decode_all();
    sim_tick();
    CHECK(write_count == 1);
    CHECK(writes[0].reg == 0x09 && writes[0].data == 0xFF);
    check_invariants();
}

static void test_code_b_constants(void)
{
    CHECK(MAX7219_DASH_CODE == 0x0A);
    CHECK(MAX7219_E_CODE == 0x0B);
    CHECK(MAX7219_H_CODE == 0x0C);
    CHECK(MAX7219_L_CODE == 0x0D);
    CHECK(MAX7219_P_CODE == 0x0E);
    CHECK(MAX7219_BLANK_CODE == 0x0F);
    CHECK(MAX7219_DP_BIT == 0x80);
}

int main(void)
{
    test_init_sequence();
    test_set_digit_mapping();
    test_set_digit_point();
    test_invalid_args_are_ignored();
    test_control_registers();
    test_code_b_constants();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    printf("%d check(s) failed.\n", failures);
    return 1;
}
