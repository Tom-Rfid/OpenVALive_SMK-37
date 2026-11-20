# Changelog

## 20 November 2026 — no breathing

- Lights and the screen keyboard: offered but not on = dim (steady), on = lit, a few blink fast (MUTE ALL...);
  nothing breathes any more (keys, buttons, A VOL).

## 20 November 2026 — pads at power-on, MODE / REC held

- At power-on the pads go off at once, then a wave (mint on top, orange below) while the boot screen shows.
- Fixed: MODE or REC held made a roll (a repeated note, "ROLL 1/32"): the set-up pads take no pressure.
- DRUMS and MIDI: the pads off until hit. And OpenVALive CORE's changes (hints, HOME keys, CAPTURE, sounds).

## 20 November 2026 — scroll bars, NEXT PAGE

- MAP CONTROL, OPTION CONTROL and SOUND EDIT show a scroll bar on the right when their list is longer than the screen.
- The menus' last line is "→ NEXT PAGE" (was MENU); on it, the help says "<- PRESET ->".

## 20 November 2026 — SAVE ALL is a key

- SAVE alone saves nothing any more: SAVE + the 1st black key = **SAVE ALL** (blinking when something is to be
  saved). Every SAVE-layer key is valid on release (SAVE let go first): LOAD USER 1–8, SAVE USER 1–8, SAVE ALL.
- On a screen keyboard: LOAD keys violet when they hold a sound, grey when empty.

## 20 November 2026 — SAVE in steps, colours by meaning

As OpenVALive CORE: SAVE in steps (each pause short, only what changed), SAVE not lit by a mute, FX / NOTE REPEAT
lit, GLOBE lit while playing, A VOL valid on release; the screen keyboard's colours (yellow pressed, violet on, mint
offered, pink valid on release, blue set-up). The SMK-37's LEDs have one level: no breathing on its buttons.

## 20 November 2026 — SINGLE / LOOP clips, the pads' own drum kit, LOAD / SAVE

- CLIPS: a tap plays once (SINGLE); MODE + pad, or the pad held then MODE, latches a LOOP from the next beat
  (blinking green / violet); a tap lets it go.
- DRUMS: the pads have their own kit (MIDI channel 10); REC held shows the five kits, REC + pad chooses (kept).
- SCENES: REC held, the save pads pink. SAVE / GLO: LOAD keys 1–8, SAVE keys 9–16 (pink on screen); held keys
  yellow, muted tracks red. Host test: 97 checks.

## 20 November 2026 — the pads redone: MODE, REC, CLIPS, DRUMS, MIDI

- The two right pads set up the others: **MODE** (top right, a tap: CLIPS → DRUMS → PERFORM → SCENES → MIDI, in the
  mode's colour) and **REC** (bottom right: REC + pad, REC let go first, records a clip or stores a scene).
- **CLIPS**: 14 phrases on a pad; the take starts at the first note, any pad / button ends it, its own pad plays it;
  rounded to the beat, 4 bars at most; MODE + pad starts on the beat; SHIFT + pad deletes. Colours by sound.
- **DRUMS** replaces PLAY (the pads no longer copy the keyboard): the first drum track, kick bottom left, rolls kept.
- **PERFORM** on 14 pads (LOOP 1/32 and DRIVE stay on the keys); **MIDI**: channel 16 only.
- Fixed: keys played at power-on did not end the boot logo. PERFORM's title: "∞ DIV / FILTER".
- Host test: 90 checks (clips, REC valid on release, MODE, MIDI, DRUMS, the boot screen).
- OPTION CONTROL: **PLAY MODE** LIVE (default) / FULL MIDI. Doc 21: the FM-1 ↔ SMK-37 controls side by side.

## 20 November 2026 — breathing keys on the screen keyboard

As on the FM-1, on the screen keyboard (the SMK-37's LEDs have one level).

## 20 November 2026 — the SMK-37's own names on screen

- The screen names the SMK-37's buttons (NOTE, GLOBE, PARA, PATCH, BT, SCALE, CHORD, PAGE, PRESETS). With the
  FM-1's changes: HOME (PATCH) + white keys 8–10, titles, REC / CAPTURE while playing.

## 19 November 2026 — SEL (NOTE REPEAT) valid on release, lights, drum levels

As on the FM-1; NOTE REPEAT blinks fast while a sound waits for it. Host test: every layer valid on release (ARP,
SEQ, GLO, SEL).

## 19 November 2026 — A plays its own sound, SAVE in one pause, chords

- Fixed: with A selected, the keys and the pads played the last track's sound (T4) instead of A's own; now A's voice
  (MIDI channel 5), as on the FM-1. Host test: a key held while moving to A, then a key on A.
- SAVE (PARA): one short silence instead of a choppy sound (the flash writes pause the audio).
- GLOBE (EDIT) blinks while recording; the infinity sign shows with the pink line; LOOP EDIT chords; UNDER.

## 19 November 2026 — 12 new sounds, families by character

As on the FM-1 (48 sounds; ACID, PSY, BASSES, DRUMS, PUNCH, SFX, DARK, SOFT, USER).

## 19 November 2026 — recording's pink from the first note

As on the FM-1.

## 19 November 2026 — lights checked, PAN target

As on the FM-1; the screen keyboard shows the black keys too (checked in the host test).

## 19 November 2026 — new sounds, MAP CONTROL, OPTION CONTROL, EQ page

As on the FM-1; PERC 1–6 (D#3 to G#5). The Bluetooth button held opens MAP CONTROL.

## 19 November 2026 — A VOL, MUTE ALL on a black key

- SELECT held: the black keys act as the FM-1's (1st C#3 MUTE ALL, 4th G#3 A VOL); SCENES pads: 13 MUTE ALL (fast
  red blink when all are out), 14 A VOL (green swell while it runs).

## 19 November 2026 — SOUNDS and SETTINGS tidied

As on the FM-1: SOUNDS place line under the title, colours; DRUM KEYS as two keys, ON green / OFF red.

## 19 November 2026 — SOUNDS: one line a family

As on the FM-1: one line a family with its colour, a MENU line to SETTINGS, any menu button closes and saves.

## 19 November 2026 — SOUNDS (favourites)

As on the FM-1: HOME + white key 14 chooses the sounds PRESET offers; the set-up keys are red on the screen keyboard.

## 19 November 2026 — wheel PAN, softer keys, STAB / POLY / DRUM EQ, flash tool

- FX + white key 14: the pitch wheel BEND or PAN (the selected track, up = left), kept past a power cycle.
- The keys' velocity curved: more nuance when playing softly.
- As on the FM-1 (CORE): STAB and POLY, the DRUM EQ page (red) on a drum track.
- `FLASH-SMK37.bat` finds the keyboard as "SMK…" or "USB-Midi…", and points to jieli-usbkey-unbrick if it is not seen.

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
