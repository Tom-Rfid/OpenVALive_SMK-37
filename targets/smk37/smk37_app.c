/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Copyright (c) 2026 Tom-Rfid (OpenVALive).
 *
 * OpenVALive - SMK-37 Elite / Pro: the app.
 *
 * The SMK-37 runs the OpenVALive app (OpenVALive CORE's, in va/: the same Live Core, sounds, pages and layers
 * as the FM-1) on a small platform of its own, on M-VAVE's smk37_open SDK. This one unit holds:
 *
 *   1. the platform the FM-1 app expects (fm1-base's plat.h: buttons, encoders, MIDI, storage, screen), on the SDK;
 *   2. the app itself (va/app/openvalive_app.c, unchanged; this file sets its two hooks);
 *   3. what the SMK-37 adds: 37 keys, 16 RGB pads with pressure, 4 faders, the wheels, a pedal, a master
 *      encoder, and a MIDI OUT jack (clock and notes to a drum machine: the Roland T-8, say).
 *
 * Licence: this file and the rest of targets/smk37 are GPL-3.0 (with the SDK's linking exception), because the
 * SDK's are. OpenVALive CORE (va/, MIT) and fm1-base's files (MPL-2.0) keep their own licences.
 */
#include <stdint.h>
#include <string.h>
#include "fm1.h"
#include "glue.h"

/* ------------------------------------------------------------------ the platform (fm1-base's plat.h) */

#include "plat.h"
#include "touch.h"

/* a target's pads may stand for the FM-1's white keys with a layer of their own (ui_input.c, key_pressed) */
static uint32_t smk_key_layer(uint32_t k);
#define APP_KEY_LAYER(k) smk_key_layer(k)
/* the keyboard drawn on the screen while a layer button is held: the 37 keys from C3 */
static int32_t smk_screen_key(uint32_t i);
#define APP_DELAY_LEN 15360u                    /* the tracks' delays 1024 samples shorter than the FM-1's: 8 KB of RAM
                                                 * for the SDK's heap, which the firmware update needs (it stops reading
                                                 * the update loader with 55788 B); only delays past 0.35 s clamp. The
                                                 * first try (8eba494) put the USB buffers across 0x01C70000 and stayed
                                                 * on the logo: build.py keeps them inside one 64 KB block now */
#define APP_SCREEN_KEYS 37u
#define APP_SCREEN_FIRST 0u
#define APP_SCREEN_KEY(i) smk_screen_key(i)
/* the CONTROL MAP's slots: the USER page's K1..K4, the 4 faders (USER bank), the pitch and mod wheels */
#define APP_MAP_SLOTS 10u

#include "app.h"

/* The screen: fm1-base's gfx.c draws byte-swapped pixels into a canvas, sent here in one go. */
static void lcd_sync(void) {}
/* The scanner's queue holds 256 events and the buttons are read when it is read: a long screen frame lost button
 * taps and, past 256 events, let every button go (smk_input_panic). So it is also read between two bands. */
static uint8_t in_scan;
static void smk_scan(void)
{
    if (in_scan)
        return;
    in_scan = 1;
    smk_panel_poll();
    in_scan = 0;
}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    smk_screen_blit_be(x, y, w, h, px);
    smk_scan();
}
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { smk_screen_fill(x, y, w, h, c); }
#include "gfx.c"

/* No screen reader on the SMK-37 (fm1-base's announce.c speaks to fm1-reader over the FM-1's own USB stack). */
#define ANN_NOW 1u
static int ann_on(void) { return 0; }
static int ann_where(void) { return 0; }
static void ann_say(const char *a, const char *b, uint32_t flags) { (void)a; (void)b; (void)flags; }
static void ann_tick(uint32_t now) { (void)now; }

/* --- the panel as the FM-1 app sees it: FM-1 buttons, encoders and 27 keys (the "slots" of the layers) --- */

static volatile uint32_t v_buttons;            /* FM-1 buttons held (B_*) */
static uint32_t v_buttons_tapped;              /* pressed since the app last looked (a quick tap is not lost) */
static uint32_t v_keys;                        /* FM-1 keys held: the layers' slots */
static uint32_t v_keys_tapped;
static uint32_t v_keys_perform;                /* ...of those, the ones held by a pad in PERFORM mode */
static uint32_t v_keys_scenes, v_keys_tracks;   /* ...and by a pad in SCENES mode (a scene; a track's mute) */
static uint8_t v_vel[NKEYS];
static uint8_t v_press[NKEYS];
static int32_t v_enc[NE];
static uint32_t lights_buttons;
static uint8_t play_red, glow;

uint32_t plat_ms(void) { return fm1_millis(); }
uint32_t plat_ticks24(void) { return smk_ticks24(); }
uint32_t plat_cpu_pct(void) { return smk_audio_cpu_pct(); }
int plat_headphones(void) { return -1; }       /* no speaker to protect: the full sound everywhere */

uint32_t plat_buttons(void)
{
    uint32_t b = v_buttons | v_buttons_tapped;
    v_buttons_tapped = 0;
    return b;
}

int32_t plat_enc(int role)
{
    int32_t d = 0;
    if (role >= 0 && role < NE) {
        d = v_enc[role];
        v_enc[role] = 0;
    }
    return d;
}

void plat_glow(int on) { glow = (uint8_t)on; }
void plat_play_red(int red) { play_red = (uint8_t)red; }
void plat_leds(uint32_t buttons, uint32_t keys)
{
    (void)keys;                                 /* the pads show the layers (pad_colours) */
    lights_buttons = buttons;
}

/* the app's two levels: the buttons' LEDs have one, so the lit ones show (PLAY green or red: one LED here) */
void plat_lights(uint32_t buttons, uint32_t keys, uint32_t dim_buttons, uint32_t dim_keys, int glow)
{
    (void)dim_buttons;
    (void)dim_keys;
    (void)glow;
    plat_leds((buttons & ((1u << NB) - 1u)) | (buttons & LED_PLAY_GREEN ? 1u << B_PLAY : 0u), keys);
}

/* touch.h, "state" side: the slots the app reads */
void touch_on(int on) { (void)on; }
void touch_poll(void) {}
uint32_t touch_keys(void)
{
    uint32_t k = v_keys | v_keys_tapped;
    v_keys_tapped = 0;
    v_keys_perform &= k;                        /* a perform pad's slot stays marked until the app saw it go */
    v_keys_scenes &= k;
    v_keys_tracks &= k;
    return k;
}
uint8_t touch_vel(uint32_t key) { return key < NKEYS && v_vel[key] ? v_vel[key] : 100u; }
uint8_t touch_pressure(uint32_t key) { return key < NKEYS ? v_press[key] : 0u; }

static void slot_down(uint32_t k, uint32_t velocity)
{
    if (k >= NKEYS)
        return;
    v_vel[k] = (uint8_t)(velocity ? velocity : 1u);
    v_press[k] = 0;
    v_keys |= 1u << k;
    v_keys_tapped |= 1u << k;
}

static void slot_up(uint32_t k)
{
    if (k < NKEYS) {
        v_keys &= ~(1u << k);
        v_press[k] = 0;
    }
}

/* --- MIDI: in from the routing task (time-stamped), out from the audio and the UI to USB and the MIDI OUT jack --- */

#define MIDI_RING 64u                           /* (RAM: the SDK's heap needs it for the firmware update) */
static struct { uint32_t pkt, t24; uint8_t port; } midi_in_ring[MIDI_RING];
static volatile uint32_t midi_in_head, midi_in_tail;
static uint32_t midi_in_last_t24;

typedef struct {
    uint32_t pkt[MIDI_RING];
    uint8_t sent[MIDI_RING];                    /* ports it already went out on (a full port is retried alone) */
    volatile uint32_t head, tail;
} midi_out_ring_t;
static midi_out_ring_t midi_out_audio, midi_out_ui;
static volatile uint8_t in_render;

static uint32_t midi_length(uint32_t status)
{
    if (status >= 0xF8u || status == 0xF6u)
        return 1;
    if ((status & 0xE0u) == 0xC0u || status == 0xF1u || status == 0xF3u)
        return 2;
    return 3;
}

void smk_app_midi_in(unsigned source, const uint8_t *m, unsigned n)
{
    if (!m || !n || n > 3 || m[0] < 0x80u)
        return;
    uint32_t h = midi_in_head;
    if (h - midi_in_tail >= MIDI_RING)
        return;                                 /* full: dropped (the audio reads them every 2.9 ms) */
    uint32_t status = m[0];
    uint32_t cin = status >= 0xF8u ? 0x0Fu : (status >= 0xF0u ? (n == 2 ? 0x02u : 0x03u) : (status >> 4));
    uint32_t pkt = cin | status << 8 | (n > 1 ? (uint32_t)m[1] << 16 : 0u) | (n > 2 ? (uint32_t)m[2] << 24 : 0u);
    midi_in_ring[h % MIDI_RING].pkt = pkt;
    midi_in_ring[h % MIDI_RING].t24 = smk_ticks24();
    midi_in_ring[h % MIDI_RING].port = (uint8_t)(source == FM1_MIDI_USB ? PORT_USB
                                                : source == FM1_MIDI_BLE ? PORT_BLE : PORT_TRS);
    __asm__ volatile("" ::: "memory");
    midi_in_head = h + 1u;
}

int plat_midi_in(uint32_t *pkt, int *port)
{
    uint32_t t = midi_in_tail;
    if (t == midi_in_head)
        return 0;
    *pkt = midi_in_ring[t % MIDI_RING].pkt;
    if (port)
        *port = midi_in_ring[t % MIDI_RING].port;
    midi_in_last_t24 = midi_in_ring[t % MIDI_RING].t24;
    __asm__ volatile("" ::: "memory");
    midi_in_tail = t + 1u;
    return 1;
}

uint32_t plat_midi_in_t24(void) { return midi_in_last_t24; }

/* Every message goes to USB and to the MIDI OUT jack (the T-8 takes the clock, the start and the stop there;
 * track 2's notes on channel 2 play its bass). Called from the render or from the UI: two queues, one writer each. */
void plat_midi_send(uint32_t pkt, uint32_t to)
{
    (void)to;
    midi_out_ring_t *q = in_render ? &midi_out_audio : &midi_out_ui;
    uint32_t h = q->head;
    if (h - q->tail >= MIDI_RING)
        return;
    q->pkt[h % MIDI_RING] = pkt;
    q->sent[h % MIDI_RING] = 0;
    __asm__ volatile("" ::: "memory");
    q->head = h + 1u;
}

static int flush_ring(midi_out_ring_t *q, unsigned ports)
{
    while (q->tail != q->head) {
        uint32_t t = q->tail;
        uint32_t pkt = q->pkt[t % MIDI_RING];
        uint8_t m[3] = {(uint8_t)(pkt >> 8), (uint8_t)(pkt >> 16), (uint8_t)(pkt >> 24)};
        uint32_t n = midi_length(m[0]);
        unsigned todo = ports & ~q->sent[t % MIDI_RING];
        for (unsigned p = 0; p < 3; p++) {
            if (!(todo & (1u << p)))
                continue;
            int r = fm1_midi_send_port(p, m, n);
            if (r == -2)
                return 0;                       /* this port is full: the rest waits, in order */
            q->sent[t % MIDI_RING] |= (uint8_t)(1u << p);   /* sent, or a port that is gone: either way, done */
        }
        q->tail = t + 1u;
    }
    return 1;
}

void smk_app_midi_flush(void)
{
    unsigned ports = fm1_midi_ready_ports() & ((1u << FM1_MIDI_DIN) | (1u << FM1_MIDI_USB));
    if (!ports) {
        midi_out_audio.tail = midi_out_audio.head;   /* nobody listens: nothing piles up */
        midi_out_ui.tail = midi_out_ui.head;
        return;
    }
    flush_ring(&midi_out_audio, ports);         /* the clock first */
    flush_ring(&midi_out_ui, ports);
}

/* --- storage: the app's objects in the SDK's user partition, two sectors each (A/B), CRC'd, torn-write safe --- */

/* M-VAVE Open keeps 16 project slots, its settings and a staging sector at the start of the partition. We use the
 * sectors of its project slots 6..15 (5 objects x 2): Open's projects 1..5, settings and samples are left alone.
 * OBJ_PANEL is the FM-1 platform's calibration: none here, so it takes no sectors. */
#define STORE_FIRST_SECTOR 5u
#define STORE_MAGIC 0x4F4C5353u                 /* "OLSS" */
typedef struct {
    uint32_t magic;
    uint16_t obj, len;
    uint32_t seq;
    uint32_t crc;
} store_head_t;

static uint32_t crc32_step(uint32_t c, const uint8_t *p, uint32_t n)
{
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++)
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return c;
}

#define STORE_OBJECTS (OBJ_NOBJ - 1u)
static uint32_t store_sector(uint32_t obj, uint32_t copy)
{
    uint32_t index = obj > OBJ_PANEL ? obj - 1u : obj;
    return (STORE_FIRST_SECTOR + index * 2u + copy) * FM1_FLASH_SECTOR_BYTES;
}

/* the copy's head if it holds a good copy of obj (its sequence number, else 0); no RAM buffer: read in pieces */
static uint32_t store_check(uint32_t obj, uint32_t copy, store_head_t *h)
{
    uint8_t piece[128];
    uint32_t at = store_sector(obj, copy);
    if (fm1_flash_read(at, h, sizeof *h) || h->magic != STORE_MAGIC || h->obj != obj || h->len > PLAT_STORE_MAX
        || !h->seq)
        return 0;
    uint32_t c = 0xFFFFFFFFu;
    for (uint32_t done = 0; done < h->len;) {
        uint32_t n = h->len - done < sizeof piece ? h->len - done : (uint32_t)sizeof piece;
        if (fm1_flash_read(at + (uint32_t)sizeof *h + done, piece, n))
            return 0;
        c = crc32_step(c, piece, n);
        done += n;
    }
    return ~c == h->crc ? h->seq : 0;
}

int plat_store_load(uint32_t obj, void *dst, uint32_t max)
{
    store_head_t ha, hb;
    if (obj >= OBJ_NOBJ || obj == OBJ_PANEL)
        return -1;
    uint32_t a = store_check(obj, 0, &ha), b = store_check(obj, 1, &hb);
    if (!a && !b)
        return -1;
    uint32_t copy = a > b ? 0u : 1u;
    const store_head_t *h = copy ? &hb : &ha;
    uint32_t n = h->len < max ? h->len : max;
    if (fm1_flash_read(store_sector(obj, copy) + (uint32_t)sizeof *h, dst, n))
        return -1;
    return (int)n;
}

int plat_store_save(uint32_t obj, const void *src, uint32_t len)
{
    store_head_t ha, hb;
    if (obj >= OBJ_NOBJ || obj == OBJ_PANEL || len > PLAT_STORE_MAX || fm1_flash_size() < store_sector(OBJ_NOBJ, 0))
        return -1;
    uint32_t a = store_check(obj, 0, &ha), b = store_check(obj, 1, &hb);
    uint32_t target = (a <= b) ? 0u : 1u;       /* the older (or broken) copy is the one rewritten */
    store_head_t h = {STORE_MAGIC, (uint16_t)obj, (uint16_t)len, (a > b ? a : b) + 1u, 0};
    h.crc = ~crc32_step(0xFFFFFFFFu, src, len);
    uint32_t at = store_sector(obj, target);
    if (fm1_flash_erase(at) || fm1_flash_write(at + (uint32_t)sizeof h, src, len))
        return -2;
    return fm1_flash_write(at, &h, sizeof h) ? -2 : 0;   /* the head last: a copy counts once complete */
}

/* SAVE writes several flash objects; the SDK pauses the audio around each: one pause around them all is one short
 * clean silence (board/audio_ols.c counts the nested pauses); a setting kept at once is written the same way */
void fm1_audio_suspend(void);
void fm1_audio_resume(void);
#define APP_SAVE_BEGIN() fm1_audio_suspend()
#define APP_SAVE_END() fm1_audio_resume()
#define APP_SAVE_ONE_GO 1                       /* SAVE ALL: the sound faded out, every object written at once, back in:
                                                 * one short clean silence (in steps it stuttered) */

/* MODE held a moment (no pad with it): the pads' map in place of the scope (smk_pad_map, below) */
static int smk_pad_map(void);
#define APP_SCOPE_OVERLAY() smk_pad_map()

/* ------------------------------------------------------------------ the app (OpenVALive CORE) */

#include "../../va/app/openvalive_app.c"

/* ------------------------------------------------------------------ what the SMK-37 adds */

/* physical button -> the app's button (the panel prints the same names; as on the FM-1, FX holds the tracks layer,
 * SEQ the perform layer and SELECT turns the pages: openvalive_app.h BTN_*); 0xFF: the SMK-37's own */
static const uint8_t BUTTON_MAP[SMK_BUTTONS] = {
    0xFF, 0xFF, 0xFF, 0xFF, B_OCTDN, B_OCTUP, B_ARP, B_SEL, B_ENV, B_LFO,
    B_EDIT, B_GLO, B_HOME, B_SAVE, B_FX, B_SEQ, B_PLAY, B_REC
};
/* FM-1 button -> the physical button whose LED shows it */
static const uint8_t LED_OF[NB] = {
    [B_FX] = SMK_B_FX, [B_SEL] = SMK_B_SELECT, [B_ENV] = SMK_B_ENV, [B_LFO] = SMK_B_LFO, [B_EDIT] = SMK_B_EDIT,
    [B_GLO] = SMK_B_GLO, [B_HOME] = SMK_B_HOME, [B_SAVE] = SMK_B_SAVE, [B_ARP] = SMK_B_ARP, [B_SEQ] = SMK_B_SEQ,
    [B_PLAY] = SMK_B_PLAY, [B_REC] = SMK_B_REC, [B_OCTDN] = SMK_B_OCT_DOWN, [B_OCTUP] = SMK_B_OCT_UP
};
#define SMK_LED_PLAY_ALT 18u                    /* PLAY's second LED */
/* physical encoder -> FM-1 encoder (7: MASTER) */
static const int8_t ENCODER_MAP[SMK_ENCODERS] = { EN_SELECT, EN_PRESET, EN_K1, EN_K2, EN_K3, EN_K4, EN_ALGO, -1 };

#define FIRST_SMK_NOTE 48                       /* the leftmost key is C3 */
#define LAYER_BUTTONS ((1u << B_GLO) | (1u << B_ARP) | (1u << BTN_TRACKS) | (1u << B_SAVE) | (1u << BTN_PERFORM) \
                       | (1u << B_HOME) | (1u << BTN_PAGES))

/* The pads: the two on the right are set-up pads, MODE (top right) and REC (bottom right); the 14 others play the
 * mode MODE (or PAD/STEP) goes round: CLIPS, DRUMS, PERFORM, SCENES, MIDI. */
enum { PADS_CLIPS, PADS_DRUMS, PADS_PERFORM, PADS_LIVE2, PADS_SCENES, PADS_MIDI, PAD_MODES };   /* PERFORM is
                                                 * LIVE 1: LIVE 1 and LIVE 2 are the pads played all the time */
#define LIVE_MODE(m) ((m) == PADS_PERFORM || (m) == PADS_LIVE2)   /* REC is their switch, green */
#define PAD_MODE 7u                             /* the top right pad */
#define PAD_REC 15u                             /* the bottom right pad */
#define PLAY_PADS 14u
#define SCENE_PADS 8u                           /* SCENES: 1-8 the scenes, 9-12 the tracks' mutes, 13 all, 14 A VOL */
#define MIDI_PAD_CHANNEL 15u                    /* MIDI: the pads' notes on channel 16, no sound inside */
/* PERFORM on 14 pads: the 16 keys' effects but DRIVE and DRUMS ONLY */
static const uint8_t PERFORM_PAD[PLAY_PADS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 14, 15 };
enum { FADERS_LEVEL, FADERS_MACRO, FADERS_USER, FADER_BANKS };   /* FADER BANK goes round */
#define LONG_PRESS_MS 500u

static struct {
    uint32_t phys;                              /* physical buttons held */
    uint8_t pad_mode, fader_mode;
    uint32_t master_q12;
    uint8_t key_note[SMK_KEYS], key_track[SMK_KEYS], key_slot[SMK_KEYS], key_held_by_pedal[SMK_KEYS];
    uint8_t pad_note[SMK_PADS], pad_track[SMK_PADS], pad_slot[SMK_PADS], pad_press[SMK_PADS];
    uint8_t pedal;
    uint16_t pad_down;                          /* pads held */
    uint8_t pad_vel[SMK_PADS], roll_level[SMK_PADS];  /* a drum pad's roll: its velocity, how fast (0: none) */
    uint8_t roll_want[SMK_PADS];                /* how fast the pressure asks for: reached a step at a time */
    uint32_t roll_step_at[SMK_PADS];            /* when the roll last went a step faster */
    uint32_t roll_at[SMK_PADS];                 /* the grid line it last hit on */
    uint64_t key_down;                          /* keys held */
    uint8_t fader_seen[4];
    uint32_t leds_shown;
    uint32_t bt_down_at;                        /* the Bluetooth button: a tap = TAP, held = the CONTROL MAP */
    uint8_t bt_long;
    uint8_t booted;
    uint32_t last_frame;
    int8_t rec_target;                          /* -2: REC served a pad (letting it go does nothing more) */
    uint8_t mode_used;                          /* MODE held served a pad: letting it go does not change the mode */
    uint8_t mode_map;                           /* MODE held long: its map shown, letting it go changes nothing */
    uint32_t mode_down_at;                      /* when MODE was pressed */
    uint32_t wave_t0;                           /* the light going round the pads: when it started (top left) */
    uint32_t map_until;                         /* a mode just taken: its pads' map shown until then (or a touch) */
    uint8_t wave_on;                            /* it is going round (the logo, the help pages) */
    uint32_t pads_quiet_until;                  /* after it: the pads stay off and do nothing until then */
    uint16_t pad_ignored;                       /* pads pressed then: their release does nothing either */
} smk = { .master_q12 = 2867u, .pad_mode = PADS_PERFORM, .rec_target = -1 };

static int shift_held(void) { return (smk.phys >> SMK_B_SHIFT) & 1u; }
static int layer_held(void) { return (v_buttons & LAYER_BUTTONS) != 0; }

/* white key w (0..) of the SMK-37's keyboard, or -1 for a black key */
static int smk_white_index(uint32_t id)
{
    static const int8_t IN_OCTAVE[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
    int w = IN_OCTAVE[id % 12u];
    return w < 0 ? -1 : (int)(id / 12u) * 7 + w;
}

/* black key b (0..) of the SMK-37's keyboard, or -1 for a white key */
static int smk_black_index(uint32_t id)
{
    static const int8_t IN_OCTAVE[12] = { -1, 0, -1, 1, -1, -1, 2, -1, 3, -1, 4, -1 };
    int b = IN_OCTAVE[id % 12u];
    return b < 0 ? -1 : (int)(id / 12u) * 5 + b;
}

/* a key of the keyboard: the app key (layer slot) it stands for, -1 none (the layers are on the first 16 white
 * keys, and the first black ones in order: the 1st MUTE ALL, the 4th A VOL under SEL, as on the FM-1) */
static int32_t smk_screen_key(uint32_t i)
{
    int w = smk_white_index(i), b = smk_black_index(i);
    if (b >= 0)
        return b < BLACK_KEYS ? (int32_t)BLACK_KEY[b] : -1;
    return w >= 0 && w < WHITE_KEYS ? (int32_t)WHITE_KEY[w] : -1;
}

static uint32_t smk_key_layer(uint32_t k)
{
    if ((v_keys_scenes >> k) & 1u)
        return 1u << B_GLO;                     /* a pad in SCENES mode: a scene to recall or store */
    if ((v_keys_tracks >> k) & 1u)
        return 1u << BTN_TRACKS;                /* ...or a track to mute */
    return (v_keys_perform >> k) & 1u ? (1u << BTN_PERFORM) : 0u;   /* a pad in PERFORM mode: the perform effects */
}

/* --- notes (keys, and pads in PLAY mode): straight to the selected track, out over MIDI too --- */

/* where a key or a pad plays: A's own voice when A is selected (live, not recorded), else the selected track */
static uint8_t play_target(void)
{
    return ui.on_a ? T_LIVE : mirror.track;
}

/* a line's MIDI channel (0..15): tracks 1-4, A 5 (as the FM-1), the pads' drum kit 10 (GM drums) */
static uint32_t line_channel(uint8_t track)
{
    return track == T_LIVE ? TRACKS : (track == T_PADS ? 9u : track);
}

static void note_on(uint8_t track, uint32_t note, uint32_t velocity)
{
    if (!set.controller)
        send_note_on(track, (uint8_t)note, (uint8_t)velocity);
    midi_out_note_on(line_channel(track), note, velocity);
}

static void note_off(uint8_t track, uint32_t note)
{
    send_note_off(track, (uint8_t)note);
    midi_out_note_off(line_channel(track), note);
}

/* The keys' velocity, curved: soft playing gets more of the range (the scanner's is linear and feels hard; the
 * FM-1's touch keys feel livelier). Half linear, half square root: 32 -> 48, 64 -> 77, 100 -> 106, 127 -> 127. */
static uint32_t isqrt32(uint32_t x)
{
    uint32_t r = 0, b = 1u << 14;
    while (b) {
        if ((r + b) * (r + b) <= x)
            r += b;
        b >>= 1;
    }
    return r;
}

static int key_velocity(int v)
{
    if (v <= 0)
        return 0;
    if (v > 127)
        v = 127;
    return (v + (int)isqrt32((uint32_t)v * 127u) + 1) / 2;
}

static void key_event(uint32_t id, int velocity)
{
    velocity = key_velocity(velocity);
    if (id >= SMK_KEYS)
        return;
    uint64_t bit = (uint64_t)1u << id;
    if (!velocity && !(smk.key_down & bit))
        return;
    smk.key_down = velocity ? (smk.key_down | bit) : (smk.key_down & ~bit);
    if (velocity) {
        ui.touched = 1;                         /* (the boot screen goes) */
        int32_t slot = smk_screen_key(id);
        if (layer_held() && slot >= 0) {        /* layer held: the white keys are its 16 slots, the black ones the
                                                 * FM-1's black keys in order */
            smk.key_slot[id] = (uint8_t)slot;
            slot_down((uint32_t)slot, (uint32_t)velocity);
            return;
        }
        if (smk.key_held_by_pedal[id]) {          /* struck again under the pedal: the old one ends first */
            note_off(smk.key_track[id], smk.key_note[id]);
            smk.key_held_by_pedal[id] = 0;
        }
        smk.key_slot[id] = 0xFF;
        uint32_t key_note = (uint32_t)clamp(FIRST_SMK_NOTE + (int32_t)id + 12 * *line_octave(), 0, 127);
        smk.key_note[id] = app_play_note(key_note);
        smk.key_track[id] = play_target();
        note_on(smk.key_track[id], smk.key_note[id], app_play_vel(key_note, (uint32_t)velocity));
        ui.redraw = 1;
        return;
    }
    if (smk.key_slot[id] != 0xFF) {
        slot_up(smk.key_slot[id]);
        smk.key_slot[id] = 0xFF;
    } else if (smk.pedal) {
        smk.key_held_by_pedal[id] = 1;            /* the pedal holds it */
    } else {
        note_off(smk.key_track[id], smk.key_note[id]);
    }
}

static void pedal_event(int value)
{
    smk.pedal = value >= 64;
    if (smk.pedal)
        return;
    for (uint32_t id = 0; id < SMK_KEYS; id++) {
        if (smk.key_held_by_pedal[id]) {
            smk.key_held_by_pedal[id] = 0;
            note_off(smk.key_track[id], smk.key_note[id]);
        }
    }
}

/* the 14 playing pads, 0..13: the top row then the bottom one, MODE and REC left out; -1 for those two */
static int play_index(uint32_t pad)
{
    if (pad == PAD_MODE || pad == PAD_REC || pad >= SMK_PADS)
        return -1;
    return pad < PAD_MODE ? (int)pad : (int)pad - 1;
}

/* DRUMS and MIDI: a note per pad, from C2 (36) on the bottom row's left, as drum machines (the top row goes on) */
static uint32_t pad_drum_note(int i)
{
    return i >= 7 ? 36u + (uint32_t)(i - 7) : 43u + (uint32_t)i;
}

static void clip_send(uint8_t what, uint32_t slot, uint8_t line)
{
    push_event(EV_CLIP, line, what, (uint8_t)slot, 0, 0.0f);
}

static void clip_say(const char *what, int slot, const char *how)
{
    char t[12];
    copy_text(t, what, sizeof t);
    text_cat(t, " ");
    int_text(slot + 1, t + strlen(t));
    app_say(t, how);
}

/* a take armed or running: it ends (a pad, MODE, REC or a button); 1 if there was one */
static int clip_take_end(void)
{
    if (crec.slot < 0)
        return 0;
    clip_send(CLIP_END, 0, 0);
    return 1;
}

/* REC + a pad, REC let go first: taken. CLIPS: the pad records (from the first note played, on the selected line);
 * SCENES 1-8: the scene stored. */
static void rec_taken(int i)
{
    if (smk.pad_mode == PADS_DRUMS) {           /* the pads' kit: any of the five, the hidden ones too; kept */
        set.pad_kit = (uint8_t)i;
        push_event(EV_PRESET, T_PADS, (uint8_t)i, 0, 0, 0.0f);
        mark_dirty();
        app_say("PAD KIT", ols_sound_name(OLS_SOUND_FIRST_DRUM + i));
        ui.msg_style = MSG_TAKEN;
        ui.msg_until = plat_ms() + 900u;
        return;
    }
    if (smk.pad_mode == PADS_CLIPS) {
        clip_send(CLIP_ARM, (uint32_t)i, play_target());
        clip_say("CLIP", i, "PLAY TO REC");
        ui.msg_until = plat_ms() + 3000u;
        return;
    }
    scene_key(8 + i);                           /* "STORED SCENE n", in green */
    ui.msg_style = MSG_TAKEN;
    ui.msg_until = plat_ms() + 700u;
}

static void rec_say(int i)                      /* what REC + this pad does: "SAVE 3", "REPLACE 2", "DRUMS 909" */
{
    if (smk.pad_mode == PADS_DRUMS) {
        app_say(ols_sound_name(OLS_SOUND_FIRST_DRUM + i), "");
        return;
    }
    clip_say(smk.pad_mode == PADS_SCENES ? "SAVE" : (clips[i].used ? "REPLACE" : "CLIP"), i, "");
}

/* a pad mode just taken: its map on the screen a few seconds, until anything is touched (smk_input_event) */
#define MAP_SHOW_MS 4000u
static void pads_map_show(void)
{
    smk.map_until = fm1_millis() + MAP_SHOW_MS;
    ui.msg_until = smk.map_until;
    ui.redraw = 1;
}

/* the set-up pads: MODE (a tap: the next mode; held: the pads' map) and REC (held + a pad: record / store / choose,
 * at once; CLIPS: a clip pad held, then REC: LOOP) */
static void setup_pad(uint32_t pad, int down)
{
    if (pad == PAD_MODE) {
        if (down) {
            smk.mode_down_at = fm1_millis();
            smk.mode_map = 0;
            smk.mode_used = (uint8_t)clip_take_end();
            return;
        }
        if (smk.mode_used || smk.mode_map) {    /* (a pad with it, or its map looked at: no change) */
            smk.mode_map = 0;
            ui.redraw = 1;
            return;
        }
        static const char *const NAME[PAD_MODES] = { "CLIPS", "DRUMS", "LIVE 1 LOOP", "LIVE 2 MUTE", "SCENES", "MIDI CH 16" };
        smk.pad_mode = (uint8_t)((smk.pad_mode + 1u) % PAD_MODES);
        app_say("PADS", NAME[smk.pad_mode]);
        pads_map_show();
        return;
    }
    if (down) {                                 /* REC */
        if (clip_take_end()) {
            app_say("CLIP", "TAKEN");
            smk.rec_target = -2;                /* (letting REC go then does nothing more) */
            return;
        }
        if (smk.pad_mode == PADS_CLIPS) {       /* a clip pad held, then REC: that clip goes on looping (LOOP, in
                                                 * time: from the next beat) */
            int looped = 0;
            for (uint32_t p = 0; p < SMK_PADS; p++) {
                int c = play_index(p);
                if (c >= 0 && ((smk.pad_down >> p) & 1u) && clips[c].used) {
                    clip_send(clips[c].playing ? CLIP_LATCH : CLIP_PLAY_SYNC, (uint32_t)c, 0);
                    clip_say("LOOP", c, "");
                    looped = 1;
                }
            }
            if (looped) {
                smk.rec_target = -2;
                return;
            }
        }
        if (LIVE_MODE(smk.pad_mode)) {          /* LIVE 1 / LIVE 2: REC is their switch (nothing to record there) */
            static const char *const LIVE_NAME[2] = { "LIVE 1 LOOP", "LIVE 2 MUTE" };
            smk.pad_mode = smk.pad_mode == PADS_PERFORM ? PADS_LIVE2 : PADS_PERFORM;
            app_say("PADS", LIVE_NAME[smk.pad_mode == PADS_LIVE2]);
            pads_map_show();
            smk.rec_target = -2;
            return;
        }
        smk.rec_target = -1;
        if (smk.pad_mode == PADS_DRUMS) {       /* the kits on the first five pads, the one playing bright */
            app_say("PAD KIT", ols_sound_name(OLS_SOUND_FIRST_DRUM + set.pad_kit));
            ui.msg_until = plat_ms() + 60000u;  /* (while REC is held) */
        } else if (smk.pad_mode == PADS_CLIPS || smk.pad_mode == PADS_SCENES) {
            app_say("REC", smk.pad_mode == PADS_CLIPS ? "+ PAD: RECORD" : "+ PAD: SAVE");
        }
        return;
    }
    int i = smk.rec_target;
    smk.rec_target = -1;
    if (i < 0) {
        if (smk.pad_mode == PADS_DRUMS)
            ui.msg_until = plat_ms();           /* (the kit's name goes with REC) */
        return;
    }
    rec_taken(i);
}

/* LIVE 2: what is needed at once while playing, one pad each. Top: MUTE 1-4 (the tracks' own mutes, as SELECT + key),
 * MUTE ALL, RESTART, CLEAR TRACK. Bottom: LOOP REC (as EDIT tapped), UNDO, REDO, MOD (what the mod wheel does: MORPH,
 * FILTER, DELAY, CRUSH, DRIVE, PAN of the selected track, in turn), A VOL, 1/2 (the selected track's loop half as
 * long: its first half again; ENV undoes it), PAGE (the knobs' next page, as FX tapped, in that page's colour). REC
 * (bottom right) goes back to LIVE 1. */
enum { L2_M1, L2_M2, L2_M3, L2_M4, L2_MUTE_ALL, L2_RESTART, L2_CLEAR, L2_CAPTURE, L2_UNDO, L2_REDO, L2_MOD,
       L2_AVOL, L2_HALVE, L2_PAGE };

/* MOD: what the mod wheel does, in turn (the CONTROL MAP's mod wheel slot); its pad shows it in its colour */
static const struct { uint8_t target; uint32_t rgb; const char *name; } MOD_WHEEL[] = {
    { MAP_MORPH, 0x9030FF, "MORPH" }, { MAP_FILTER, 0xFF6000, "FILTER" }, { MAP_DELAY, 0xFFC000, "DELAY" },
    { MAP_CRUSH, 0xFF2080, "CRUSH" }, { MAP_DRIVE, 0xFF1010, "DRIVE" }, { MAP_PAN, 0x00C0FF, "PAN" },
};
#define MOD_WHEEL_N (sizeof MOD_WHEEL / sizeof MOD_WHEEL[0])

static int mod_wheel_now(void)                  /* its place in MOD_WHEEL, -1: something else (set in MAP CONTROL) */
{
    for (uint32_t k = 0; k < MOD_WHEEL_N; k++)
        if (cmap.target[MAP_MOD] == MOD_WHEEL[k].target)
            return (int)k;
    return -1;
}

static void mod_wheel_next(void)
{
    uint32_t k = (uint32_t)(mod_wheel_now() + 1) % MOD_WHEEL_N;
    map_retarget(MAP_MOD, MOD_WHEEL[k].target);
    mark_dirty();
    app_say("MOD WHEEL", MOD_WHEEL[k].name);
}

/* the knob pages' colours, as the screen shows them (ui_draw.c PAGE_COLOUR) */
static const uint32_t PAGE_RGB[PAGE_COUNT] = { 0xC0C0C0, 0x00C0FF, 0x20FFA0, 0xFF2080, 0x9030FF, 0xFF1010, 0xFF6000 };

static void live2_halve(void)
{
    uint32_t len = live.track[mirror.track].pat.length;
    if (ui.on_a || !live.track[mirror.track].pat.count) {
        app_say("1/2", ui.on_a ? "PICK A TRACK" : "NO LOOP");
        return;
    }
    if (len / 2u < OLS_PPQN) {
        app_say("1/2", "ONE BEAT ALREADY");
        return;
    }
    send_live(LIVE_LENGTH, 0, (uint16_t)(len / 2u), 0.0f, mirror.track);
    say_undo("LOOP 1/2");
}

static void live2_pad(int i)
{
    switch (i) {
    case L2_M1: case L2_M2: case L2_M3: case L2_M4:
        track_key(i);
        break;
    case L2_MUTE_ALL:
        mute_all_key();
        break;
    case L2_RESTART:
        home_key(1);
        break;
    case L2_CLEAR:
        track_key(5);                           /* the selected track (on A: every track); ENV undoes it */
        break;
    case L2_CAPTURE:                            /* as EDIT tapped: LOOP REC armed, again: taken */
        if (ui.cap_armed) {
            take_end();
        } else {
            ui.edit_used = 0;
            edit_released();
        }
        break;
    case L2_UNDO:
        button_pressed(B_ENV, ui.buttons);
        break;
    case L2_REDO:
        button_pressed(B_LFO, ui.buttons);
        break;
    case L2_MOD:
        mod_wheel_next();
        break;
    case L2_HALVE:
        live2_halve();
        break;
    case L2_PAGE:
        ui.pages_used = 0;
        pages_released();                       /* as FX tapped: the knobs' next page */
        app_say("PAGE", PAGE_NAME[set.page]);
        break;
    case L2_AVOL:
        avol_key();
        break;
    default:
        break;
    }
}

static uint32_t dim(uint32_t rgb, uint32_t level);
static uint32_t live2_colour(int i, int held)
{
    if (i <= L2_M4)                             /* the mutes: green playing (the selected one brighter), red out */
        return live.track[i].mute ? 0x400000 : (i == mirror.track && !ui.on_a ? 0x20E020 : 0x087008);
    switch (i) {
    case L2_MUTE_ALL: {
        int all_out = 1;
        for (uint32_t k = 0; k < TRACKS; k++)
            all_out &= live.track[k].mute != 0;
        return all_out ? ((fm1_millis() / 125u) & 1u ? 0xFF0010 : 0x100000) : (held ? 0xFF2080 : 0x401020);   /* pink */
    }
    case L2_RESTART:
        return held ? 0x00FF40 : 0x00300C;
    case L2_CLEAR:                              /* white */
        return held ? 0xFFFFFF : 0x303030;
    case L2_CAPTURE:                            /* LOOP REC: red; armed / taking: blinking red; held: mint */
        if (held)
            return 0x20FFA0;
        return ui.cap_armed ? ((fm1_millis() / 125u) & 1u ? 0xFF0000 : 0x200000) : 0x300000;
    case L2_UNDO:
        return ols_live_can_undo_track(&live, mirror.track) ? 0xC08000 : 0x100800;
    case L2_REDO:
        return ols_live_can_redo_track(&live, mirror.track) ? 0xC08000 : 0x100800;
    case L2_MOD: {                              /* in the colour of what the mod wheel does */
        int k = mod_wheel_now();
        uint32_t c = k < 0 ? 0x808080 : MOD_WHEEL[k].rgb;
        return held ? c : dim(c, 70u);
    }
    case L2_HALVE:                              /* cyan, as the loops */
        return held ? 0x00C0FF : (live.track[mirror.track].pat.count && !ui.on_a ? 0x003040 : 0x000810);
    case L2_PAGE:                               /* the knobs' page now, in its colour */
        return held ? PAGE_RGB[set.page % PAGE_COUNT] : dim(PAGE_RGB[set.page % PAGE_COUNT], 70u);
    default:
        return avol.on ? 0x00FF60 : 0x002810;   /* A VOL: lit green while it runs */
    }
}

#define PADS_QUIET_MS 500u

static void pad_event(uint32_t pad, int velocity)
{
    if (pad >= SMK_PADS)
        return;
    int was_down = (smk.pad_down >> pad) & 1u;
    smk.pad_down = (uint16_t)(velocity ? (smk.pad_down | (1u << pad)) : (smk.pad_down & ~(1u << pad)));
    if (!velocity && !was_down)
        return;
    if (velocity && (smk.wave_on || ui.splash_until || ui.help
                     || (int32_t)(fm1_millis() - smk.pads_quiet_until) < 0)) {
        ui.touched = 1;                         /* the light going round (logo, help): a pad ends both, does nothing */
        ui.help = 0;
        smk.pad_ignored |= (uint16_t)(1u << pad);
        return;
    }
    if (!velocity && ((smk.pad_ignored >> pad) & 1u)) {
        smk.pad_ignored &= (uint16_t)~(1u << pad);
        return;                                 /* (its press did nothing) */
    }
    if (velocity)
        ui.touched = 1;                         /* (the boot screen goes) */
    int i = play_index(pad);
    if (i < 0) {
        smk.pad_slot[pad] = 0xFE;               /* (no note, no roll, no pressure) */
        smk.roll_level[pad] = 0;
        smk.roll_want[pad] = 0;
        setup_pad(pad, velocity != 0);
        return;
    }
    if (velocity) {
        smk.pad_press[pad] = 0;
        smk.pad_slot[pad] = 0xFE;               /* (most modes: its release does nothing) */
        if ((smk.pad_down >> PAD_MODE) & 1u)
            smk.mode_used = 1;
        int rec_held = (smk.pad_down >> PAD_REC) & 1u;
        if (rec_held && smk.rec_target < 0
            && (smk.pad_mode == PADS_CLIPS || (smk.pad_mode == PADS_SCENES && i < (int)SCENE_PADS)
                || (smk.pad_mode == PADS_DRUMS && i < DR_KIT_COUNT))) {
            smk.rec_target = -2;                /* REC + pad: at once (letting REC go then does nothing more) */
            rec_taken(i);
            return;
        }
        if (smk.pad_mode == PADS_CLIPS) {
            if (crec.slot >= 0) {               /* a take armed or running: any pad ends it; its own plays it */
                int own = crec.slot == i && crec.started;
                clip_take_end();
                if (own) {
                    clip_send(CLIP_PLAY, (uint32_t)i, 0);
                    clip_say("CLIP", i, "PLAYS");
                } else {
                    app_say("CLIP", "TAKEN");
                }
                return;
            }
            if (shift_held()) {                 /* SHIFT + pad: the clip deleted */
                if (clips[i].used) {
                    clip_send(CLIP_DELETE, (uint32_t)i, 0);
                    clip_say("CLIP", i, "DELETED");
                }
                return;
            }
            if (!clips[i].used) {
                clip_say("CLIP", i, "EMPTY: REC+PAD");
            } else if (clips[i].playing) {
                clip_send(CLIP_STOP, (uint32_t)i, 0);    /* playing (once or LOOP): the pad again stops it */
                clip_say("CLIP", i, "STOP");
            } else {
                clip_send(CLIP_PLAY, (uint32_t)i, 0);    /* SINGLE: once, now; held + REC: LOOP */
            }
            return;
        }
        clip_take_end();                        /* (another mode: a take still armed ends too) */
        if (smk.pad_mode == PADS_SCENES) {      /* scenes and mutes, at once */
            uint32_t w;
            if (i < (int)SCENE_PADS)
                w = (uint32_t)i + (shift_held() ? 8u : 0u);    /* 1-8 recall, SHIFT + them store */
            else
                w = (uint32_t)i - SCENE_PADS;   /* the tracks' mutes, then all four, then A VOL */
            uint32_t k = i < (int)SCENE_PADS ? WHITE_KEY[w] : (w == 4u ? KEY_MUTE_ALL : (w == 5u ? KEY_AVOL : WHITE_KEY[w]));
            if ((v_keys >> k) & 1u)             /* its slot already held (another pad, a key) */
                return;
            smk.pad_slot[pad] = (uint8_t)k;
            if (i < (int)SCENE_PADS)
                v_keys_scenes |= 1u << k;
            else
                v_keys_tracks |= 1u << k;
            slot_down(k, (uint32_t)velocity);
            return;
        }
        if (smk.pad_mode == PADS_LIVE2) {       /* quick gestures, at once */
            live2_pad(i);
            return;
        }
        if (smk.pad_mode == PADS_PERFORM) {     /* 14 of the perform effects (the pads never follow a held layer) */
            uint32_t k = WHITE_KEY[PERFORM_PAD[i]];
            smk.pad_slot[pad] = (uint8_t)k;
            v_keys_perform |= 1u << k;
            slot_down(k, (uint32_t)velocity);
            return;
        }
        smk.pad_note[pad] = (uint8_t)pad_drum_note(i);
        if (smk.pad_mode == PADS_MIDI) {        /* out only, channel 16 */
            smk.pad_slot[pad] = 0xFD;
            midi_out_note_on(MIDI_PAD_CHANNEL, smk.pad_note[pad], (uint32_t)velocity);
            return;
        }
        smk.pad_slot[pad] = 0xFF;               /* DRUMS: the pads' own kit (REC held: which) */
        smk.pad_track[pad] = T_PADS;
        smk.pad_vel[pad] = (uint8_t)velocity;
        smk.roll_level[pad] = 0;
        smk.roll_want[pad] = 0;
        note_on(smk.pad_track[pad], smk.pad_note[pad], (uint32_t)velocity);
        return;
    }
    smk.roll_level[pad] = 0;
    smk.roll_want[pad] = 0;
    if (smk.rec_target == i) {                  /* the pad let go before REC: given up */
        smk.rec_target = -2;
        rec_say(i);
        ui.msg_style = MSG_NOT_TAKEN;
        ui.msg_until = plat_ms() + 700u;
    }
    if (smk.pad_slot[pad] == 0xFE) {
        smk.pad_slot[pad] = 0xFF;
    } else if (smk.pad_slot[pad] == 0xFD) {
        midi_out_note_off(MIDI_PAD_CHANNEL, smk.pad_note[pad]);
        smk.pad_slot[pad] = 0xFF;
    } else if (smk.pad_slot[pad] != 0xFF) {
        slot_up(smk.pad_slot[pad]);
        smk.pad_slot[pad] = 0xFF;
    } else {
        note_off(smk.pad_track[pad], smk.pad_note[pad]);
        if (smk.pad_press[pad]) {                 /* the pressure it gave goes back to rest */
            if (smk.pad_track[pad] != T_PADS) {
                push_event(EV_PRESSURE, smk.pad_track[pad], 0, 0, 0, 0.0f);
                midi_out_pressure(line_channel(smk.pad_track[pad]), 0);
            }
        }
    }
    smk.pad_press[pad] = 0;
}

/* --- drum rolls: a drum pad pressed harder repeats on the beat grid, 1/8, 1/16, 1/32 the harder --- */

static uint32_t roll_now(void)
{
    return live.clock.running ? live.clock.tick : live.clock.free_tick;
}

/* 0 a plain hit, 1 1/8, 2 1/16, 3 1/32; a little lower to go back down (no flicker on a threshold) */
static uint32_t roll_level(int pressure, uint32_t now_level)
{
    static const int AT[3] = { 100, 116, 126 };   /* (a hand resting on a pad presses ~60-100: no roll) */
    uint32_t l = 0;
    while (l < 3u && pressure >= AT[l])
        l++;
    if (l < now_level && pressure + 6 >= AT[now_level - 1u])
        return now_level;
    return l;
}

/* the roll goes faster a step at a time, ROLL_STEP_MS each (the pads' pressure rises in a few ms: it went from no
 * roll straight to 1/32); slower or off at once */
#define ROLL_STEP_MS 250u
static void roll_set(uint32_t pad, uint32_t l)
{
    smk.roll_level[pad] = (uint8_t)l;
    smk.roll_step_at[pad] = fm1_millis();
    if (l) {
        smk.roll_at[pad] = roll_now() / (OLS_PPQN >> l);   /* the next grid line is the first repeat */
        static const char *const RATE[3] = { "1/8", "1/16", "1/32" };
        app_say("ROLL", RATE[l - 1u]);
    }
}

static void roll_pressure(uint32_t pad, int pressure)
{
    uint32_t l = roll_level(pressure, smk.roll_want[pad]);
    smk.roll_want[pad] = (uint8_t)l;
    if (l < smk.roll_level[pad] || (l > smk.roll_level[pad] && !smk.roll_level[pad]))
        roll_set(pad, l > smk.roll_level[pad] ? 1u : l);   /* down at once; up: 1/8 first */
}

static void roll_steps(void)
{
    for (uint32_t pad = 0; pad < SMK_PADS; pad++) {
        if (smk.roll_level[pad] && smk.roll_want[pad] > smk.roll_level[pad]
            && fm1_millis() - smk.roll_step_at[pad] >= ROLL_STEP_MS)
            roll_set(pad, smk.roll_level[pad] + 1u);
    }
}

static void pad_rolls(void)
{
    roll_steps();
    uint32_t now = roll_now();
    for (uint32_t pad = 0; pad < SMK_PADS; pad++) {
        uint32_t rate = smk.roll_level[pad] ? OLS_PPQN >> smk.roll_level[pad] : 0u;
        if (!rate || !((smk.pad_down >> pad) & 1u) || smk.pad_slot[pad] != 0xFF)
            continue;
        uint32_t line = now / rate;
        if (line == smk.roll_at[pad])
            continue;
        smk.roll_at[pad] = line;
        note_off(smk.pad_track[pad], smk.pad_note[pad]);
        note_on(smk.pad_track[pad], smk.pad_note[pad], smk.pad_vel[pad]);
    }
}

/* Pad pressure: on a held filter / delay / drive / crush effect, how far it goes (a light touch goes 40 % of the
 * way, the full press all of it); on a drum pad, a roll; on a pad playing a note, the track's aftertouch (filter
 * and vibrato). */
static void pressure_event(uint32_t pad, int value)
{
    if (pad >= SMK_PADS || value < 0 || play_index(pad) < 0)
        return;                                 /* (MODE and REC: no pressure) */
    int moved = value - smk.pad_press[pad];
    if (moved > -3 && moved < 3 && value != 0)
        return;
    smk.pad_press[pad] = (uint8_t)value;
    float p = (float)value * (1.0f / 127.0f);
    if (smk.pad_slot[pad] == 0xFE || smk.pad_slot[pad] == 0xFD)
        return;                                 /* (a set-up pad, a clip, a MIDI pad) */
    if (smk.pad_slot[pad] != 0xFF) {
        v_press[smk.pad_slot[pad]] = (uint8_t)value;
        int w = white_index(smk.pad_slot[pad]);
        if (w >= 0 && w < FX_KEYS && PERFORM_KEY[w].type == FX_ROLE && ui.fx_on[w]
            && ((v_keys_perform >> smk.pad_slot[pad]) & 1u)) {
            float depth = 0.4f + 0.6f * p;
            float v = 0.5f + (PERFORM_KEY[w].value - 0.5f) * depth;
            send_live(LIVE_OVERRIDE, 1, PERFORM_KEY[w].arg, v, ui.fx_target[w]);   /* (the effect's own track:
                                                 * letting it go releases that one) */
        }
        return;
    }
    uint8_t t = smk.pad_track[pad];
    if (t == T_PADS || (t < TRACKS && live.track[t].kind == OLS_KIND_DRUM)) {
        roll_pressure(pad, value);
        return;
    }
    if (!set.controller)
        push_event(EV_PRESSURE, t, 0, 0, 0, p);                 /* (A: its own voice) */
    midi_out_pressure(line_channel(t), (uint32_t)value);
}

/* Faders: the four track levels, the four macros ENERGY SPACE TENSION DENSITY, or (USER) what the CONTROL MAP
 * gives them; FADER BANK goes round the three. The first reading after power-on only learns where a fader sits:
 * nothing jumps at boot. The wheels follow the CONTROL MAP (by default: pitch bend, MORPH). */
static void fader_event(uint32_t id, int value)
{
    float v = (float)value * (1.0f / 127.0f);
    if (id < 4) {
        if (!smk.fader_seen[id]) {
            smk.fader_seen[id] = 1;
            return;
        }
        if (smk.fader_mode == FADERS_USER) {
            map_touch(MAP_F1 + id);
            map_set(MAP_F1 + id, v);
        } else if (smk.fader_mode == FADERS_MACRO) {
            ui.macro[id] = v;
            send_live(LIVE_MACRO, (uint8_t)id, 0, v, 0);
            say_int(MACRO_NAME[id], (int32_t)(v * 100.0f + 0.5f), "%");
        } else {
            ui.level[id] = v * 1.25f;             /* unity at 80 % of the travel, some headroom above */
            send_live(LIVE_GAIN, 0, 0, ui.level[id], (uint8_t)id);
            char t[8] = "LEVEL ";
            t[6] = (char)('1' + id);
            t[7] = 0;
            say_int(t, (int32_t)(ui.level[id] * 100.0f + 0.5f), "%");
        }
        mark_dirty();
        return;
    }
    if (id == 4) {                                /* pitch wheel: 64 (its rest) is exactly the middle */
        float c = value >= 64 ? 0.5f + (float)(value - 64) * (0.5f / 63.0f) : (float)value * (0.5f / 64.0f);
        if (set.wheel_pan && !ui.on_a) {          /* PAN mode: up = left, down = right, at rest the centre */
            send_live(LIVE_PAN, 0, 0, (0.5f - c) * 2.0f, mirror.track);
            return;
        }
        map_touch(MAP_PITCH);
        map_set(MAP_PITCH, c);
        return;
    }
    if (id == 5) {
        map_touch(MAP_MOD);
        map_set(MAP_MOD, v);
        ui.redraw = 1;
    }
}

static void encoder_event(uint32_t id, int delta)
{
    if (id >= SMK_ENCODERS || !delta)
        return;
    if (ENCODER_MAP[id] < 0) {                    /* MASTER */
        int32_t q = (int32_t)smk.master_q12 + delta * 64;
        smk.master_q12 = (uint32_t)clamp(q, 0, 4096);
        smk_audio_set_master(smk.master_q12);
        say_int("MASTER", (int32_t)((smk.master_q12 * 100u + 2048u) / 4096u), "%");
        return;
    }
    int role = ENCODER_MAP[id];
    if (shift_held() && !map_is_open() && !sedit_is_open() && role >= EN_K1 && role <= EN_K4) {
        live_knob((uint32_t)(role - EN_K1), delta);   /* SHIFT: the LIVE page, whatever page is shown */
        ui.redraw = 1;
        return;
    }
    v_enc[role] += delta;
}

static void update_virtual_buttons(void)
{
    uint32_t b = 0;
    for (uint32_t i = 0; i < SMK_BUTTONS; i++) {
        if (!((smk.phys >> i) & 1u) || BUTTON_MAP[i] == 0xFF)
            continue;
        uint32_t fm = BUTTON_MAP[i];
        if (i == SMK_B_ENV && shift_held())
            fm = B_LFO;                         /* one button: UNDO, SHIFT + it: REDO */
        b |= 1u << fm;
    }
    v_buttons = b;
}

static void button_event(uint32_t id, int down)
{
    if (id >= SMK_BUTTONS)
        return;
    uint32_t bit = 1u << id;
    if (!!(smk.phys & bit) == !!down)
        return;
    smk.phys = down ? (smk.phys | bit) : (smk.phys & ~bit);
    if (down)
        ui.touched = 1;                         /* (the boot screen goes) */
    uint32_t before = v_buttons;
    update_virtual_buttons();
    v_buttons_tapped |= v_buttons & ~before;
    if (id == SMK_B_BACK) {                  /* no Bluetooth here: a tap = TAP, held = the CONTROL MAP */
        if (down) {
            smk.bt_down_at = fm1_millis();
            smk.bt_long = 0;
        } else if (!smk.bt_long) {
            if (map_is_open()) {
                map_close();
            } else {
                send_live(LIVE_TAP, 0, 0, 0.0f, 0);
                app_say("TAP", "");
            }
        }
        return;
    }
    if (!down)
        return;
    if (clip_take_end())
        app_say("CLIP", "TAKEN");               /* any button ends a take (and does its own thing) */
    if (id == SMK_B_SELECT && shift_held() && !map_is_open() && !sedit_is_open()) {
        sedit_open();                           /* SHIFT + SELECT: SOUND EDIT (ARP + SEQ too) */
        return;
    }
    switch (id) {
    case SMK_B_PAD_STEP:
        smk.mode_used = 0;                      /* as a tap on MODE */
        setup_pad(PAD_MODE, 0);
        break;
    case SMK_B_FADER_BANK: {
        static const char *const BANK[FADER_BANKS] = { "TRACK LEVELS", "MACROS", "USER (CONTROL MAP)" };
        smk.fader_mode = (uint8_t)((smk.fader_mode + 1u) % FADER_BANKS);
        app_say("FADERS", BANK[smk.fader_mode]);
        break;
    }
    default:
        break;
    }
}

void smk_input_event(unsigned kind, unsigned id, int value)
{
    if (smk.map_until && value > 0 && kind != SMK_PRESSURE && kind != SMK_FADER) {
        smk.map_until = 0;                      /* the map of a mode just taken: a touch ends it */
        ui.msg_until = fm1_millis();
        ui.redraw = 1;
    }
    switch (kind) {
    case SMK_KEY:
        key_event(id, value);
        break;
    case SMK_PAD:
        pad_event(id, value);
        break;
    case SMK_PRESSURE:
        pressure_event(id, value);
        break;
    case SMK_ENCODER:
        encoder_event(id, value);
        break;
    case SMK_FADER:
        fader_event(id, value);
        break;
    case SMK_PEDAL:
        pedal_event(value);
        break;
    case SMK_BUTTON:
        button_event(id, value);
        break;
    default:
        break;
    }
}

/* The scanner lost events: every key, pad and button is taken as let go. */
void smk_input_panic(void)
{
    smk.pedal = 0;
    for (uint32_t i = 0; i < SMK_KEYS; i++) {
        smk.key_held_by_pedal[i] = 0;
        if ((smk.key_down >> i) & 1u)
            key_event(i, 0);
    }
    smk.mode_used = 1;                          /* (MODE let go here changes nothing) */
    for (uint32_t i = 0; i < SMK_PADS; i++)
        pad_event(i, 0);
    smk.phys = 0;
    update_virtual_buttons();
    v_keys = 0;
}

/* --- lights: the buttons as the app lights them, the pads show what the held layer offers --- */

static uint32_t dim(uint32_t rgb, uint32_t level)   /* level 0..256 */
{
    uint32_t r = ((rgb >> 16) & 255u) * level >> 8, g = ((rgb >> 8) & 255u) * level >> 8, b = (rgb & 255u) * level >> 8;
    return r << 16 | g << 8 | b;
}

/* a perform pad's colour, by group as on the keyboard's names: the loops cyan, the slicer violet, the filters orange,
 * the sound effects (delay, drive, crush) yellow, the mutes red, ALL FX OFF white. On: bright (latched: a little
 * less); a running loop blinks at its own length (OCT- / OCT+ change it: the blink follows) */
static uint32_t perform_colour(int w)
{
    uint8_t type = PERFORM_KEY[w].type;
    uint32_t c = type == FX_LOOP ? 0x00C0FF : (type == FX_SLICE ? 0x9040FF : (type == FX_MUTE ? 0xFF0020
               : (type == FX_ALL_OFF ? 0xFFFFFF : (PERFORM_KEY[w].arg == OLS_ROLE_CUTOFF ? 0xFF6000 : 0xFFC000))));
    if (!ui.fx_on[w] && !(type == FX_LOOP && w == 4 && ui.fx_on[LOOP_1_32]))
        return dim(c, 18u);
    if (type == FX_LOOP && live.stutter && live.stutter_len) {
        uint32_t t = live.clock.running ? live.clock.tick : live.clock.free_tick;
        if ((t - live.stutter_start) % live.stutter_len >= live.stutter_len / 2u)
            return dim(c, 40u);
    }
    return dim(c, ui.fx_latched[w] ? 140u : 255u);
}

/* a clip's colour, by the sound it was taken with: red the drums, green the acid ones, pink the USER sounds, blue
 * the others */
static uint32_t clip_colour(const clip_t *c)
{
    int f = c->line < TRACKS && live.track[c->line].kind == OLS_KIND_DRUM ? 3 : sound_family(c->sound);
    return f == 3 ? 0xFF0010 : (f == 1 ? 0x00FF20 : (f == FAMILIES ? 0xFF2080 : 0x0040FF));
}

/* the pads' modes' colours (the MODE pad shows the one it is in) */
static const uint32_t MODE_COLOUR[PAD_MODES] = { 0xFFD000, 0xFF1010, 0x20FFA0, 0x9030FF, 0x2040FF, 0xC0C0C0 };
                                                /* CLIPS yellow, DRUMS red, LIVE 1 (PERFORM) mint (the live colour),
                                                 * LIVE 2 violet, SCENES blue, MIDI white */

#define MODE_MAP_MS 400u                        /* MODE held this long, alone: the pads' map shows */

/* the pads' map shown while MODE is held (generated from the docs' pads sheet): each pad's short name and type */
static const struct { char name[4]; uint8_t type; } PAD_MAP[PAD_MODES][SMK_PADS] = {
    {
        { "C1", 4 }, { "C2", 4 }, { "C3", 4 }, { "C4", 4 },
        { "C5", 4 }, { "C6", 4 }, { "C7", 4 }, { "SW", 10 },
        { "C8", 4 }, { "C9", 4 }, { "C10", 4 }, { "C11", 4 },
        { "C12", 4 }, { "C13", 4 }, { "C14", 4 }, { "REC", 11 },
    },
    {
        { "HFT", 5 }, { "PHH", 5 }, { "LT", 5 }, { "OHH", 5 },
        { "LMT", 5 }, { "HMT", 5 }, { "CRS", 5 }, { "SW", 10 },
        { "KCK", 5 }, { "RIM", 5 }, { "SNR", 5 }, { "CLP", 5 },
        { "SN2", 5 }, { "LFT", 5 }, { "CHH", 5 }, { "REC", 11 },
    },
    {
        { "L1", 0 }, { "L2", 0 }, { "L4", 0 }, { "L8", 0 },
        { "L16", 0 }, { "SLC", 12 }, { "DLY", 13 }, { "SW", 10 },
        { "FL-", 1 }, { "FL+", 1 }, { "CRU", 13 }, { "BRK", 2 },
        { "SOL", 2 }, { "OUT", 2 }, { "OFF", 3 }, { "LV2", 14 },
    },
    {
        { "M1", 8 }, { "M2", 8 }, { "M3", 8 }, { "M4", 8 },
        { "MA", 16 }, { "RST", 14 }, { "CLR", 3 }, { "SW", 10 },
        { "CAP", 11 }, { "UND", 15 }, { "RDO", 15 }, { "MOD", 17 },
        { "AV", 9 }, { "1/2", 0 }, { "PG", 18 }, { "LV1", 14 },
    },
    {
        { "SC1", 7 }, { "SC2", 7 }, { "SC3", 7 }, { "SC4", 7 },
        { "SC5", 7 }, { "SC6", 7 }, { "SC7", 7 }, { "SW", 10 },
        { "SC8", 7 }, { "M1", 8 }, { "M2", 8 }, { "M3", 8 },
        { "M4", 8 }, { "MA", 8 }, { "AV", 9 }, { "REC", 11 },
    },
    {
        { "G2", 6 }, { "G#2", 6 }, { "A2", 6 }, { "A#2", 6 },
        { "B2", 6 }, { "C3", 6 }, { "C#3", 6 }, { "SW", 10 },
        { "C2", 6 }, { "C#2", 6 }, { "D2", 6 }, { "D#2", 6 },
        { "E2", 6 }, { "F2", 6 }, { "F#2", 6 }, { "REC", 11 },
    },
};


/* MODE held alone a moment: the pads of the mode they are in, drawn small in place of the scope (as the keyboard is
 * for a layer button), each in its colour with its short name, MODE and REC framed on the right */
static int smk_pad_map(void)
{
    int shown = smk.map_until && (int32_t)(fm1_millis() - smk.map_until) < 0;
    if (!shown && smk.map_until) {
        smk.map_until = 0;
        ui.redraw = 1;
    }
    if (!shown && (!((smk.pad_down >> PAD_MODE) & 1u) || smk.mode_used || fm1_millis() - smk.mode_down_at < MODE_MAP_MS))
        return 0;
    static const uint32_t TYPE_RGB[18] = { 0x00C0FF, 0xFF6000, 0xFF1A2E, 0xF2F2F2, 0xFFD000, 0xFF3A20, 0xC8C8C8,
                                           0x4A7DFF, 0x22B04A, 0x00E070, 0, 0xFF1010, 0x9040FF, 0xFFC000, 0x00FF40,
                                           0xC08000, 0xFF2080, 0 };   /* ... pink (MUTE ALL), 17: the mod wheel's */
                                                /* loop, filter, mute, off, clip, drum, MIDI, scene, track, A VOL,
                                                 * (MODE), REC, slicer, sound effect, LIVE switch / restart, undo */
    if (!smk.mode_map && !shown) {
        static const char *const NAME[PAD_MODES] = { "CLIPS", "DRUMS", "LIVE 1 LOOP", "LIVE 2 MUTE", "SCENES", "MIDI CH 16" };
        app_say("PADS", NAME[smk.pad_mode]);
        ui.msg_until = plat_ms() + 60000u;      /* (while MODE is held) */
    }
    smk.mode_map = (uint8_t)!shown || ((smk.pad_down >> PAD_MODE) & 1u);
    const int32_t pw = 25, ph = 24, gap = 3, side = 5;
    const int32_t x0 = (240 - (8 * pw + 7 * gap + side)) / 2, y0 = (SCOPE_H - (2 * ph + gap)) / 2;
    cv_begin(240, SCOPE_H, C_BLACK);
    for (uint32_t p = 0; p < SMK_PADS; p++) {
        int32_t col = (int32_t)(p % 8u), row = (int32_t)(p / 8u);
        int32_t x = x0 + col * (pw + gap) + (col == 7 ? side : 0), y = y0 + row * (ph + gap);
        uint8_t t = PAD_MAP[smk.pad_mode][p].type;
        uint32_t c = t == 10u ? MODE_COLOUR[smk.pad_mode] : (t == 17u ? live2_colour(L2_MOD, 1) : (t == 18u ? live2_colour(L2_PAGE, 1)
                   : (t < 18u ? TYPE_RGB[t] : 0x24252Cu)));
        cv_rect(x, y, pw, ph, RGB((c >> 16) & 255u, (c >> 8) & 255u, c & 255u));
        const char *n = PAD_MAP[smk.pad_mode][p].name;
        cv_text(x + (pw - text_w(&FONT_XS, n)) / 2, y + (ph - (int32_t)FONT_XS.h) / 2, &FONT_XS, n, C_BLACK);
    }
    int32_t fx = x0 + 7 * (pw + gap) + side - 3, fy = y0 - 3, fw = pw + 6, fh = 2 * ph + gap + 6;
    cv_rect(fx, fy, fw, 1, C_WHITE);            /* MODE and REC, framed */
    cv_rect(fx, fy + fh - 1, fw, 1, C_WHITE);
    cv_rect(fx, fy, 1, fh, C_WHITE);
    cv_rect(fx + fw - 1, fy, 1, fh, C_WHITE);
    cv_blit(0, SCOPE_Y);
    ui.redraw = 1;
    return 1;
}

/* the boot screen's time (and the help pages after it): a light goes round the 16 pads clockwise (the top row left
 * to right, the bottom row right to left), one lap in 1.6 s, with a trail fading on the three pads behind it. Its
 * colour flows round the colour wheel, a step a lap: red, orange, yellow, mint, blue, violet (the modes' colours and
 * the hues between them; violet in place of MIDI's white, which turns amber when dimmed); a pad keeps the colour the light had when it passed */
#define WAVE_STEP_MS 100u                       /* a pad to the next */
static uint32_t blend(uint32_t a, uint32_t b, uint32_t t)   /* t 0..256: a .. b */
{
    uint32_t c = 0;
    for (uint32_t sh = 0; sh < 24u; sh += 8u) {
        uint32_t x = (a >> sh) & 255u, y = (b >> sh) & 255u;
        c |= ((x * (256u - t) + y * t) >> 8) << sh;
    }
    return c;
}

static uint32_t boot_wave(uint32_t pad)
{
    static const uint32_t WHEEL[6] = { 0xFF1010, 0xFF6000, 0xFFD000, 0x20FFA0, 0x2040FF, 0x8020FF };
    uint32_t ring = pad < 8u ? pad : 15u - (pad - 8u);  /* the pad's place on the ring, clockwise from top left */
    uint32_t pos = (fm1_millis() - smk.wave_t0) * 256u / WAVE_STEP_MS;   /* the light's place, in 256ths of a pad
                                                         * (from the top left at power-on) */
    uint32_t behind = (pos - ring * 256u) & 4095u;      /* how far the light has gone past this pad */
    if (behind >= 1024u)
        return 0;
    uint32_t at = pos - behind, lap = at >> 12;         /* when the light was here: its lap, and how far into it */
    uint32_t c = blend(WHEEL[lap % 6u], WHEEL[(lap + 1u) % 6u], (at & 4095u) >> 4);
    if (behind < 256u)
        return c;                                       /* the light itself */
    uint32_t f = 1024u - behind;                        /* the trail: fading over three pads, faster at first */
    return dim(c, f * f / 2304u);                       /* (768 * 768 / 256) */
}

static uint32_t pad_colour(uint32_t pad)
{
    /* the pads show their own mode only (a held layer is on the keys and the screen) */
    if (ui.splash_until || ui.help)
        return boot_wave(pad);                    /* the logo and the help pages: a light going round */
    if ((int32_t)(fm1_millis() - smk.pads_quiet_until) < 0)
        return 0;                                 /* just after: every pad off a moment, then their mode */
    uint32_t held = (smk.pad_down >> pad) & 1u;
    if (pad == PAD_MODE) {                        /* MODE: the mode's colour */
        return dim(MODE_COLOUR[smk.pad_mode], held ? 256u : 110u);
    }
    if (pad == PAD_REC) {                         /* REC: a take armed or running blinks it, 4 times a second */
        if (LIVE_MODE(smk.pad_mode) && crec.slot < 0)
            return held ? 0x00FF40 : 0x00601A;    /* LIVE 1 / 2: the switch, green */
        if (crec.slot >= 0)
            return (fm1_millis() / 250u) & 1u ? 0xFF0000 : 0x180000;
        if (held)
            return 0xFF0000;
        return smk.pad_mode != PADS_PERFORM && smk.pad_mode != PADS_MIDI ? 0x300000 : 0x040000;
    }
    int i = play_index(pad);
    if (smk.pad_mode == PADS_CLIPS) {
        if (crec.slot == i)                       /* taking: red, a 1/4 s on, a 1/4 s off */
            return (fm1_millis() / 250u) & 1u ? 0xFF0000 : 0x100000;
        if (smk.rec_target == i)
            return 0xFF0000;
        const clip_t *c = &clips[i];
        if (!c->used)
            return 0;                             /* empty: off */
        if (c->playing && c->loop)                /* LOOP: green and violet, half a second each */
            return (fm1_millis() / 500u) & 1u ? 0x8020FF : 0x00FF40;
        return c->playing ? clip_colour(c) : dim(clip_colour(c), 30u);   /* bright: SINGLE playing */
    }
    if (smk.pad_mode == PADS_PERFORM)
        return perform_colour(PERFORM_PAD[i]);
    if (smk.pad_mode == PADS_LIVE2)
        return live2_colour(i, held);
    if (smk.pad_mode == PADS_SCENES) {            /* 1-8 the scenes (REC or SHIFT: store), 9-12 the mutes, 13 all, 14 A VOL */
        if (i < (int)SCENE_PADS) {
            if (shift_held() || ((smk.pad_down >> PAD_REC) & 1u))    /* saving: pink */
                return held ? 0xFF2080 : (live.scene[i].valid ? 0x601030 : 0x180410);
            return live.scene[i].valid ? (i == mirror.scene_last ? 0x60A0FF : 0x0828A0) : 0x020210;
        }
        uint32_t t = (uint32_t)i - SCENE_PADS;
        if (t < TRACKS)
            return live.track[t].mute ? 0x400000 : (t == mirror.track && !ui.on_a ? 0x8060FF : 0x08A008);
        if (t == TRACKS) {                        /* MUTE ALL: fast red blink while every track is out */
            int all_out = 1;
            for (uint32_t k = 0; k < TRACKS; k++)
                all_out &= live.track[k].mute != 0;
            return all_out ? ((fm1_millis() / 125u) & 1u ? 0xFF0010 : 0x100000) : 0x200040;
        }
        return avol.on ? 0x00FF60 : 0x002810;     /* A VOL: lit green while it runs */
    }
    if (smk.pad_mode == PADS_MIDI)                /* MIDI: white while hit, else off */
        return held ? 0xFFFFFF : 0;
    if ((smk.pad_down >> PAD_REC) & 1u)           /* REC held: the five kits, the pads' one bright */
        return i < DR_KIT_COUNT ? (i == set.pad_kit || smk.rec_target == i ? 0xFF2000 : 0x300800) : 0;
    return held ? 0xFF2000 : 0;                   /* DRUMS: red while hit, else off */
}

static void update_lights(void)
{
    uint32_t want = 0;
    for (uint32_t b = 0; b < NB; b++) {
        if ((lights_buttons >> b) & 1u)
            want |= 1u << LED_OF[b];
    }
    if (smk.fader_mode == FADERS_MACRO || (smk.fader_mode == FADERS_USER && (fm1_millis() / 400u) % 2u))
        want |= 1u << SMK_B_FADER_BANK;         /* lit: the macros; blinking: the USER bank */
    if (map_is_open())
        want |= 1u << SMK_B_BACK;
    if (!LIVE_MODE(smk.pad_mode))
        want |= 1u << SMK_B_PAD_STEP;           /* off: LIVE 1 / 2, lit: another mode (the MODE pad's colour says which) */
    if (shift_held())
        want |= 1u << SMK_B_SHIFT;
    if (play_red)
        want |= 1u << SMK_LED_PLAY_ALT;
    uint32_t changed = want ^ smk.leds_shown;
    for (uint32_t i = 0; i <= SMK_LED_PLAY_ALT; i++) {
        if ((changed >> i) & 1u)
            smk_led(i, (want >> i) & 1u);
    }
    smk.leds_shown = want;
    if (ui.splash_until || ui.help) {
        smk.wave_on = 1;
    } else if (smk.wave_on) {                   /* the light stops: the pads off and inert for half a second */
        smk.wave_on = 0;
        smk.pads_quiet_until = fm1_millis() + PADS_QUIET_MS;
    }
    for (uint32_t p = 0; p < SMK_PADS; p++)
        smk_pad_rgb(p, pad_colour(p));
}

/* ------------------------------------------------------------------ the loop */

/* the render, with the audio's MIDI out marked as such; a block over 90 % of its time sheds a voice */
static uint32_t smk_master_pct(void)
{
    return (smk.master_q12 * 100u + 2048u) / 4096u;
}

static void smk_render(int32_t *lr, uint32_t frames, uint32_t master_q12)
{
    uint32_t t0 = smk_ticks24();
    in_render = 1;
    app_render(lr, frames, master_q12);
    in_render = 0;
    uint32_t used = smk_ticks24() - t0;
    if (used > frames * 544u * 9u / 10u)        /* 544 ticks of 24 MHz a sample */
        app_overload();
}

/* SEQ + REC held at power-on: the chip's update mode (its ROM's USB loader, "download mode"), before anything else
 * of ours runs: M-Flasher then writes a firmware from any computer, with no USB key and nothing opened. The same keys
 * as m-bootloader's, which takes them first when it is installed. The SDK's fm1_device_recovery() restarts there. */
void fm1_device_recovery(void);                 /* the SDK (fm1_device.h) */

static int update_mode_held(void)
{
    uint32_t both = (1u << SMK_B_SEQ) | (1u << SMK_B_REC);
    return (smk.phys & both) == both;
}

static void update_mode(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    cv_begin(240, 64, C_BLACK);
    cv_text(120 - text_w(&FONT_B, "UPDATE MODE") / 2, 4, &FONT_B, "UPDATE MODE", RGB(238, 90, 36));
    cv_text(120 - text_w(&FONT_XS, "SEQ + REC AT POWER-ON") / 2, 28, &FONT_XS, "SEQ + REC AT POWER-ON", C_WHITE);
    cv_text(120 - text_w(&FONT_XS, "M-FLASHER, THEN OFF / ON") / 2, 44, &FONT_XS, "M-FLASHER, THEN OFF / ON", C_WHITE);
    cv_blit(0, 88);
    fm1_device_recovery();                      /* (does not come back on the keyboard) */
}

void smk_app_boot(void)
{
    if (smk.booted)
        return;
    smk.booted = 1;
    memset(smk.key_slot, 0xFF, sizeof smk.key_slot);
    memset(smk.pad_slot, 0xFF, sizeof smk.pad_slot);
#ifndef OLS_NO_BLUETOOTH
    (void)fm1_bluetooth_configure(0);           /* no Bluetooth: its radio stays off */
#endif
    smk_panel_init();
    for (uint32_t p = 0; p < SMK_PADS; p++)
        smk_pad_rgb(p, 0);                      /* the pads off at once (they keep the last firmware's colours) */
    palette_set(1u);                            /* (fm1-base APP_PALETTE, as on the FM-1) the screen's greys and values (fm1-base's main does it on the FM-1) */
    {
        uint32_t t0 = fm1_millis();             /* a moment for the scanner to see the buttons held */
        while (fm1_millis() - t0 < 150u)
            smk_panel_poll();
        if (update_mode_held())
            update_mode();
    }
#ifdef APP_SPLASH
    app_splash();                               /* the logo and the version (the first thing to drop if flash is short) */
#else
    {                                           /* the name and the version in the logo's orange */
        char v[24];
        app_version_text(v, sizeof v);
        lcd_fill(0, 0, 240, 240, C_BLACK);
        cv_begin(240, 48, C_BLACK);
        cv_text(120 - text_w(&FONT_B, "OpenVALive") / 2, 6, &FONT_B, "OpenVALive", RGB(238, 90, 36));
        cv_text(120 - text_w(&FONT_XS, v) / 2, 28, &FONT_XS, v, RGB(238, 90, 36));
        cv_blit(0, 96);
    }
#endif
    app_init();
    smk_audio_set_master(smk.master_q12);
    smk_audio_set_render(smk_render);
    /* a moment for the scanner to see the buttons: HOME held at power-on is the safe start */
    uint32_t t0 = fm1_millis();
    while (fm1_millis() - t0 < 150u)
        smk_panel_poll();
    app_ui_init();
    smk.last_frame = fm1_millis();
    smk.wave_t0 = smk.last_frame;               /* the light starts on the top left pad */
}

void smk_app_poll(void)
{
    if (!smk.booted)
        smk_app_boot();
    smk_scan();
    pad_rolls();
    uint32_t now = fm1_millis();
    if (((smk.phys >> SMK_B_BACK) & 1u) && !smk.bt_long && now - smk.bt_down_at >= LONG_PRESS_MS) {
        smk.bt_long = 1;                        /* held: the CONTROL MAP opens (or closes) */
        if (map_is_open())
            map_close();
        else
            map_open();
    }
    if (now - smk.last_frame >= 16u) {          /* ~60 frames a second, input in between */
        smk.last_frame = now;
        app_frame();
        update_lights();
    } else {
        app_input_only();
    }
    smk_panel_service();
    smk_app_midi_flush();
}
