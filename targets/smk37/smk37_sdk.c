/* SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
 * Copyright (c) 2026 Tom-Rfid (OpenVALive).
 *
 * OpenVALive - SMK-37: the calls M-VAVE's SDK makes into its app (smk37_open's groove_* hooks), answered by
 * OpenVALive. The SDK runs the chip: start-up, USB (MIDI, audio), the firmware update, the flash; it calls
 * groove_screen_init once and groove_screen_control in its control loop. M-VAVE's own instrument stays in the SDK
 * but is never asked to render (board/audio_ols.c renders OpenVALive instead), so its sequencer hooks below are
 * never called; they are here because the SDK links against them. */
#include <stdint.h>
#include <stddef.h>
#include "fm1.h"
#include "glue.h"

/* --- the app's loop --- */
void groove_screen_init(void) { smk_app_boot(); }
void groove_screen_control(void) { smk_app_poll(); }

/* --- MIDI --- */
void fm1_app_midi_received(unsigned source, const uint8_t *message, size_t bytes)
{
    smk_app_midi_in(source, message, (unsigned)bytes);
}
void fm1_app_sysex(const uint8_t *message, size_t bytes) { (void)message; (void)bytes; }   /* updates: the SDK's */
void groove_midi_init(void) {}
void groove_midi_poll(void) { smk_app_midi_flush(); }
void groove_midi_panic(void) {}
#ifdef OLS_NO_BLUETOOTH
/* no Bluetooth (no_bluetooth.py): every call the SDK made into the Bluetooth stack lands here, and gets 0 */
int groove_midi_panel_reset(void) { return 0; }
#endif
void groove_midi_clock_reset(void) {}

/* M-VAVE's sequencer output (from inside its render, which never runs here) */
FM1_AUDIO_CODE unsigned groove_midi_mode(unsigned track) { (void)track; return 0; }
FM1_AUDIO_CODE void groove_midi_note(unsigned track, unsigned note, unsigned velocity)
{
    (void)track; (void)note; (void)velocity;
}
FM1_AUDIO_CODE void groove_midi_arp_note(unsigned track, unsigned note, unsigned velocity)
{
    (void)track; (void)note; (void)velocity;
}
FM1_AUDIO_CODE void groove_midi_track_reset(unsigned track) { (void)track; }
FM1_AUDIO_CODE void groove_midi_transport(unsigned owner, uint8_t status) { (void)owner; (void)status; }
FM1_AUDIO_CODE void groove_midi_clock(unsigned owner, unsigned tempo) { (void)owner; (void)tempo; }
FM1_AUDIO_CODE void groove_fm_program(uint8_t patch[156], unsigned program)
{
    (void)program;
    for (unsigned i = 0; i < 156; ++i)
        patch[i] = 0;
}

/* --- M-VAVE's projects, samples, DX7 library and transfers: OpenVALive keeps its own (smk37_app.c) --- */
struct groove_project;
static uint64_t project_scratch[4096 / 8];
struct groove_project *groove_project_scratch(void) { return (struct groove_project *)(void *)project_scratch; }
void groove_project_init(void) {}
void groove_project_poll(void) {}
void groove_project_metadata(const struct groove_project *p) { (void)p; }
void groove_project_name(char name[13])
{
    static const char NAME[13] = "OPENVALIVE";
    for (unsigned i = 0; i < 13; ++i)
        name[i] = NAME[i];
}
void groove_samples_init(void) {}
void groove_samples_poll(void) {}
void groove_fm_library_init(void) {}
void groove_transfer_init(void) {}
void groove_transfer_poll(void) {}
