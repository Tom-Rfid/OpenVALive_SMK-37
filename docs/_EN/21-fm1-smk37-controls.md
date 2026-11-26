# 21 — FM-1 ↔ SMK-37: the controls side by side

*(Copy of OpenVALive CORE `docs/_EN/19-fm1-smk37-controls.md`, kept the same in OpenVALive_FM-1 and OpenVALive_SMK-37.)*

The shared app (`app/`) is written for the FM-1's panel; the SMK-37 maps its own controls onto it
(`targets/smk37/smk37_app.c` in OpenVALive_SMK-37: `BUTTON_MAP`, `ENCODER_MAP`, `smk_screen_key`, the pads).
**Rule:** a gesture added to the app is added here, and checked on both (FM-1 simulator script, SMK-37 host test).
Same behaviour on both; the SMK-37 only adds what it has more (pads, faders, wheels, pedal, more keys).

## Buttons (same names on both panels)

| App button | FM-1 | SMK-37 | Alone | Held (layer) |
|---|---|---|---|---|
| `B_FX` (`BTN_PAGES`) | FX | FX | next knob page (MAP CONTROL's order) | line reset (key 1), drum map (keys 6 / 7), wheel BEND / PAN (key 14) |
| `B_SEL` (`BTN_TRACKS`, `BTN_VALIDATE`) | SEL | NOTE REPEAT | takes a waiting sound | tracks layer: mutes (at once), then A VOL, QUANTIZE, CLOCK, EUCLID, CONTROLLER (valid on release); black keys MUTE ALL, LOOP EDIT (at once), CLEAR TRACK; held + SELECT: "SWAP TRACK" / "SELECT LINE" in the title, OCT- / OCT+ give it up; blinks fast while a sound waits for it (+SEL) |
| `B_SEQ` (`BTN_PERFORM`) | SEQ | SEQ | tapped: a loop lets go | perform layer, title "∞ DIV / FILTER": loops 1–5 valid on release (1/32 by OCT+), SLICER (the selected track; OCT- / OCT+ its length) and the other effects at once, in groups: time, filters, sound, cuts |
| `B_ARP` | ARP | ARP | on A: arpeggio mode | creative layer |
| `B_GLO` | GLO | BT | — | scenes layer: 1–8 LOAD, 9–16 SAVE (pink); GLO + SEL: MAP CONTROL; GLO + PLAY: chain |
| `B_HOME` | HOME | PATCH | panic | PANIC / RESTART / STOP, OPTION CONTROL (white key 7), SOUNDS (white key 8), LOOP EDIT (4th black key): valid on release (set-up keys blue) |
| `B_SAVE` | SAVE | PARA | — (alone: nothing) | valid on release: white keys 1–8 LOAD USER 1–8, 9–16 SAVE as USER 1–8, 6th black key, between LOAD 8 and SAVE 1, **SAVE ALL** (the live set; blinks when something is to be saved) |
| `B_ENV` / `B_LFO` | ENV / LFO | SCALE / CHORD | UNDO / REDO | — |
| `B_EDIT` | EDIT | GLOBE | tapped while playing: LOOP REC, the first note starts the take, EDIT or a menu button ends it (the loop; ENV undoes it); in a menu: the row's default; blinks while recording | EDIT + note: punch; LOOP EDIT: EDIT + keys = chord |
| `B_PLAY` / `B_REC` | PLAY / REC | PLAY / REC | transport / record (REC and LOOP REC only while playing) | SEL + PLAY tap tempo, SEL + REC metronome; REC + white key 1 (valid on release): RESET the track |
| `B_OCTDN` / `B_OCTUP` | OCT− / OCT+ | OCT− / OCT+ | octave; family jump; loop ×2 / ÷2 | — |
| — | — | KNOB BANK = SHIFT | — | SHIFT + K1–K4: LIVE page; SHIFT + SELECT: SOUND EDIT; SHIFT + pad (SCENES): store the scene; SHIFT + pad (CLIPS): delete the clip |
| — | — | FADER BANK | faders: levels → macros → USER | — |
| — | — | < (Bluetooth) | TAP tempo | held 0.5 s: MAP CONTROL |
| — | — | > (PAD/STEP) | pads: CLIPS → DRUMS → PERFORM → SCENES → MIDI | — |
| — | — | MODE pad (top right) | the same: a tap = the next mode (its colour) | held alone 0.4 s: the pads' map on the screen in place of the scope (letting it go keeps the mode). CLIPS: a clip pad tapped = SINGLE, again = stop, held then REC = LOOP (in time); REC + pad = record, at once |
| — | — | REC pad (bottom right) | — | REC + pad, REC let go first: record a clip / save a scene / choose the pads' drum kit |

## Knobs

| App encoder | FM-1 | SMK-37 |
|---|---|---|
| `EN_SELECT` | SELECT | encoder 1 |
| `EN_PRESET` | PRESET | encoder 2 (PRESETS) |
| `EN_K1`..`EN_K4` | K1–K4 | encoders 3–6 |
| `EN_ALGO` | ALGO (tempo) | encoder 7 (PAGE) |
| master | MASTER pot | encoder 8 |

## Keys under a layer

| | FM-1 (27 keys, F3–G5) | SMK-37 (37 keys, C3–C6) |
|---|---|---|
| layer slots 1–16 | the 16 white keys F3 … G5 | the first 16 white keys C3 … D5 |
| black keys (`BLACK_KEY[]`) | F#3 G#3 A#3 C#4 D#4 F#4 … (11) | C#3 D#3 F#3 G#3 A#3 … (the n-th black = the FM-1's n-th) |
| SEL + 1st black key: MUTE ALL | F#3 | C#3 (and the 13th playing pad in SCENES) |
| SEL + 6th white key: A VOL (valid on release) | D4 | A3 (and the 14th playing pad in SCENES) |
| SEL + 3rd black key: LOOP EDIT (at once) | A#3 | F#3 |
| SEL + 4th black key: CLEAR TRACK (valid on release) | C#4 | G#3 |
| drum PERC keys (OPTION CONTROL) | PERC 1–4: G#3 D#4 G#4 D#5 | PERC 1–6: D#3 G#3 D#4 G#4 D#5 G#5 (`APP_PERC_FIRST` / `APP_PERC_COUNT`) |
| key lights | the keys' LEDs | the screen keyboard (KEYS ON SCREEN), while a layer is held |

## Settings (OPTION CONTROL, both)

| Row | Values | What |
|---|---|---|
| PLAY MODE | **LIVE** (default) / FULL MIDI | LIVE: the keys and pads play the sounds inside (and go out on MIDI). FULL MIDI: they only send MIDI (local off), for a DAW or another synth; also SEL + 16th white key |
| DRUM KEYS | ORIGINAL / GM | the drum keys' map |
| PERC 1–4 (FM-1) / 1–6 (SMK-37) | 10 percussions | what each drum D# / G# plays |
| KEYS ON SCREEN | ON / OFF | the screen keyboard while a layer is held |
| HELP AT START | ON / OFF | after the logo, two help pages in turn every 3 s: KEY COLOURS (blue, pink, mint) and IN PLAY (EDIT tapped: FULL LOOP AT END, held: PART LIVE LOOP; REC: LIVE OVERDUB). Notes leave them up; SEL (NOTE on the SMK-37) turns them off for good (HELP AT START OFF, that press does nothing else); any other button or a knob closes them (and does its job). A logo cut short (a key, a button) skips them; after three rounds (18 s) they go by themselves |
| T1–T4, A NEW SOUND | SAME PAGE / a page | the knob page a new sound brings |

## Lights

Buttons: **FX**, **SEL** and **SEQ** lit (SEQ blinks while an effect is latched, flickers while a loop runs);
offered but not on = dim, on = lit, a few blink fast (MUTE ALL…), nothing breathes; **EDIT** lit while playing
(blinking while recording); **SAVE** blinks while it saves (each part only if it changed; on the SMK-37 all at once,
the sound faded out a moment); something to save: SAVE + its 6th black key (SAVE ALL) blinks.
Screen keyboard (and the FM-1's keys where they can): Colours: **yellow** pressed; **violet** something on (a track playing, an effect running…); **mint** offered, not on; **pink** valid on release (let the button go first to take it); **mint / pink in turn** valid on release and on now (A VOL running…); **blue** the set-up screens (SOUNDS, LOOP EDIT, OPTIONS); **red** a muted track (SEL).

Key names on the screen (3 characters at most, 5x7 pixels, written top to bottom; white keys at the bottom, under the black keys, black keys at the top; a dark orange line, 2 pixels, left of each C):

| Layer | White keys 1 → 16 | Black keys |
|---|---|---|
| SEL | M1 M2 M3 M4 · — · AV QNT CKI CKO · E3 E4 E5 (yellow) E6 E7 E8 · MID | 1st MA · 3rd LE (blue, violet key) · 4th CLR (red) |
| SEQ | L1 L2 L4 L8 L16 · SLC (violet) DLY (yellow) · FL- FL+ (orange) · DRV CRU (yellow) · BRK SOL DRM OUT (red) · OFF | — |
| GLO | SC1–SC8 · SV1–SV8 (yellow) | — |
| ARP | MUT HUM FIL SMP RO< RO> REV · T-1 (red) T+1 (green) · EXP CON CHA RAT · O- (red) O+ (green) | — |
| HOME | PNC · RST (green) · STP (red) · 7th OPT · 8th SND | 4th LE |
| FX | RST (red) · 6th ORG · 7th GM · 14th PAN (SMK-37) | — |
| SAVE | U1–U8 · SU1–SU8 | 6th ALL |
| REC | RST | — |

## Only on the SMK-37 (its own controls, nothing the FM-1 needs)

| SMK-37 | What | On the FM-1 |
|---|---|---|
| 16 pads: MODE + REC set-up pads, 14 playing pads | CLIPS (SINGLE / LOOP), DRUMS (their own kit, MIDI ch 10, rolls by pressure), LIVE 1 = PERFORM (14 effects), LIVE 2 (mutes 1–4, MUTE ALL, RESTART, CLEAR TRACK, LOOP REC, UNDO, REDO, MOD wheel's job, A VOL, loop 1/2, next knob page; REC switches LIVE 1 ↔ 2), SCENES (scenes, mutes, MUTE ALL, A VOL), MIDI (channel 16) | PERFORM, SCENES, mutes, MUTE ALL, A VOL, and LIVE 2's other gestures: the same on the keys and buttons (SEL, HOME, EDIT, ENV / LFO, FX). **CLIPS, the MIDI pads, the pad rolls, LIVE 2's one-pad layout, its MOD and 1/2 have no FM-1 equivalent** |
| 4 faders | levels, macros, USER slots (FADER BANK) | the knobs (LIVE / MOOD pages, MAP CONTROL) |
| pitch wheel, mod wheel | BEND or PAN, MORPH (mappable) | — |
| sustain pedal | holds the notes | — |
| MIDI OUT (TRS jack) | notes, clock | USB MIDI only |
| 10 more keys, velocity curve | C3–C6 | 27 keys F3–G5 |
| SHIFT (KNOB BANK) | SHIFT + K1–K4: LIVE page; SHIFT + SELECT: SOUND EDIT | ARP + SEL: SOUND EDIT (also on the SMK-37); LIVE page: FX pages |
| SEQ + REC held at power-on | the chip's update mode ("UPDATE MODE"): M-Flasher writes a firmware, no USB key (m-bootloader's keys) | REC held at power-on (fm1-base) |

## Only on the FM-1

| FM-1 | On the SMK-37 |
|---|---|
| a LED in each key (the layer's slots lit) | the screen keyboard (KEYS ON SCREEN) |
| the built-in speaker | headphones / line out |
| MASTER pot | encoder 8 |

**Rule for what comes next:** the common functions are what the FM-1 can do (its 14 buttons, 27 keys, 6 knobs). A
new gesture first gets a place on the FM-1; when it cannot (it needs a pad, a fader, a pedal), it is SMK-37 only and
goes in the table above.
