/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Copyright (c) 2026 珠海笙科智能科技有限公司 (M-VAVE): the RGB and LED code, from smk37_open app/board/panel.c.
 * Copyright (c) 2026 Tom-Rfid (OpenVALive): the adaptation.
 *
 * OpenVALive - SMK-37 panel: M-VAVE's scanner (board/scan.c, unchanged), screen (lcd.inc, unchanged) and pad
 * LEDs, handed to the app through glue.h. The scanner reads the factory calibration of each key and pad (read-only);
 * nothing here writes the flash. */
#include "smk37_hw.h"
#include "fm1_lcd_hw.h"
#include "lcd.inc"
#include "../glue.h"

static uint8_t rgb_buffer[420] __attribute__((aligned(4)));
static uint32_t rgb_colors[16];
static unsigned rgb_dirty = 0xffff;
/* the pads' order on the LED chain */
static const uint8_t rgb_map[16] = {0, 15, 14, 13, 12, 11, 10, 9, 1, 2, 3, 4, 5, 6, 7, 8};

/* PH8 is an encoder input on both boards: no TRS MIDI receiver (the TRS jack is MIDI OUT only). */
uint8_t fm1_io_trs_midi_enabled(void) { return 0; }

/* The scanner's callbacks (board/scan.c): every change of a control, in order, in the control task. */
void smk37_control_event(unsigned kind, unsigned id, int value) { smk_input_event(kind, id, value); }
void smk37_controls_panic(void) { smk_input_panic(); }

void smk_panel_init(void)
{
    smk37_scan_init();          /* also starts TIMER4, the 24 MHz counter */
    fm1_screen_init();
    for (unsigned i = 0; i < 18; ++i)
        smk37_led(i, 0);
}

void smk_panel_poll(void) { smk37_scan_read(); }

uint32_t smk_ticks24(void) { return fm1_device_ticks(); }

void smk_led(unsigned button, unsigned on) { smk37_led(button, on); }

void smk37_rgb_set(unsigned id, unsigned rgb)
{
    if (id < 16 && rgb_colors[id] != rgb) {
        rgb_colors[id] = rgb;
        rgb_dirty |= 1U << id;
    }
}

void smk_pad_rgb(unsigned pad, uint32_t rgb) { smk37_rgb_set(pad, rgb); }

/* WS2812 bits as SPI nibbles, sent on SPI1 between two screen transfers (one task owns SPI1). */
void smk37_rgb_service(void)
{
    if (!rgb_dirty || fm1_screen_pending())
        return;
    static const uint32_t nibble[16] = {
        0xc0c0c0c0, 0xf0c0c0c0, 0xc0f0c0c0, 0xf0f0c0c0,
        0xc0c0f0c0, 0xf0c0f0c0, 0xc0f0f0c0, 0xf0f0f0c0,
        0xc0c0c0f0, 0xf0c0c0f0, 0xc0f0c0f0, 0xf0f0c0f0,
        0xc0c0f0f0, 0xf0c0f0f0, 0xc0f0f0f0, 0xf0f0f0f0};
    for (unsigned i = 0; i < 16; ++i) {
        if (!(rgb_dirty & (1U << i)))
            continue;
        unsigned rgb = rgb_colors[i], grb = ((rgb & 0xff00) << 8) | ((rgb >> 8) & 0xff00) | (rgb & 255);
        for (unsigned n = 0; n < 6; ++n) {
            uint32_t word = nibble[(grb >> (20 - 4 * n)) & 15];
            memcpy(rgb_buffer + 36 + 24 * rgb_map[i] + 4 * n, &word, 4);
        }
    }
    fm1_lcd_deselect();
    JL_IOMAP->CON1 |= BIT(4);
    JL_SPI1->BAUD = 8;
    JL_SPI1->CON |= BIT(14);
    fm1_device_fence();
    JL_SPI1->ADR = (uint32_t)(uintptr_t)rgb_buffer;
    JL_SPI1->CNT = sizeof rgb_buffer;
    uint32_t errors = fm1_lcd_timeouts;
    fm1_lcd_wait();
    JL_IOMAP->CON1 &= ~BIT(4);
    JL_SPI1->BAUD = 4;
    if (errors == fm1_lcd_timeouts)
        rgb_dirty = 0;
}

void smk_panel_service(void) { smk37_rgb_service(); }

void smk_screen_fill(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t rgb565)
{
    fm1_screen_fill(x, y, w, h, rgb565);
}

/* The app draws byte-swapped pixels (the panel's own order, fm1-base gfx.c): sent as they are, a band at a time. */
void smk_screen_blit_be(unsigned x, unsigned y, unsigned w, unsigned h, const uint16_t *big_endian)
{
    if (!display_valid(x, y, w, h, big_endian, (size_t)w * h))
        return;
    display_finish();
    display_window(x, y, w, h);
    for (unsigned remaining = w * h; remaining;) {
        unsigned n = remaining < 240U * 16U ? remaining : 240U * 16U;
        display_data(big_endian, 2 * n);
        big_endian += n;
        remaining -= n;
    }
}

/* The SDK's panel API (fm1.h), for completeness: OpenVALive reads the scanner itself. */
void fm1_controls_init(void) { smk37_scan_init(); }
void fm1_controls_read(struct fm1_controls *state)
{
    smk37_scan_read();
    memset(state, 0, sizeof *state);
    state->battery = smk37_battery_adc();
}
void fm1_led_set(unsigned key, int on) { (void)key; (void)on; }
