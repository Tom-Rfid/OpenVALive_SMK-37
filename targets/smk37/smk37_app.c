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

#define MIDI_RING 128u
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
 * clean silence (board/audio_ols.c counts the nested pauses) */
void fm1_audio_suspend(void);
void fm1_audio_resume(void);
#define APP_SAVE_BEGIN() fm1_audio_suspend()
#define APP_SAVE_END() fm1_audio_resume()

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
enum { PADS_CLIPS, PADS_DRUMS, PADS_PERFORM, PADS_SCENES, PADS_MIDI, PAD_MODES };
#define PAD_MODE 7u                             /* the top right pad */
#define PAD_REC 15u                             /* the bottom right pad */
#define PLAY_PADS 14u
#define SCENE_PADS 8u                           /* SCENES: 1-8 the scenes, 9-12 the tracks' mutes, 13 all, 14 A VOL */
#define MIDI_PAD_CHANNEL 15u                    /* MIDI: the pads' notes on channel 16, no sound inside */
/* PERFORM on 14 pads: the 16 effects but LOOP 1/32 and DRIVE */
static const uint8_t PERFORM_PAD[PLAY_PADS] = { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13, 14, 15 };
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
    uint32_t roll_at[SMK_PADS];                 /* the grid line it last hit on */
    uint64_t key_down;                          /* keys held */
    uint8_t fader_seen[4];
    uint32_t leds_shown;
    uint32_t bt_down_at;                        /* the Bluetooth button: a tap = TAP, held = the CONTROL MAP */
    uint8_t bt_long;
    uint8_t booted;
    uint32_t last_frame;
    int8_t rec_target;                          /* REC + this pad, waiting for REC to be let go (valid on release) */
    uint8_t mode_used;                          /* MODE held served a pad: letting it go does not change the mode */
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

/* the set-up pads: MODE (a tap: the next mode; held: CLIPS start on the beat) and REC (held + a pad: keep in it) */
static void setup_pad(uint32_t pad, int down)
{
    if (pad == PAD_MODE) {
        if (down) {
            smk.mode_used = (uint8_t)clip_take_end();
            if (smk.pad_mode == PADS_CLIPS) {   /* a clip pad held, then MODE: that clip goes on looping (LOOP) */
                for (uint32_t p = 0; p < SMK_PADS; p++) {
                    int c = play_index(p);
                    if (c >= 0 && ((smk.pad_down >> p) & 1u) && clips[c].used) {
                        clip_send(clips[c].playing ? CLIP_LATCH : CLIP_PLAY_SYNC, (uint32_t)c, 0);
                        clip_say("LOOP", c, "");
                        smk.mode_used = 1;
                    }
                }
            }
            return;
        }
        if (smk.mode_used)
            return;
        static const char *const NAME[PAD_MODES] = { "CLIPS", "DRUMS", "PERFORM", "SCENES", "MIDI CH 16" };
        smk.pad_mode = (uint8_t)((smk.pad_mode + 1u) % PAD_MODES);
        app_say("PADS", NAME[smk.pad_mode]);
        return;
    }
    if (down) {                                 /* REC */
        if (clip_take_end()) {
            app_say("CLIP", "TAKEN");
            smk.rec_target = -2;                /* (letting REC go then does nothing more) */
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

static void pad_event(uint32_t pad, int velocity)
{
    if (pad >= SMK_PADS)
        return;
    int was_down = (smk.pad_down >> pad) & 1u;
    smk.pad_down = (uint16_t)(velocity ? (smk.pad_down | (1u << pad)) : (smk.pad_down & ~(1u << pad)));
    if (!velocity && !was_down)
        return;
    if (velocity)
        ui.touched = 1;                         /* (the boot screen goes) */
    int i = play_index(pad);
    if (i < 0) {
        smk.pad_slot[pad] = 0xFE;               /* (no note, no roll, no pressure) */
        smk.roll_level[pad] = 0;
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
            smk.rec_target = (int8_t)i;         /* REC + pad: taken when REC is let go first */
            rec_say(i);
            copy_text(ui.msg[1], "REC", sizeof ui.msg[1]);
            ui.msg_style = MSG_ARMED;
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
            } else if ((smk.pad_down >> PAD_MODE) & 1u) {   /* MODE + pad: LOOP, latched, from the next beat */
                if (clips[i].playing && clips[i].loop) {
                    clip_send(CLIP_STOP, (uint32_t)i, 0);
                } else {
                    clip_send(CLIP_PLAY_SYNC, (uint32_t)i, 0);
                    clip_say("LOOP", i, "");
                }
            } else if (clips[i].playing && clips[i].loop) {
                clip_send(CLIP_STOP, (uint32_t)i, 0);    /* a LOOP: a tap lets it go */
            } else {
                clip_send(CLIP_PLAY, (uint32_t)i, 0);    /* SINGLE: once, now (again: from the start) */
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
        note_on(smk.pad_track[pad], smk.pad_note[pad], (uint32_t)velocity);
        return;
    }
    smk.roll_level[pad] = 0;
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
    static const int AT[3] = { 48, 84, 112 };
    uint32_t l = 0;
    while (l < 3u && pressure >= AT[l])
        l++;
    if (l < now_level && pressure + 6 >= AT[now_level - 1u])
        return now_level;
    return l;
}

static void roll_pressure(uint32_t pad, int pressure)
{
    uint32_t l = roll_level(pressure, smk.roll_level[pad]);
    if (l == smk.roll_level[pad])
        return;
    smk.roll_level[pad] = (uint8_t)l;
    if (l) {
        smk.roll_at[pad] = roll_now() / (OLS_PPQN >> l);   /* the next grid line is the first repeat */
        static const char *const RATE[3] = { "1/8", "1/16", "1/32" };
        app_say("ROLL", RATE[l - 1u]);
    }
}

static void pad_rolls(void)
{
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
            send_live(LIVE_OVERRIDE, 1, PERFORM_KEY[w].arg, v, OLS_TRACK_ALL);
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

static uint32_t perform_colour(int w)
{
    static const uint32_t TYPE_COLOUR[4] = { 0x00C0FF, 0xFF7000, 0xFF0020, 0xFFFFFF };   /* loop, filter/fx, mute, off */
    uint32_t c = TYPE_COLOUR[PERFORM_KEY[w].type & 3u];
    return ui.fx_on[w] ? dim(c, ui.fx_latched[w] ? 140u : 255u) : dim(c, 18u);
}

/* a clip's colour, by the sound it was taken with: red the drums, green the acid ones, pink the USER sounds, blue
 * the others */
static uint32_t clip_colour(const clip_t *c)
{
    int f = c->line < TRACKS && live.track[c->line].kind == OLS_KIND_DRUM ? 4 : sound_family(c->sound);
    return f == 4 ? 0xFF0010 : (f == 1 ? 0x00FF20 : (f == FAMILIES ? 0xFF2080 : 0x0040FF));
}

/* the boot screen's time: a wave goes over the pads, left to right, the top row mint, the bottom one orange (the
 * logo's colours); then each pad its mode's colour */
static uint32_t boot_wave(uint32_t pad)
{
    uint32_t col = pad % 8u, t = (fm1_millis() / 3u + 512u - col * 48u) % 512u;
    uint32_t level = t < 256u ? t : 511u - t;   /* 0 .. 255 .. 0 */
    return dim(pad < 8u ? 0x20FFA0 : 0xFF5A24, level * level / 256u);
}

static uint32_t pad_colour(uint32_t pad)
{
    /* the pads show their own mode only (a held layer is on the keys and the screen) */
    if (ui.splash_until)
        return boot_wave(pad);
    uint32_t held = (smk.pad_down >> pad) & 1u;
    if (pad == PAD_MODE) {                        /* MODE: the mode's colour */
        static const uint32_t MODE_COLOUR[PAD_MODES] = { 0x20FFA0, 0xFF1010, 0xFF6000, 0x2040FF, 0xC0C0C0 };
        return dim(MODE_COLOUR[smk.pad_mode], held ? 256u : 110u);
    }
    if (pad == PAD_REC) {                         /* REC: a take armed or running blinks it, 4 times a second */
        if (crec.slot >= 0)
            return (fm1_millis() / 125u) & 1u ? 0xFF0000 : 0x180000;
        if (held)
            return 0xFF0000;
        return smk.pad_mode != PADS_PERFORM && smk.pad_mode != PADS_MIDI ? 0x300000 : 0x040000;
    }
    int i = play_index(pad);
    if (smk.pad_mode == PADS_CLIPS) {
        if (crec.slot == i)                       /* taking: fast red, 4 times a second */
            return (fm1_millis() / 125u) & 1u ? 0xFF0000 : 0x100000;
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
    if (smk.pad_mode != PADS_PERFORM)
        want |= 1u << SMK_B_PAD_STEP;           /* off: PERFORM, lit: another mode (the MODE pad's colour says which) */
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
