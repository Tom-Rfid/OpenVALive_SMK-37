# Changelog

# v0.1.1 — 27 November 2026

- `MFLASHER-SMK37.bat` in the zip: beside M-Flasher.exe, installs OpenVALive on a keyboard in UPDATE MODE (SEQ + REC
  held at power-on, on the firmwares that have it). The guide explains both ways: M-Flasher from UPDATE MODE, then
  FLASH-SMK37.bat over MIDI for every update (kept: it also installs another firmware from OpenVALive).
- `fmhub.json`: the card image (docs/img/smk37-card.png) and the visual guide.

## 27 November 2026 — the MIDI update again; SEQ + REC at power-on taken out

- LIVE 1 / LIVE 2 taken: the pads' map stays on the screen until a button or a pad is touched (no time limit; the
  keys and knobs leave it).
- SEQ + REC at power-on (the chip's update mode, a7f11db) taken out: the firmwares with it stopped the MIDI update when
  they read the update loader, 20 November's without it updated. Recovery: jieli-usbkey-unbrick + M-Flasher.
- The DSP compiled -Os (the flash image 0x50000); build.py refuses an image past 0x51000.

## 26 November 2026 — CORE 8416fe9: MOOD per track, FX held aligns the tracks, a taken sound in green, BLEEP / METAL levels

- MACRO renamed MOOD, each track its own; USER's knobs on the selected track keep a value per track; FX held + a knob:
  the other tracks come to its value in five detents (mint bars); a sound taken: the title green with a check mark.
- BLEEP 20 % louder, METAL 10 % softer. (CORE's CHANGELOG has the details.)

## 26 November 2026 — LIVE 2 reworked; the pads' map after a change of mode; FLASH-SMK37.bat works again

- The install over USB-MIDI stopped with "stopped answering after 9 requests; nothing was written": the SMK-37's
  firmware (an SMK37 OPEN SDK one, ours too) takes only a package it has been told about, as M-VAVE's workbench does.
  `smk37_install.py` (in the zip, used by FLASH-SMK37.bat) tells it first: cmd 49 (its state), cmd 50 (the image's
  length, the build id, `NAME.build_id` beside the package). Tested against a simulated SMK-37 (tests/install_test.py).

- LIVE 2: TAP and the scene pads gone; MOD (what the mod wheel does, in turn: MORPH, FILTER, DELAY, CRUSH, DRIVE, PAN of
  the selected track, the pad in its colour), A VOL, 1/2 (the loop half as long), PAGE (the knobs' next page, in its
  colour). Colours: mutes green, MUTE ALL pink, CLEAR TRACK white, LOOP REC red (blinking while armed, mint held).
- Titles "LIVE 1 LOOP" / "LIVE 2 MUTE"; a mode taken shows its pads' map 4 s, or until anything is touched.

## 26 November 2026 — the firmware update gets its RAM back

- The MIDI update stopped after 9 requests (the SDK's heap at 54.6 KB): the tracks' delay buffers are 1024 samples
  shorter (8 KB back; only delay times past 0.35 s clamp) and the build now refuses less than 60 KB of heap.

## 26 November 2026 — the pads' LIVE 2 mode

- A pad mode LIVE 2 (violet), right after PERFORM (now called LIVE 1): MUTE 1–4 (the tracks' own mutes), MUTE ALL,
  RESTART, CLEAR TRACK on top; LOOP REC, UNDO, REDO, TAP, the previous / next stored scene, A VOL below.
- In LIVE 1 and LIVE 2, the bottom right pad switches between them (green, "LV2" / "LV1" on the pads' map); it is
  REC (red) in the other modes.

## 20 November 2026 — the sounds levelled by measure; the slicer on a track; SEL and SEQ rearranged

- Every synth sound levelled by measure (`tools/level_sounds.py`: a K-weighted level, each note by its loudest 100 ms,
  to the median, 3 dB of headroom at the peak): 50 of 59 within 0.1 dB; it replaces the levels set by ear today.
- SLICER: the selected track only (on A, the whole mix as before); OCT- / OCT+ while it runs: 1/4 … 1/64.
- SEL layer: A VOL on the 6th white key, CLEAR TRACK on the 4th black key (red name), LOOP EDIT on the 3rd black key
  (at once). SEL + SELECT: "SWAP TRACK" / "SELECT LINE" in the title; OCT- / OCT+ give it up.
- SEQ layer in groups: the loops, SLICER, DELAY (time), FILTER shut / open, DRIVE, CRUSH, the cuts, OFF; the names
  coloured by group. FX: the drum keys ORIGINAL / GM on the 6th / 7th white keys.
- SEQ lit with FX and SEL; SAVE no longer lit for something to save (its SAVE ALL key blinks).
- The C lines on the screen keyboard 2 pixels wide.
- A setting kept at once (HELP AT START...) is written with the sound paused and tried again if the flash refuses.
- SAVE can go in one step behind a fade (APP_SAVE_ONE_GO, the SMK-37).
- SMK-37 pads: PERFORM mint, CLIPS yellow; PERFORM grouped by colour (cyan loops blinking at their length, violet
  slicer, yellow delay / crush, orange filters, red cuts); REC + pad acts at once; a clip pad again stops it, held then
  REC = LOOP; the take blinks every 1/4 s; rolls go faster a step at a time (no more straight to 1/32); a pad's
  pressure acts on its effect's own track.
- SMK-37 boot: the pads off before the screen's set-up; the light starts on the top left pad; a pad pressed then ends
  the logo and the help and does nothing else; every pad off half a second after.
- SMK-37: SAVE ALL in one go behind a short fade (it stuttered).

## 20 November 2026 — bigger key names on the screen keyboard

- The keys' names on the screen keyboard in a 5x7 font (was 3x5): easier to read. The black keys are 20 % shorter
  from the bottom, so the white keys' names sit under them.
- SMK-37: the light going round the pads during the logo flows round the colour wheel (red, orange, yellow, mint,
  blue, violet), a step a lap, the trail keeping the light's colour.
- SMK-37: the pads' LED chain ends each update with a low reset too, so the colours latch at once: the top left pad
  took a stray bit at the start of the next update and flashed green or showed old colours.
- SMK-37 host test: pictures of the screen keyboard (SMK_SHOTS).

## 20 November 2026 — SEQ + REC at power-on: the update mode

- SEQ + REC held while switching on: "UPDATE MODE" on the screen and the chip's own USB loader (download mode), checked
  before the app starts. M-Flasher then writes a firmware from any computer, with no USB key and nothing opened (the
  same keys as m-bootloader's, which takes them first when installed).

## 20 November 2026 — the top left pad no longer flashes green during the logo

- The pads' LED chain gets a longer reset before each update (200 bytes, was 36): the top left pad, first on the
  chain, flashed green during the logo and the help.
- LOAD USER (CORE) takes the sound at once, also while the loop plays.

## 20 November 2026 — sound levels evened out

- Louder: DEEP SUB, ARP and DUB CHORD by 20 %; RUBBER, PLUCK, POLY, VINTAGE KEYS, LUTE and the whole F7 family (PAD,
  STRINGS, DEEP PAD, STRINGS 70, CHOIR, SWEEP, FLUTE, WHALE) by 10 %.
- Softer: PSY BASS by 20 %; the whole F9 family (DRONE, RUMBLE, WIND, HOLD, CREAK, GRUNGE, SCREECH) by 5 %.

## 20 November 2026 — the pads' light during the logo: slower, with a trail

- During the logo and the help pages, one light goes round the 16 pads clockwise, one lap in 1.6 s (it was 1 s),
  with a trail fading on the three pads behind it; each lap in the next mode's colour.

## 20 November 2026 — the help pages go by themselves; GOA LEAD and METAL 10 % lower; MODE held shows the pads' map

- The help pages go by themselves after three rounds (six pages, 18 s).
- GOA LEAD and METAL 10 % lower.
- SMK-37: MODE held alone a moment shows the pads' map on the screen, in place of the scope (each pad in its colour
  with its short name, MODE and REC framed); letting it go keeps the mode, a tap still changes it. (CORE: a target may
  draw in place of the scope, APP_SCOPE_OVERLAY.)

## 20 November 2026 — the pads during the logo

- During the logo and the help pages, two pads go round the 16 clockwise, one lap a second, in the next mode's
  colour each lap (it was a wave over every pad, which looked like a fault).

## 20 November 2026 — nine new families; SEL on the help pages turns them off

- The 65 sounds in nine families, the most played first in each: ACID, BASS, DRUMS, PERC (short tones), LEAD, KEYS
  (stabs, chords, keys), SOFT, FX, DARK (drones and the harsh ones); USER last. The sounds' numbers in saves are kept.
- On the help pages SEL (NOTE on the SMK-37) turns HELP AT START off for good ("SEL: NO HELP AT START"), and that
  press does nothing else (no tracks layer); another button closes them.

## 20 November 2026 — LOOP REC on EDIT, SAVE on A, sounds

- EDIT tapped while playing arms a loop take: the first note starts it (the line paints green), EDIT again or a menu
  button ends it, and everything played since the first note becomes the loop, to the end of the take (Live Core:
  LIVE_CAPTURE_ARM, LIVE_CAPTURE b = 1). EDIT held + notes is still the punch, over what was there.
- SAVE + SU1–SU8 on A keeps A's own sound (it took the last track's).
- CLICK, DROPS and BLIP longer and louder, DUB CHORD as loud as the other chords, RAVE SAX brassier (two saws, the
  filter blown open on the attack, the pitch scooping up, drive).

## 20 November 2026 — help page KEY COLOURS redrawn; doc 22 and the README at a glance

- KEY COLOURS shows do ré mi (do# ré# neutral) in each colour; pink reads ON RELEASE, LET GO BTN MENU FIRST, THEN KEY,
  with green arrows up.
- Doc 22 (EN / FR): the key colours, the help pages, every layer with its keys' names, the buttons, the set-up
  menus, the pads and the sound families, with the firmware's screens; the README shows it at a glance.

## 20 November 2026 — help pages at start

- After the logo, two help pages in turn every 3 s: KEY COLOURS (blue set-up screens, pink on release, mint ready)
  and IN PLAY (EDIT: push FULL LOOP AT END, hold PART LIVE LOOP; REC: LIVE OVERDUB, adds notes on top). Notes and
  SEL (NOTE on the SMK-37) leave them up; any other button or a knob closes them and does its job. A logo cut short
  skips them. OPTION CONTROL: HELP AT START ON / OFF (in a byte that was padding: the settings format stays OLSF).

## 20 November 2026 — key names on the screen keyboard

- The screen keyboard writes each key's name, 3 characters at most, stacked down the key in a 3x5 font (white keys
  at the bottom, every name starting on the same line; black keys at the top). Some names are coloured: E3–E5 and
  SV1–SV8 yellow, STP / FX's RST / T-1 / O- red, HOME's RST / T+1 / O+ green. A dark orange line marks each C.
- SAVE ALL moves to the 6th black key (C#4), between LOAD 8 and SAVE 1.

## 20 November 2026 — WORLD family, RAVE SAX, LUTE, FLUTE, BAGPIPE, DROPS (65 sounds)

- New family **WORLD** (instruments in the module's electro / underground style): RAVE SAX, LUTE, FLUTE, BAGPIPE.
  DROPS (water drops) joins MINIMAL. Nine factory families F1–F9, then USER shown as "US".
- Settings OLSF: the drum kits and USER sounds 5 higher, OLSE / OLSD moved (hidden sounds, USER origins too).

## 20 November 2026 — USER sounds named after their origin

- A USER sound shows the factory sound it was made from: "U3 ACID 303" (in place of "F9 USER 3"); one kept before
  shows "U3 -- NA --". Settings OLSE (OLSD moved, the origins unknown).

## 20 November 2026 — SLICER, LOOP 1/32 by OCT+, effects at once

- PERFORM key 6 is the **SLICER** (the whole mix cut in 1/16s on the beat, smoothed edges). LOOP 1/32 has no key:
  OCT+ from LOOP 1/16 reaches it, OCT- comes back.
- On a track, only the loops are valid on release (SEQ let go first); the effects and breaks (SLICER, filters, delay,
  DRIVE, CRUSH, BREAK, SOLO...) act at once: hold = while held, tap = latched.
- PERFORM pads: SLICER in, DRUMS ONLY out (DRIVE was already out). Host test: 101 checks.

## 20 November 2026 — 12 new sounds (60), families by style and dynamics

- New: PLATE (struck metal sheet), DUB CHORD, DEEP STAB, DEEP PAD, RUMBLE (deep, dub, underground), BOOM (808-style
  bass), RUBBER, BLEEP, CLICK (minimal), SQUELCH (between acid and techno), GOA LEAD, HOOVER (psytrance, rave).
- Families: ACID, BASSES, DRUMS, PUNCH (leads, stabs, keys: GOA LEAD, HOOVER, ARP join), MINIMAL (RUBBER, BLEEP,
  CLICK, BLIP, METAL), UNDER (deep, dub, underground), SFX (ZAP and LASER join the transitions), SOFT, USER.
- Settings OLSD: the drum kits and USER sounds 12 higher, older settings moved (hidden sounds too).

## 20 November 2026 — CAPTURE only on an empty track, METAL rings

- CAPTURE's green line (EDIT blinking fast) starts only on a track without a loop: after a REC take, the next note
  no longer starts it. Simulator script capture.txt.
- METAL rings on while held and after (longer decay and release, a little chorus and echo).

## 20 November 2026 — STAB, PLUCK, RISER

- STAB and PLUCK hold with the keys (a body after the attack, a longer release) instead of a 200 ms blip; RISER is
  heard at once (shorter attack, the filter a little open).

## 20 November 2026 — more RAM for the firmware update, CAPTURE shows what it takes

- Playing on the selected track (not recording), from the first note the line paints green as the playhead goes (what
  CAPTURE would take: you see when the loop comes round), the red infinity sign shows and EDIT blinks fast; CAPTURE
  (EDIT tapped) puts the line back at once. A stop, REC or another line ends it too.
- Clips keep their notes in one pool (160 notes for the 14): 3 KB of RAM less.
- The firmware update did not start from the 20 November builds (0 requests): the SDK's heap had shrunk from 58 KB to
  52 KB (clips, the pads' kit). Clips' pool and smaller MIDI rings give 4.4 KB back; the build now refuses less than
  54 KB of heap.

## 20 November 2026 — REC + key 1: RESET

- REC is a layer too: REC + the leftmost white key, REC let go first, RESETs (clears) the selected track's loop (ENV
  undoes it). REC alone now acts on release.
- Drum pad rolls need a firm press (a hand resting on a pad no longer rolls at 1/32).

## 20 November 2026 — families: PSY BASS with the basses, PSY before USER

- PSY BASS joins the basses (F2) and holds as long as the key. The psy hits (ZAP, LASER, BLIP, METAL, ARP) go to F8,
  just before USER: ACID, BASSES, DRUMS, PUNCH, SFX, UNDER, SOFT, PSY, USER.

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
