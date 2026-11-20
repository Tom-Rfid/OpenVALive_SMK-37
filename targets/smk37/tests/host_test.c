/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * OpenVALive - SMK-37: the app on the host, with a fake SDK and a fake panel. It plays the SMK-37's controls
 * (keys, pads, faders, wheels, encoders, buttons) and checks what the Live Core does, the MIDI that goes out and
 * what the flash keeps. Built and run by tests/run_tests.sh (needs smk37_open's headers and fm1-base). */
#include <stdio.h>
#include <stdlib.h>
#include "../smk37_app.c"

/* ------------------------------------------------------------ fake SDK */
static uint32_t now_ms;
static uint8_t flash[600 * 1024];
static int flash_writes;
static uint8_t midi_sent[3][4096];
static unsigned midi_sent_n[3];
static smk_render_fn render_fn;
static uint32_t leds_on;
static uint32_t pad_rgb[16];

static int booting;
uint32_t fm1_millis(void) { return booting ? now_ms++ : now_ms; }   /* the boot waits on the clock */
unsigned fm1_midi_ready_ports(void) { return 3u; }                  /* DIN (MIDI OUT jack) and USB */
int fm1_midi_send_port(unsigned port, const uint8_t *m, size_t n)
{
    for (size_t i = 0; i < n && midi_sent_n[port] < sizeof midi_sent[0]; i++)
        midi_sent[port][midi_sent_n[port]++] = m[i];
    return 0;
}
uint32_t fm1_flash_size(void) { return 536576u; }
int fm1_flash_read(uint32_t off, void *d, size_t n) { memcpy(d, flash + off, n); return 0; }
int fm1_flash_erase(uint32_t off) { memset(flash + off, 0xFF, 4096); return 0; }
int fm1_flash_write(uint32_t off, const void *d, size_t n)
{
    const uint8_t *s = d;
    for (size_t i = 0; i < n; i++)
        flash[off + i] &= s[i];             /* NOR: 1 -> 0 only */
    flash_writes++;
    return 0;
}
int fm1_bluetooth_configure(unsigned f) { (void)f; return 0; }
static int audio_paused;
void fm1_audio_suspend(void) { audio_paused++; }
void fm1_audio_resume(void) { audio_paused--; }
void smk_panel_init(void) {}
void smk_panel_poll(void) {}
void smk_panel_service(void) {}
void smk_led(unsigned b, unsigned on) { leds_on = on ? (leds_on | 1u << b) : (leds_on & ~(1u << b)); }
void smk_pad_rgb(unsigned p, uint32_t rgb) { pad_rgb[p] = rgb; }
void smk_screen_fill(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
void smk_screen_blit_be(unsigned x, unsigned y, unsigned w, unsigned h, const uint16_t *p) { (void)x; (void)y; (void)w; (void)h; (void)p; }
uint32_t smk_ticks24(void) { return now_ms * 24000u; }
void smk_audio_set_render(smk_render_fn fn) { render_fn = fn; }
void smk_audio_set_master(uint32_t q12) { (void)q12; }
uint32_t smk_audio_cpu_pct(void) { return 0; }

/* the note-ons (velocity > 0) sent on a port for one status and note */
static int note_ons(unsigned port, uint8_t status, uint8_t note)
{
    int n = 0;
    for (unsigned i = 0; i + 2 < midi_sent_n[port]; i++)
        n += midi_sent[port][i] == status && midi_sent[port][i + 1] == note && midi_sent[port][i + 2] > 0;
    return n;
}

/* ------------------------------------------------------------ driving it */
static int checks, failed;
#define CHECK(cond, what) do { checks++; if (!(cond)) { failed++; printf("FAIL %s (line %d)\n", what, __LINE__); } } while (0)

static int32_t out[256];
/* time passes: the audio renders its blocks, the control loop polls */
static void run_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        now_ms++;
        if (now_ms % 3u == 0 && render_fn)
            render_fn(out, 128, 4096);
        smk_app_poll();
    }
}
static void ev(unsigned kind, unsigned id, int value) { smk_input_event(kind, id, value); run_ms(20); }
static void tap(unsigned kind, unsigned id) { ev(kind, id, kind == SMK_BUTTON ? 1 : 100); ev(kind, id, 0); }

static int midi_has(unsigned port, uint8_t status)
{
    for (unsigned i = 0; i < midi_sent_n[port]; i++)
        if (midi_sent[port][i] == status)
            return 1;
    return 0;
}

int main(void)
{
    memset(flash, 0xFF, sizeof flash);
    booting = 1;
    smk_app_boot();
    booting = 0;
    run_ms(200);
    CHECK(render_fn != 0, "the render is installed");
    CHECK(live.track[3].kind == OLS_KIND_DRUM, "track 4 is the drums");

    /* keys: C3 on the selected track, velocity kept */
    ev(SMK_KEY, 0, 110);
    CHECK(voices_sounding() > 0, "a key sounds");
    CHECK(midi_has(FM1_MIDI_USB, 0x90) && midi_has(FM1_MIDI_DIN, 0x90), "the key goes out on USB and the MIDI OUT jack");
    ev(SMK_KEY, 0, 0);
    run_ms(400);
    CHECK(ui.splash_until == 0, "a key played: the boot screen goes");
    run_ms(1600);
    CHECK(voices_sounding() == 0, "the key let go is silent");

    /* A (SELECT past track 4): the keys play A's own voice, not the last track (a key held over the move too) */
    ev(SMK_KEY, 2, 100);                                /* held on track 1 ... */
    for (int i = 0; i < 4; i++)
        ev(SMK_ENCODER, 0, 1);                          /* ... SELECT to A */
    run_ms(50);
    CHECK(ui.on_a, "SELECT past track 4: A");
    ev(SMK_KEY, 2, 0);
    run_ms(2000);
    ev(SMK_KEY, 4, 100);
    run_ms(30);
    int a_voices = 0;
    for (int i = 0; i < VA_VOICES; i++)
        a_voices += lv.voice[i].amp.stage != ENV_IDLE;
    CHECK(a_voices > 0 && voices_sounding() == 0, "on A a key plays A's voice, no track");
    ev(SMK_KEY, 4, 0);
    run_ms(2000);
    for (int i = 0; i < 4; i++)
        ev(SMK_ENCODER, 0, -1);                         /* back to track 1 */
    run_ms(50);

    /* PERFORM pads (the default): pad 1 = LOOP 1 BAR while held, a tap latches */
    ev(SMK_BUTTON, SMK_B_PLAY, 1); ev(SMK_BUTTON, SMK_B_PLAY, 0);
    CHECK(live.clock.running, "PLAY plays");
    ev(SMK_PAD, 0, 100);
    CHECK(ui.fx_on[0] && live.stutter, "pad 1 holds the 1-bar loop");
    run_ms(400);
    ev(SMK_PAD, 0, 0);
    CHECK(!ui.fx_on[0], "a held pad let go ends it");
    tap(SMK_PAD, 5);                                    /* FILTER SHUT (LOOP 1/32 is not on the pads), tapped: latched */
    CHECK(ui.fx_on[6] && ui.fx_latched[6], "a tapped pad latches its effect");
    CHECK(pad_rgb[5] == perform_colour(6) && pad_rgb[5] != perform_colour(5), "the pad shows the effect running");
    CHECK(pad_rgb[PAD_MODE] == dim(0xFF6000, 110u), "MODE shows PERFORM's orange");
    tap(SMK_PAD, 14);                                   /* ALL FX OFF (the 14th playing pad) */
    CHECK(!ui.fx_on[6], "ALL FX OFF");

    /* pressure on a held filter pad goes part of the way, then all the way */
    ev(SMK_PAD, 8, 100);                                /* DELAY THROW */
    ev(SMK_PRESSURE, 8, 127);
    CHECK(ui.fx_on[8], "delay throw held");
    ev(SMK_PAD, 8, 0);

    /* GLO + white key 9 (D4), GLO let go first: store scene 1 (the pads do not follow a held layer) */
    ev(SMK_BUTTON, SMK_B_GLO, 1);
    ev(SMK_KEY, 14, 100);
    run_ms(30);
    ev(SMK_BUTTON, SMK_B_GLO, 0);
    ev(SMK_KEY, 14, 0);
    run_ms(100);
    CHECK(live.scene[0].valid, "GLO + white key 9 (GLO let go first) stores scene 1");

    /* every layer valid on release: the button + a key, the button let go first (the key still held) takes it */
    ev(SMK_BUTTON, SMK_B_ARP, 1);                       /* ARP + white key 3 (E3): FILL */
    ev(SMK_KEY, 4, 100);
    run_ms(30);
    CHECK(ui.key_role[WHITE_KEY[2]] == KEY_ARMED_ARP, "ARP + key: armed");
    ev(SMK_BUTTON, SMK_B_ARP, 0);
    run_ms(30);
    CHECK(ui.msg_style == MSG_TAKEN, "ARP let go first: taken");
    ev(SMK_KEY, 4, 0);
    run_ms(100);
    ev(SMK_BUTTON, SMK_B_SEQ, 1);                       /* SEQ + white key 8 (C4): FILTER SHUT, latched */
    ev(SMK_KEY, 12, 100);
    run_ms(30);
    ev(SMK_BUTTON, SMK_B_SEQ, 0);
    run_ms(30);
    CHECK(ui.fx_on[7] && ui.fx_latched[7], "SEQ let go first: taken, latched");
    ev(SMK_KEY, 12, 0);
    run_ms(50);
    ev(SMK_BUTTON, SMK_B_SEQ, 1);
    ev(SMK_KEY, 12, 100);
    run_ms(30);
    ev(SMK_BUTTON, SMK_B_SEQ, 0);
    ev(SMK_KEY, 12, 0);
    run_ms(50);
    CHECK(!ui.fx_on[7], "...and again: off");

    /* SELECT + white key 1 (C3): mute track 1 */
    ev(SMK_BUTTON, SMK_B_SELECT, 1);
    tap(SMK_KEY, 0);
    run_ms(50);
    CHECK(live.track[0].mute, "SELECT + white key 1 mutes track 1");
    tap(SMK_KEY, 0);
    run_ms(50);
    CHECK(!live.track[0].mute, "...and back");
    CHECK(smk_screen_key(1) == KEY_MUTE_ALL && smk_screen_key(8) == KEY_AVOL && smk_screen_key(0) == WHITE_KEY[0]
          && smk_screen_key(26) == WHITE_KEY[15] && smk_screen_key(28) == -1,
          "the screen keyboard: white keys 1-16 and the black keys stand for the FM-1's");
    tap(SMK_KEY, 1);                                    /* the first black key (C#3): MUTE ALL, as the FM-1's F#3 */
    run_ms(50);
    CHECK(live.track[0].mute && live.track[3].mute, "SELECT + the first black key mutes all four");
    tap(SMK_KEY, 1);
    run_ms(50);
    CHECK(!live.track[0].mute && !live.track[3].mute, "...and back");
    ev(SMK_BUTTON, SMK_B_SELECT, 0);

    /* PAD/STEP: SCENES. Pads 1-8 recall, SHIFT + them store (or REC + them, valid on release), 9-12 mute the tracks */
    tap(SMK_BUTTON, SMK_B_PAD_STEP);
    CHECK(smk.pad_mode == PADS_SCENES, "PAD/STEP: scenes");
    ev(SMK_BUTTON, SMK_B_SHIFT, 1);
    tap(SMK_PAD, 1);
    ev(SMK_BUTTON, SMK_B_SHIFT, 0);
    run_ms(100);
    CHECK(live.scene[1].valid, "SHIFT + pad 2 stores scene 2");
    tap(SMK_PAD, 1);
    run_ms(100);
    CHECK(mirror.scene_last == 1, "pad 2 recalls scene 2");
    CHECK(pad_rgb[1] == 0x60A0FF, "the scene just recalled is the bright pad");
    run_ms(2100);                                       /* (the recall lands on the next bar: its mutes with it) */
    ev(SMK_PAD, PAD_REC, 100);
    ev(SMK_PAD, 3, 100);
    ev(SMK_PAD, 3, 0);                                  /* the pad let go first: given up */
    ev(SMK_PAD, PAD_REC, 0);
    run_ms(100);
    CHECK(!live.scene[3].valid && ui.msg_style == MSG_NOT_TAKEN, "REC + pad 4, the pad let go first: not stored");
    ev(SMK_PAD, PAD_REC, 100);
    ev(SMK_PAD, 2, 100);
    ev(SMK_PAD, PAD_REC, 0);                            /* REC let go first: taken */
    ev(SMK_PAD, 2, 0);
    run_ms(100);
    CHECK(live.scene[2].valid, "REC + pad 3, REC let go first: scene 3 stored");
    tap(SMK_PAD, 9);
    run_ms(50);
    CHECK(live.track[0].mute, "the 2nd row's 2nd pad mutes track 1");
    tap(SMK_PAD, 9);
    run_ms(50);
    CHECK(!live.track[0].mute, "...and back");
    tap(SMK_PAD, 13);
    run_ms(50);
    CHECK(live.track[0].mute && live.track[2].mute, "the 13th playing pad mutes all four");
    tap(SMK_PAD, 13);
    run_ms(50);
    CHECK(!live.track[0].mute, "...and back");

    /* MODE (top right) tapped: MIDI, the pads' notes out on channel 16 only */
    tap(SMK_PAD, PAD_MODE);
    CHECK(smk.pad_mode == PADS_MIDI, "MODE: MIDI");
    run_ms(2000);
    midi_sent_n[FM1_MIDI_USB] = 0;
    ev(SMK_PAD, 8, 100);
    CHECK(note_ons(FM1_MIDI_USB, 0x9F, 36) == 1 && voices_sounding() == 0, "MIDI: the bottom left pad sends C2 on channel 16, silent");
    ev(SMK_PAD, 8, 0);

    /* CLIPS: REC + pad 1 arms; the first note starts the take; the pad again ends it and plays it, looping */
    tap(SMK_PAD, PAD_MODE);
    CHECK(smk.pad_mode == PADS_CLIPS, "MODE: CLIPS");
    for (int i = 0; i < 3; i++)
        ev(SMK_ENCODER, 0, 1);                          /* SELECT, 3 detents: track 4 */
    run_ms(50);
    CHECK(mirror.track == 3, "SELECT turns to track 4");
    ev(SMK_PAD, PAD_REC, 100);
    ev(SMK_PAD, 0, 100);
    ev(SMK_PAD, PAD_REC, 0);
    ev(SMK_PAD, 0, 0);
    run_ms(20);
    CHECK(crec.slot == 0 && crec.line == 3 && !crec.started, "REC + pad 1: the clip armed on track 4");
    CHECK(pad_rgb[0] == 0xFF0000 || pad_rgb[0] == 0x100000, "the pad taking blinks red");
    run_ms(300);
    CHECK(!crec.started, "nothing played: not started");
    ev(SMK_KEY, 0, 110);                                /* the take starts here */
    CHECK(crec.started, "the first note starts the take");
    ev(SMK_KEY, 0, 0);
    run_ms(250);
    ev(SMK_KEY, 2, 100);
    ev(SMK_KEY, 2, 0);
    run_ms(600);                                        /* ~ 1.8 beats at 120 */
    tap(SMK_PAD, 0);
    run_ms(20);
    CHECK(crec.slot < 0 && clips[0].used && clips[0].count == 2, "the pad again: the take kept, two notes");
    CHECK(clips[0].length % OLS_PPQN == 0 && clips[0].length >= OLS_PPQN, "its length rounded to the beat");
    CHECK(clips[0].playing, "...and it plays");
    int heard = 0;
    for (int i = 0; i < 300; i++) {
        run_ms(10);
        heard += voices_sounding() > 0;
    }
    CHECK(heard > 0, "the clip sounds, looping");
    CHECK(pad_rgb[0] == 0xFF0010, "a drum clip playing: bright red");
    tap(SMK_PAD, 0);
    run_ms(20);
    CHECK(!clips[0].playing, "a tap stops it");
    ev(SMK_PAD, PAD_MODE, 100);
    tap(SMK_PAD, 0);
    ev(SMK_PAD, PAD_MODE, 0);
    run_ms(20);
    CHECK(clips[0].playing && clips[0].origin % OLS_PPQN == 0 && smk.pad_mode == PADS_CLIPS,
          "MODE + pad: it starts on the beat (and the mode stays)");
    tap(SMK_PAD, 0);
    ev(SMK_BUTTON, SMK_B_SHIFT, 1);
    tap(SMK_PAD, 0);
    ev(SMK_BUTTON, SMK_B_SHIFT, 0);
    run_ms(20);
    CHECK(!clips[0].used, "SHIFT + pad: the clip deleted");

    /* DRUMS: the first drum track's 14 sounds, C2 on the bottom left */
    tap(SMK_PAD, PAD_MODE);
    CHECK(smk.pad_mode == PADS_DRUMS, "MODE: DRUMS");
    ev(SMK_PAD, 8, 120);
    CHECK(voices_sounding() > 0 && smk.pad_note[8] == 36, "the bottom left pad plays the kick");
    ev(SMK_PAD, 8, 0);

    /* a drum pad pressed harder rolls on the grid (1/8, 1/16, 1/32), back to a plain hit when eased off */
    if (!live.clock.running)
        tap(SMK_BUTTON, SMK_B_PLAY);
    midi_sent_n[FM1_MIDI_USB] = 0;
    ev(SMK_PAD, 2, 100);
    ev(SMK_PRESSURE, 2, 100);                           /* 1/16 */
    CHECK(smk.roll_level[2] == 2, "pressed harder: a 1/16 roll");
    run_ms(1000);
    int rolled = note_ons(FM1_MIDI_USB, 0x93, (uint8_t)smk.pad_note[2]);
    CHECK(rolled >= 6 && rolled <= 11, "the roll repeats the drum on the grid");
    ev(SMK_PRESSURE, 2, 0);
    run_ms(500);
    CHECK(note_ons(FM1_MIDI_USB, 0x93, (uint8_t)smk.pad_note[2]) == rolled, "eased off: the roll stops");
    ev(SMK_PAD, 2, 0);
    tap(SMK_BUTTON, SMK_B_PAD_STEP);
    CHECK(smk.pad_mode == PADS_PERFORM, "PAD/STEP goes round to PERFORM");

    /* HOME + white key 3 (E3), HOME let go first: STOP ALL (HOME is a layer on the keys too) */
    if (!live.clock.running)
        tap(SMK_BUTTON, SMK_B_PLAY);
    ev(SMK_BUTTON, SMK_B_HOME, 1);
    ev(SMK_KEY, 4, 100);
    ev(SMK_BUTTON, SMK_B_HOME, 0);
    ev(SMK_KEY, 4, 0);
    run_ms(100);
    CHECK(!live.clock.running, "HOME + white key 3 stops everything");
    tap(SMK_BUTTON, SMK_B_PLAY);                        /* (playing again, as the tests below expect) */

    /* faders: the first reading only learns where they are; then track levels, or the macros (FADER BANK) */
    ev(SMK_FADER, 1, 20);
    ev(SMK_FADER, 1, 100);
    CHECK(ui.level[1] > 0.9f && ui.level[1] < 1.0f, "fader 2 sets track 2's level");
    tap(SMK_BUTTON, SMK_B_FADER_BANK);
    ev(SMK_FADER, 0, 0);
    ev(SMK_FADER, 0, 127);
    run_ms(20);
    CHECK(live.macro[OLS_MACRO_ENERGY] > 0.99f, "FADER BANK: fader 1 = ENERGY");
    CHECK((leds_on >> SMK_B_FADER_BANK) & 1u, "FADER BANK lit on the macros");
    ev(SMK_FADER, 0, 64);

    /* FADER BANK again: USER, the CONTROL MAP's faders (by default: FILTER, DELAY, DRIVE, CRUSH on the whole mix) */
    tap(SMK_BUTTON, SMK_B_FADER_BANK);
    CHECK(smk.fader_mode == FADERS_USER, "FADER BANK goes round to USER");
    ev(SMK_FADER, 1, 90);                               /* DELAY on every track */
    run_ms(20);
    int held = 0;
    for (int i = 0; i < OLS_OVERRIDES; i++)
        held += live.ovr[i].active;
    CHECK(held > 0, "a USER fader holds its effect");
    ev(SMK_FADER, 1, 0);
    run_ms(20);
    held = 0;
    for (int i = 0; i < OLS_OVERRIDES; i++)
        held += live.ovr[i].active;
    CHECK(held == 0, "all the way down: the sounds as stored");

    /* the pitch wheel through the map: its rest is no bend, the top is +2 (on a synth track: SELECT back to 1) */
    for (int i = 0; i < 3; i++)
        ev(SMK_ENCODER, 0, -1);
    ev(SMK_FADER, 4, 127);
    CHECK(va[mirror.track].bend > 1.9f, "pitch wheel up: +2 semitones");
    ev(SMK_FADER, 4, 64);
    CHECK(va[mirror.track].bend > -0.01f && va[mirror.track].bend < 0.01f, "pitch wheel at rest: no bend");
    set.wheel_pan = 1;                                  /* FX + white key 14: the wheel pans the selected track */
    ev(SMK_FADER, 4, 127);
    run_ms(20);
    CHECK(live.track[mirror.track].pan < -0.95f, "PAN mode: the wheel up pans the track left");
    ev(SMK_FADER, 4, 64);
    run_ms(20);
    CHECK(live.track[mirror.track].pan == 0.0f, "...and back to the centre at rest");
    set.wheel_pan = 0;
    CHECK(key_velocity(32) > 40 && key_velocity(127) == 127 && key_velocity(1) >= 1, "the keys' velocity is curved");

    /* MAP CONTROL: Bluetooth held opens it; a fader moved picks its row; PRESETS turns its target */
    ev(SMK_BUTTON, SMK_B_BACK, 1);
    run_ms(600);
    ev(SMK_BUTTON, SMK_B_BACK, 0);
    CHECK(map_is_open(), "Bluetooth held opens MAP CONTROL");
    ev(SMK_FADER, 2, 20);                               /* (the first reading only learns where it is) */
    ev(SMK_FADER, 2, 30);
    CHECK(mapui.row == map_row_of(MAP_F3), "moving fader 3 picks its row");
    uint8_t before = cmap.target[MAP_F3];
    ev(SMK_ENCODER, 1, 1);                              /* PRESETS */
    CHECK(cmap.target[MAP_F3] == before + 1, "PRESETS changes the target");
    ev(SMK_ENCODER, 1, -1);
    ev(SMK_ENCODER, 0, 1);                              /* SELECT: next row */
    CHECK(mapui.row == map_row_of(MAP_F4), "SELECT moves the row");
    int map_writes = flash_writes;
    tap(SMK_BUTTON, SMK_B_BACK);                   /* a tap closes it */
    CHECK(!map_is_open(), "a tap on Bluetooth closes it");
    CHECK(flash_writes > map_writes, "the map is kept in the flash");
    control_map_t kept;
    CHECK(plat_store_load(OBJ_MAP, &kept, sizeof kept) == (int)sizeof kept && kept.magic == MAP_MAGIC,
          "the map reads back");
    CHECK(plat_store_load(OBJ_PANEL, &kept, sizeof kept) < 0, "no FM-1 panel object here");
    tap(SMK_BUTTON, SMK_B_FADER_BANK);                  /* back to the levels */

    /* SHIFT + K3: PUMP, whatever page is shown */
    ev(SMK_BUTTON, SMK_B_SHIFT, 1);
    ev(SMK_ENCODER, 4, 20);
    ev(SMK_BUTTON, SMK_B_SHIFT, 0);
    run_ms(20);
    CHECK(live.pump > 0.2f, "SHIFT + K3 = PUMP");

    /* ENV = UNDO, SHIFT + ENV = REDO (the FM-1's LFO) */
    ev(SMK_BUTTON, SMK_B_SHIFT, 1);
    ev(SMK_BUTTON, SMK_B_ENV, 1);
    CHECK(v_buttons & (1u << B_LFO), "SHIFT + ENV is REDO");
    ev(SMK_BUTTON, SMK_B_ENV, 0);
    ev(SMK_BUTTON, SMK_B_SHIFT, 0);

    /* the MIDI OUT jack: clock and start for a drum machine (SELECT + white key 9 = CLOCK OUT, SELECT let go first) */
    if (!live.send_clock) {
        ev(SMK_BUTTON, SMK_B_SELECT, 1);
        ev(SMK_KEY, 14, 100);                           /* 9th white key from C3: D4 */
        ev(SMK_BUTTON, SMK_B_SELECT, 0);                /* (valid on release: SELECT let go first) */
        ev(SMK_KEY, 14, 0);
    }
    CHECK(live.send_clock, "clock out on");
    ev(SMK_BUTTON, SMK_B_PLAY, 1); ev(SMK_BUTTON, SMK_B_PLAY, 0);   /* stop */
    midi_sent_n[FM1_MIDI_DIN] = 0;
    ev(SMK_BUTTON, SMK_B_PLAY, 1); ev(SMK_BUTTON, SMK_B_PLAY, 0);   /* play */
    run_ms(200);
    CHECK(midi_has(FM1_MIDI_DIN, 0xFA) && midi_has(FM1_MIDI_DIN, 0xF8), "the MIDI OUT jack gets START and the clock");

    /* the pedal holds a note until it is up */
    ev(SMK_PEDAL, 0, 127);
    ev(SMK_KEY, 4, 90);
    ev(SMK_KEY, 4, 0);
    CHECK(smk.key_held_by_pedal[4], "the pedal holds the key's note");
    ev(SMK_PEDAL, 0, 0);
    CHECK(!smk.key_held_by_pedal[4], "pedal up lets it go");

    /* MIDI in: a note from USB plays the selected track */
    ev(SMK_BUTTON, SMK_B_PLAY, 1); ev(SMK_BUTTON, SMK_B_PLAY, 0);   /* stop */
    run_ms(3000);
    uint8_t on[3] = {0x90, 60, 100};
    smk_app_midi_in(FM1_MIDI_USB, on, 3);
    run_ms(10);
    CHECK(voices_sounding() > 0, "USB MIDI in plays");
    uint8_t off[3] = {0x80, 60, 0};
    smk_app_midi_in(FM1_MIDI_USB, off, 3);

    /* SAVE: the whole set in the flash, two copies, read back */
    int writes = flash_writes;
    tap(SMK_BUTTON, SMK_B_SAVE);
    run_ms(100);
    CHECK(flash_writes > writes, "SAVE writes the flash");
    settings_t back;
    CHECK(plat_store_load(OBJ_SETTINGS, &back, sizeof back) == (int)sizeof back && back.magic == SET_MAGIC,
          "the settings read back");
    CHECK(store_sector(OBJ_NOBJ, 0) <= 15u * 4096u, "OpenVALive stays inside Open's project slots 6..15");
    tap(SMK_BUTTON, SMK_B_SAVE);
    run_ms(100);
    CHECK(plat_store_load(OBJ_SETTINGS, &back, sizeof back) == (int)sizeof back, "a second save keeps a good copy");

    /* the scanner lost events: everything up */
    ev(SMK_KEY, 10, 100);
    smk_input_panic();
    run_ms(2000);
    CHECK(voices_sounding() == 0 && smk.phys == 0, "panic: all up");

    printf("smk37 host: %d checks, %d failed\n", checks, failed);
    return failed != 0;
}
