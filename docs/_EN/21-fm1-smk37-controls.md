# 21 — FM-1 ↔ SMK-37: the controls side by side

*(Copy of OpenVALive CORE `docs/_EN/19-fm1-smk37-controls.md`, kept the same in OpenVALive_FM-1 and OpenVALive_SMK-37.)*

The shared app (`app/`) is written for the FM-1's panel; the SMK-37 maps its own controls onto it
(`targets/smk37/smk37_app.c` in OpenVALive_SMK-37: `BUTTON_MAP`, `ENCODER_MAP`, `smk_screen_key`, the pads).
**Rule:** a gesture added to the app is added here, and checked on both (FM-1 simulator script, SMK-37 host test).
Same behaviour on both; the SMK-37 only adds what it has more (pads, faders, wheels, pedal, more keys).

## Buttons (same names on both panels)

| App button | FM-1 | SMK-37 | Alone | Held (layer) |
|---|---|---|---|---|
| `B_FX` (`BTN_PAGES`) | FX | FX | next knob page (MAP CONTROL's order) | line reset (key 1), wheel BEND / PAN (key 14), drum map (keys 15 / 16) |
| `B_SEL` (`BTN_TRACKS`, `BTN_VALIDATE`) | SEL | NOTE REPEAT | takes a waiting sound | tracks layer: mutes (at once), then CLEAR, QUANTIZE, CLOCK, EUCLID, CONTROLLER (valid on release); black keys MUTE ALL, A VOL; blinks fast while a sound waits for it (+SEL) |
| `B_SEQ` (`BTN_PERFORM`) | SEQ | SEQ | tapped: a loop lets go | perform layer, title "∞ DIV / FILTER": loops 1–5 valid on release (1/32 by OCT+), SLICER and the other effects at once |
| `B_ARP` | ARP | ARP | on A: arpeggio mode | creative layer |
| `B_GLO` | GLO | BT | — | scenes layer: 1–8 LOAD, 9–16 SAVE (pink); GLO + SEL: MAP CONTROL; GLO + PLAY: chain |
| `B_HOME` | HOME | PATCH | panic | PANIC / RESTART / STOP, OPTION CONTROL (white key 7), SOUNDS (white key 8), LOOP EDIT (4th black key): valid on release (set-up keys blue) |
| `B_SAVE` | SAVE | PARA | — (alone: nothing) | valid on release: white keys 1–8 LOAD USER 1–8, 9–16 SAVE as USER 1–8, 6th black key, between LOAD 8 and SAVE 1, **SAVE ALL** (the live set; blinks when something is to be saved) |
| `B_ENV` / `B_LFO` | ENV / LFO | SCALE / CHORD | UNDO / REDO | — |
| `B_EDIT` | EDIT | GLOBE | CAPTURE (ENV undoes it); in a menu: the row's default; blinks while recording | EDIT + note: punch; LOOP EDIT: EDIT + keys = chord |
| `B_PLAY` / `B_REC` | PLAY / REC | PLAY / REC | transport / record (REC and CAPTURE only while playing) | SEL + PLAY tap tempo, SEL + REC metronome; REC + white key 1 (valid on release): RESET the track |
| `B_OCTDN` / `B_OCTUP` | OCT− / OCT+ | OCT− / OCT+ | octave; family jump; loop ×2 / ÷2 | — |
| — | — | KNOB BANK = SHIFT | — | SHIFT + K1–K4: LIVE page; SHIFT + SELECT: SOUND EDIT; SHIFT + pad (SCENES): store the scene; SHIFT + pad (CLIPS): delete the clip |
| — | — | FADER BANK | faders: levels → macros → USER | — |
| — | — | < (Bluetooth) | TAP tempo | held 0.5 s: MAP CONTROL |
| — | — | > (PAD/STEP) | pads: CLIPS → DRUMS → PERFORM → SCENES → MIDI | — |
| — | — | MODE pad (top right) | the same: a tap = the next mode (its colour) | MODE + pad (CLIPS): LOOP (latched, from the beat); a tap alone: SINGLE |
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
| SEL + 4th black key: A VOL (valid on release) | C#4 | G#3 (and the 14th playing pad in SCENES) |
| drum PERC keys (OPTION CONTROL) | PERC 1–4: G#3 D#4 G#4 D#5 | PERC 1–6: D#3 G#3 D#4 G#4 D#5 G#5 (`APP_PERC_FIRST` / `APP_PERC_COUNT`) |
| key lights | the keys' LEDs | the screen keyboard (KEYS ON SCREEN), while a layer is held |

## Settings (OPTION CONTROL, both)

| Row | Values | What |
|---|---|---|
| PLAY MODE | **LIVE** (default) / FULL MIDI | LIVE: the keys and pads play the sounds inside (and go out on MIDI). FULL MIDI: they only send MIDI (local off), for a DAW or another synth; also SEL + 16th white key |
| DRUM KEYS | ORIGINAL / GM | the drum keys' map |
| PERC 1–4 (FM-1) / 1–6 (SMK-37) | 10 percussions | what each drum D# / G# plays |
| KEYS ON SCREEN | ON / OFF | the screen keyboard while a layer is held |
| T1–T4, A NEW SOUND | SAME PAGE / a page | the knob page a new sound brings |

## Lights

Buttons: **FX** and **SEL** lit; offered but not on = dim, on = lit, a few blink fast (MUTE ALL…), nothing breathes;
**EDIT** lit while playing (blinking while recording); **SAVE** lit when something is to be saved (a mute is not),
blinking while it saves (in steps over about a second, each part only if it changed).
Screen keyboard (and the FM-1's keys where they can): Colours: **yellow** pressed; **violet** something on (a track playing, an effect running…); **mint** offered, not on; **pink** valid on release (let the button go first to take it); **mint / pink in turn** valid on release and on now (A VOL running…); **blue** the set-up screens (SOUNDS, LOOP EDIT, OPTIONS); **red** a muted track (SEL).

Key names on the screen (3 characters at most, written top to bottom; white keys at the bottom, black keys at the top; a dark orange line left of each C):

| Layer | White keys 1 → 16 | Black keys |
|---|---|---|
| SEL | M1 M2 M3 M4 · — · CLR QNT CKI CKO · E3 E4 E5 (yellow) E6 E7 E8 · MID | 1st MA · 4th AV |
| SEQ | L1 L2 L4 L8 L16 · SLC FL- FL+ DLY DRV CRU BRK SOL DRM OUT OFF | — |
| GLO | SC1–SC8 · SV1–SV8 (yellow) | — |
| ARP | MUT HUM FIL SMP RO< RO> REV · T-1 (red) T+1 (green) · EXP CON CHA RAT · O- (red) O+ (green) | — |
| HOME | PNC · RST (green) · STP (red) · 7th OPT · 8th SND | 4th LE |
| FX | RST (red) · 14th PAN (SMK-37) · 15th ORG · 16th GM | — |
| SAVE | U1–U8 · SU1–SU8 | 6th ALL |
| REC | RST | — |

## Only on the SMK-37 (its own controls, nothing the FM-1 needs)

| SMK-37 | What | On the FM-1 |
|---|---|---|
| 16 pads: MODE + REC set-up pads, 14 playing pads | CLIPS (SINGLE / LOOP), DRUMS (their own kit, MIDI ch 10, rolls by pressure), PERFORM (14 effects), SCENES (scenes, mutes, MUTE ALL, A VOL), MIDI (channel 16) | PERFORM, SCENES, mutes, MUTE ALL, A VOL: the same on the keys under a layer. **CLIPS, the MIDI pads and the pad rolls have no FM-1 equivalent** |
| 4 faders | levels, macros, USER slots (FADER BANK) | the knobs (LIVE / MACRO pages, MAP CONTROL) |
| pitch wheel, mod wheel | BEND or PAN, MORPH (mappable) | — |
| sustain pedal | holds the notes | — |
| MIDI OUT (TRS jack) | notes, clock | USB MIDI only |
| 10 more keys, velocity curve | C3–C6 | 27 keys F3–G5 |
| SHIFT (KNOB BANK) | SHIFT + K1–K4: LIVE page; SHIFT + SELECT: SOUND EDIT | ARP + SEL: SOUND EDIT (also on the SMK-37); LIVE page: FX pages |

## Only on the FM-1

| FM-1 | On the SMK-37 |
|---|---|
| a LED in each key (the layer's slots lit) | the screen keyboard (KEYS ON SCREEN) |
| the built-in speaker | headphones / line out |
| MASTER pot | encoder 8 |

**Rule for what comes next:** the common functions are what the FM-1 can do (its 14 buttons, 27 keys, 6 knobs). A
new gesture first gets a place on the FM-1; when it cannot (it needs a pad, a fader, a pedal), it is SMK-37 only and
goes in the table above.
