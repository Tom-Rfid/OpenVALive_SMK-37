/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * OpenVALive - SMK-37: the calls between the board layer (board/, on M-VAVE's smk37_open drivers) and the app
 * (smk37_app.c, the FM-1 app on a small platform of its own). Plain C only: no SDK macros cross this line. */
#pragma once
#include <stdint.h>

/* the SMK-37's controls, as board/scan.c reports them (smk37_controls.h's SMK37_* kinds) */
enum { SMK_BUTTON, SMK_KEY, SMK_PRESSURE, SMK_PAD, SMK_ENCODER, SMK_COMBINATION, SMK_PEDAL, SMK_FADER };

/* physical buttons, as the scanner numbers them (smk37_open smk37_controls.c, button_map), named by the FM-1
 * button they are. The 12 on the right in the FM-1's order: top ARP, NOTE REPEAT = SEL, SCALE = ENV, CHORD = LFO,
 * GLOBE = EDIT, BT = GLO; below PATCH = HOME, PARA = SAVE, FX, SEQ, PLAY, REC (ARP and FX swap places, as printed).
 * On the left: KNOB BANK = SHIFT, FADER BANK, the arrows (left: TAP, held the CONTROL MAP; right: the pads' mode). */
enum {
    SMK_B_SHIFT = 0, SMK_B_FADER_BANK = 1, SMK_B_BACK = 2, SMK_B_PAD_STEP = 3, SMK_B_OCT_DOWN = 4, SMK_B_OCT_UP = 5,
    SMK_B_ARP = 6, SMK_B_SELECT = 7, SMK_B_ENV = 8, SMK_B_LFO = 9, SMK_B_EDIT = 10, SMK_B_GLO = 11,
    SMK_B_HOME = 12, SMK_B_SAVE = 13, SMK_B_FX = 14, SMK_B_SEQ = 15, SMK_B_PLAY = 16, SMK_B_REC = 17, SMK_BUTTONS = 18
};
#define SMK_KEYS 37         /* C3 .. C6 */
#define SMK_PADS 16
#define SMK_ENCODERS 8      /* K1..K4, SELECT, PAGE, PRESETS, MASTER */

/* board/panel_ols.c (control task) */
void smk_panel_init(void);                  /* scanner, screen */
void smk_panel_poll(void);                  /* the scanner's events -> smk_input_event, in order */
void smk_panel_service(void);               /* the pads' colours, when the screen is idle */
void smk_led(unsigned button, unsigned on); /* a button's LED, by physical number */
void smk_pad_rgb(unsigned pad, uint32_t rgb);   /* 0xRRGGBB */
void smk_screen_fill(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t rgb565);
void smk_screen_blit_be(unsigned x, unsigned y, unsigned w, unsigned h, const uint16_t *big_endian);
uint32_t smk_ticks24(void);                 /* 24 MHz counter (TIMER4) */

/* board/audio_ols.c */
typedef void (*smk_render_fn)(int32_t *lr, uint32_t frames, uint32_t master_q12);
void smk_audio_set_render(smk_render_fn fn);   /* the app's render, called from the audio interrupt */
void smk_audio_set_master(uint32_t q12);       /* 0..4096 */
uint32_t smk_audio_cpu_pct(void);              /* render time / block time, smoothed */

/* smk37_app.c */
void smk_input_event(unsigned kind, unsigned id, int value);
void smk_input_panic(void);                  /* the scanner lost events: everything up */
void smk_app_boot(void);                     /* once, before the first poll */
void smk_app_poll(void);                     /* the control task's loop */
void smk_app_midi_in(unsigned source, const uint8_t *m, unsigned n);   /* routing task */
void smk_app_midi_flush(void);               /* send what the audio and the UI queued (control task) */
