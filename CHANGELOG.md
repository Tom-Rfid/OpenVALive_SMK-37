# Changelog

## 19 November 2026 — buttons read reliably, K1 track / K2 sound, pads on their own, recovery scripts (va: dfcc162)

- The scanner is read between the screen's bands too: a long frame lost button taps, and past 256 events let every
  button go (pushes validated too early, held titles gone, LOOP only while held, FX pages not turning).
- HOME and FX are layers on the white keys too (PANIC / RESTART / STOP, the line's reset, the drum maps).
- K1 the track, K2 the sound. The pads keep their own mode, whatever button is held.
- `recovery/`: bring back a unit that no longer boots, from a Raspberry Pi's GPIO through the USB cable (JieLi USB
  key, then M-Flasher); FR / EN scripts, a Raspberry Pi Pico variant (untested), a README.
- As on the FM-1 (va): titles held while a layer button is, ↑ gestures, ARP %, recording shown, drums +30 %.

## 18 November 2026 — the screen's greys, buttons in the FM-1's order, knobs remapped (first test on a device)

- Fixed: the screen's palette was never set (labels, values, BPM, STOP drawn in black). Now as on the FM-1.
- The 12 buttons on the right in the FM-1's order: ARP, NOTE REPEAT = SEL, SCALE = ENV, CHORD = LFO (REDO),
  GLOBE = EDIT, BT = GLO; PATCH = HOME, PARA = SAVE, FX, SEQ, PLAY, REC. Left: KNOB BANK = SHIFT, FADER BANK,
  < = TAP (held: the CONTROL MAP; also HOME + white key 16 for the settings), > = the pads' mode.
- Knobs: K1 the sound (PRESET), K2 the track (SELECT), K3–K6 the FM-1's K1–K4, K7 the tempo, K8 MASTER.

## 18 November 2026 — sound families

As on the FM-1: F1–F7 before the sound's name; PRESET then OCT− / OCT+ jumps families.

## 18 November 2026 — 7 new sounds

As on the FM-1: ACID SQUARE, ACID SCREAM, ACID DEEP, REESE, WOBBLE, DEEP SUB, DUB SIREN.

## 18 November 2026 — LOOP EDIT

As on the FM-1: HOME + white key 15 edits the selected track's loop a note at a time.

## 18 November 2026 — pads: SCENES mode, drum rolls; the 37 keys on screen (va: fbf68c9)

- PAD/STEP goes round PERFORM → PLAY → **SCENES** (LED blinking): pads 1–8 recall scenes 1–8, SHIFT + them store,
  pads 9–12 mute tracks 1–4, pad 13 all four, at once.
- **Drum rolls**: in PLAY on a drum track, a pad pressed harder repeats on the beat grid, 1/8, 1/16, 1/32 the harder;
  eased off, it stops.
- Keys on screen: the SMK-37's own 37 keys (dark grey = nothing, half filled in mint = it does something now,
  violet = on, blinking between).

## 18 November 2026 — no Bluetooth, the boot logo, keys on screen, drum maps (va: a50da80)

- Built without M-VAVE's Bluetooth stack by default (`no_bluetooth.py`: the 46 calls into it go to a stub of ours,
  the linker leaves the stack out): the app image 275 KB of 453 KB, the heap 59 KB; `--bluetooth` links it as before.
  The boot logo is back. To be confirmed on a device.
- Keys on screen (`APP_KEYS_SCREEN`): a layer button held shows its keys in place of the scope.
- Drum keys in the ORIGINAL or GM map (keys and the CONTROL MAP's settings rows); the keys send GM notes.

## 18 November 2026 — DRUMS 909, ACID 303, triangle, PRESET order, scope auto-gain (va: e894943)

As on the FM-1: DRUMS 909 after DRUMS 808, ACID named ACID 303, saved sounds moved by themselves.

## 18 November 2026 — HOME and FX gestures, readable bars, room in flash (va: 092c24e)

As on the FM-1: HOME + key 1 / 2 / 3 PANIC / RESTART / STOP ALL, FX + key 1 the line's reset, bars white on grey.
The Live Core's control side is built for size (OLS_COLD): the image fits again. No boot logo (3.7 KB short).

## 18 November 2026 — SELECT validates, PUNCH, GLO on release, REC on a muted track (va: e9919a0)

As on the FM-1: SELECT holds the tracks layer, takes a waiting sound, points at a line (from A: swaps); FX turns
the pages; EDIT held + keys: PUNCH; GLO + key / pad taken on release; OCT resizes a running loop. Host test: 47
checks. Doc 02.

## 18 November 2026 — look before you act (va: b43e89d)

As on the FM-1: on a track, ARP / SEQ + white key applies when ARP / SEQ is let go first; SEQ tapped lets the loop
go; SEQ + OCT resizes it. The perform pads still act at once. Doc 02.

## 18 November 2026 — A swaps with a track, page colours (va: 881ddb8)

From A (SELECT lit), SELECT held + SELECT knob on a synth track, let go: A and the track swap sounds and octaves
(ENV on the track swaps back). Page colours, centred values, a running LOOP: red track, EDIT flickering.

## 18 November 2026 — FX tracks, SEQ perform, SELECT pages; an octave per line (va: 61a3ed5)

As on the FM-1: FX + pad / key = tracks layer, SEQ = perform layer, SELECT = pages; OCT- / OCT+ per line (T1..T4,
A), kept with USER sounds; no UNDO / REDO on A; a running LOOP white on red. Host test: 47 checks. Doc 02.

## 18 November 2026 — the live line (va: dadae9a)

A, the live line (its own sound, live, arpeggiator), per-track gestures and undo, loops let go on the grid, from
OpenVALive CORE. The SMK-37's buttons keep their places: SEQ is still the tracks layer (SEQ + 5th pad: the
four tracks muted; CLOCK OUT moved to the 9th pad), SELECT still the pages. Host test: 47 checks.

## 17 November 2026 — its own repository

The SMK-37 target leaves OpenVALive (history kept). The app comes from OpenVALive CORE, the `va/`
submodule, through `targets/smk37/app.h`; `scripts/va.sh` / `va.ps1` move it. Docs: 01, 02 (the SMK-37), 03 (how
the synth is wired), 18 (rules). Same firmware as V0.3 in OpenVALive.

## 17 November 2026 — V0.3 (in OpenVALive)

The OpenVALive app on the SMK-37 Elite and Pro, on M-VAVE's `smk37_open`: 37 keys with the factory velocity, pads
PERFORM / PLAY with pressure, faders, wheels through the CONTROL MAP, pedal, MASTER, MIDI OUT to a T-8, storage in
Open's project slots. CI builds both models; host test with a fake SDK (47 checks).
