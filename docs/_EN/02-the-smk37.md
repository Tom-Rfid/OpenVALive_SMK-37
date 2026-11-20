# 02 — The SMK-37 (Elite and Pro)

OpenVALive runs on the **M-VAVE SMK-37 Elite and Pro** keyboards: the same app as the FM-1 (4 tracks, loops,
CAPTURE, scenes, Morph, macros, perform effects, Creative Engine), with what the SMK-37 adds: 37 keys, 16
pressure-sensitive RGB pads, 4 faders, the wheels, a pedal, a MASTER encoder and a **MIDI OUT jack**.

**Status (17 November 2026)**: CI builds the firmware for the Elite and the Pro. The app is tested on a computer
with a fake SDK and a fake panel (47 checks: keys, pads, faders, wheels, pedal, buttons, MIDI, saving). **Nothing
has run on a real SMK-37 yet**: the first test is on the owner's Elite, with the test plan below.

## Before installing

- **Licence**: the SMK-37 firmware is **GPL-3.0** (with the SDK's linking exception) because it is built on
  `smk37_open`, M-VAVE's official kit. The core and the FM-1 app stay MIT ([17 — publishing](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_EN/17-publishing.md)).
- **Calibration**: each key's and pad's velocity comes from the factory calibration. M-VAVE's scanner **reads** it;
  OpenVALive never writes it.
- **Bluetooth**: none. The firmware is built without M-VAVE's Bluetooth stack (`targets/smk37/no_bluetooth.py`: ~180 KB of flash and ~10 KB of RAM back, the boot logo with them); `build.py --bluetooth` links it as before. The Bluetooth button taps the tempo and opens the MAP CONTROL.
- **Keys on screen**: the keys have no lights, so while a layer button (SELECT, SEQ, ARP, GLO, SAVE, HOME, FX) is held the screen shows its keys in place of the scope, lit as the FM-1 lights them (KEYS ON SCREEN in the settings rows). Colours: **yellow** pressed; **violet** something on (a track playing, an effect running…); **mint** offered, not on; **pink** valid on release (let the button go first to take it); **mint / pink in turn** valid on release and on now (A VOL running…); **blue** the set-up screens (SOUNDS, LOOP EDIT, OPTIONS); **red** a muted track (SEL).
- **Drum keys**: ORIGINAL or GM map ([FM-1 doc 20](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_EN/20-sounds-and-drum-keys.md)); the pads in DRUMS mode play GM 36 to 49 on the first drum track, as drum machines do.
- **Your M-VAVE Open data**: OpenVALive keeps its set in Open's project slots **6 to 16**. Open's projects 1 to 5,
  settings and samples are not touched. Back them up with the web workbench anyway before installing.
- **One package per model**: `openvalive-smk37-elite-X.Y.Z.fwsc` for the Elite, `-pro-` for the Pro. The keyboard
  refuses the other model's package.
- **The way back**: M-VAVE's SMK37 OPEN workbench (<https://open.m-vave.com/smk37-workbench>, Chrome or Edge):
  *Restore factory V16* (factory firmware) or *Switch to Open firmware*.

## Installing (Windows)

1. From the latest [release](https://github.com/Tom-Rfid/OpenVALive_SMK-37/releases) (or the `nightly` pre-release),
   download **`OpenVALive-SMK37-Elite-X.Y.Z.zip`** (or `-Pro-`) and unzip it.
2. Plug the SMK-37 in over USB (a **data** cable), switch it on, close the web workbench, your DAW and every MIDI
   program.
3. Double-click **`FLASH-SMK37.bat`** (or right-click `flash-smk37.ps1` → *Run with PowerShell*). The script
   prepares Python 3.12 the first time, shows the keyboard's identity, then installs.

From a command line (Windows, Linux, macOS):

```bash
py -3.12 fm1_install.py --port SMK --info
py -3.12 fm1_install.py --port SMK --force --no-resources openvalive-smk37-elite-0.0.3.fwsc
```

`--force`: the package carries M-VAVE's update loader (the kit's), not Felucca's; the keyboard checks the package
before writing. `--no-resources`: nothing else is written.

If the install stops halfway, the keyboard stays in update mode: run the same command again, it resumes. If nothing
answers any more, M-VAVE's web workbench can reinstall a firmware from update mode.

## The controls

| SMK-37 | OpenVALive |
|---|---|
| **37 keys** (C3 to C6) | play the selected track, with velocity; OCT− / OCT+ shift the selected line an octave (T1..T4 and A each their own; a USER sound brings back its octave) |
| white keys + a layer button held | the FM-1's 16 "layer keys" (GLO scenes, ARP Creative, SELECT tracks, SEQ effects, SAVE sounds): 1st white key = 1. On a track, ARP / SEQ / GLO + white key names the gesture: let ARP / SEQ / GLO go first to apply it (mint), the key first to give it up |
| **16 pads**: 2 set-up pads on the right, 14 playing pads | top right **MODE**: a tap goes round **CLIPS** (mint) → **DRUMS** (red) → **PERFORM** (orange) → **SCENES** (blue) → **MIDI** (white); the pad shows the mode's colour. Bottom right **REC**: REC + a pad, **REC let go first** = keep in that pad (a clip, a scene), the pad let go first = given up (red title). **PAD/STEP** goes round the modes too (LED off: PERFORM) |
| pads in **CLIPS** | a clip = a phrase played on the selected line (a track or A), with its sound. **REC + pad** arms it (pad blinking red 4 times a second, the line goes dark green on screen); the **first note** starts the take (bright green filling the line, the full width = 4 bars, the longest); **any pad, MODE, REC or a button** ends it, its own pad ends it and plays it. Length rounded to the beat, at most **4 bars / 48 notes**, 14 clips. **A tap = SINGLE**: once, now, from its first note (again: from the start). **MODE + pad**, or the pad held then MODE = **LOOP**: latched, looping, from the next beat (blinking green / violet, half a second each); a tap lets it go. **REC + a full pad**: REPLACE (the old one stays until the first note). **SHIFT + pad**: deleted. Colours: off = empty, red = drums, green = acid, pink = USER sound, blue = the others; bright = playing. Clips play on the line's sound as it is now, are not recorded in the loop, and are not saved (power off: gone) |
| pads in **DRUMS** | the pads' **own drum kit** (not a track): 14 sounds, C2 (kick) on the bottom left, as drum machines; out on **MIDI channel 10**. **REC held**: the five kits on the first five pads (all, hidden ones too; bright = the pads' one); REC + pad, REC let go first = that kit, kept (SAVE). Pressure = a **roll** on the beat grid, faster the harder: 1/8, 1/16, 1/32; eased off, it stops. Not recorded in the loops |
| pads in **PERFORM** (the default) | 14 perform effects: loops 1 bar to 1/16, filter shut / open, delay, crush, break, solo, drums only, all out, all off (LOOP 1/32 and DRIVE stay on the keys). Hold = while held; a short tap = latched. **Pressure** on a held filter / delay / crush: a light touch = 40 %, a full press = 100 % |
| pads in **SCENES** | top row + the bottom left: scenes 1–8 (on the next bar when playing; bright: the last one, dim: stored, dark: empty); **REC + pad** (or SHIFT + pad) SAVES (pink); then mute / unmute tracks 1–4, all four (fast red blink while all are out), **A VOL** (green swell while it runs) |
| pads in **MIDI** | 14 notes from C2 on **MIDI channel 16** only (USB and the MIDI OUT jack), no sound inside: for a drum machine or a sampler |
| pads + a layer button held | that layer's 16 slots, in any mode: GLO + pads 1–8 recall the scenes, 9–16 store them… |
| HOME + the 4th black key (G#3) | **LOOP EDIT**: the loop's notes one at a time (SELECT, a key = pitch, K1–K4 position / length / velocity / pitch, REC deletes, EDIT adds, EDIT held + keys: a chord) |
| **the 12 buttons on the right** | in the FM-1's order. Top: ARP, NOTE REPEAT = SEL, SCALE = ENV (undo), CHORD = LFO (redo), GLOBE = EDIT, BT = GLO. Below: PATCH = HOME, PARA = SAVE, FX, SEQ, PLAY, REC |
| **left buttons** | KNOB BANK = SHIFT, FADER BANK; **<** = TAP (held: MAP CONTROL), **>** = the pads' mode |
| **8 knobs** | K1 the track (SELECT), K2 the sound (PRESET), K3–K6 the FM-1's K1–K4, K7 the tempo, K8 MASTER |
| **pads** | their own mode only (CLIPS / DRUMS / PERFORM / SCENES / MIDI): they never follow a held layer button (the keys and the screen do) |
| **4 faders** | the levels of tracks 1 to 4 (100 % at 4/5 of the travel) |
| **FADER BANK** | goes round three banks: **levels** → **macros** ENERGY, SPACE, TENSION, DENSITY (LED lit) → **USER**, what MAP CONTROL gives them (LED blinking) |
| **HOME** + white key 8 (C4), HOME let go first | **SOUNDS**: which sounds PRESET offers. The screen stays open with HOME let go (both hands free). One line a family F1–F9: the whole line red = family hidden, “F2” blue = partly, green = all offered; a square a sound; on the chosen line, the chosen sound's name (and under the title its place “3/5”, its family and ON / HIDDEN). Each family's name takes its colour. SELECT the line, PRESET the sound, SEL offered (green) / hidden (red), OCT− / OCT+ the whole family hidden / offered. Last line **→ NEXT PAGE** (the help says "<- PRESET ->"): PRESET or OCT goes on to **MAP CONTROL** (OCT−: **OPTION CONTROL**), which have the same last line; a list longer than the screen has a scroll bar on the right. Any menu button (HOME, FX, ARP, SEQ, GLO, SAVE) closes and saves. A hidden sound still plays where it is; PRESET and the family jumps skip it |
| **FX** + white key 14, FX let go first | the pitch wheel: **BEND** ↔ **PAN** (kept). In PAN, the wheel moves the selected track left (up) and right (down), back to the centre at rest |
| **key velocity** | curved: soft playing gets more of the range (half linear, half square root) |
| **pitch wheel** | by default ±2 semitones on the selected track (at rest: none); set in MAP CONTROL |
| **mod wheel** | by default **MORPH** between the last two recalled scenes (otherwise aftertouch); settable |
| **pedal** | holds the notes (sustain) |
| K1–K4 | the knob page shown (FX changes page: SOUND, MACRO, LIVE, SETUP, USER, **EQ** (the master: BASS, MID, TREBLE, MID FREQ), DRUM EQ; order and pages shown: MAP CONTROL) |
| **SHIFT + K1–K4** | the LIVE page (MORPH, SWING, PUMP, LEVEL), without leaving the page shown |
| SELECT / PAGE / PRESETS | track / tempo / sound (like SELECT, ALGO, PRESET on the FM-1) |
| **MASTER** (8th encoder) | the overall volume |
| ENV | UNDO of the selected track; **SHIFT + ENV**: REDO (none on A: live only) |
| **SHIFT + SELECT** (or ARP + SELECT) | **SOUND EDIT**: every parameter of the sound, tempo-locked LFOs, modulation matrix (see the manual) |
| SELECT (held: tracks layer — its black keys as the FM-1's: 1st (C#3) MUTE ALL, 4th (G#3) A VOL; pressed: takes a waiting sound; held + SELECT knob: point at a line, from A: swap), SEQ (held: perform layer), FX (knob pages) | as on the FM-1 |
| HOME, SAVE, ARP, GLO, PLAY, REC, EDIT | as on the FM-1 ([05](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_EN/05-using-the-fm1.md)) |
| **Bluetooth** | tapped: TAP tempo; **held half a second: MAP CONTROL** |
| **MIDI OUT** (TRS jack) | clock, START/STOP and notes: everything that goes out over USB goes out there too |

### MAP CONTROL and OPTION CONTROL: the pages, what the faders, the wheels and the knobs do

**Hold the Bluetooth button** (or GLO + SEL): **MAP CONTROL** opens. First the knob pages in FX's order (PRESETS
moves one up / down, PAGE shows / hides it), then the knobs of MACRO, LIVE, EQ and USER (K1 to K4 each), FADER 1 to 4
(the USER bank) and the PITCH and MOD wheels. **HOME + white key 7 (B3)** opens **OPTION CONTROL**: DRUM KEYS (ORIGINAL /
GM), **PERC 1–6** (each D# / G# key of the drum keyboard, D#3 to G#5, its own percussion), KEYS ON SCREEN, and for
T1–T4 and A the page a new sound brings (SAME PAGE: none). On a row **EDIT** puts its default back; the last row, MENU,
goes on to SOUNDS / MAP CONTROL / OPTION CONTROL with PRESETS or OCT. Kept when the page closes, without SAVE.

| In the page | Does |
|---|---|
| move a fader, a wheel, turn K1–K4 | picks its row (the fader or wheel keeps playing) |
| SELECT | previous / next row |
| PRESETS | the target: LEVEL, ENERGY, SPACE, TENSION, DENSITY, MORPH, SWING, PUMP, **FILTER, RESO, DRIVE, DELAY, CHORUS, CRUSH, DECAY**, SOUND (a knob of the track's sound), TEMPO (60–180), BEND, AFTER (aftertouch), **MIDI CC**, BASS, MID, TREBLE, MID FREQ (the master EQ), **PAN** (a track left / right, notched in the middle), -- (nothing) |
| PAGE | the option: the track (1–4, or the selected one), ALL (every track, for the effects), the sound's knob, the CC number (0–127, e.g. to play the T-8's controls) |
| EDIT | the row back to its default |
| tap Bluetooth, or a menu button (HOME, FX, ARP, SEQ, GLO, SAVE), or SEL | closes the page; the map is kept in the flash (and with SAVE) |

The **effects** hold their value on the chosen tracks while the control is up; **all the way down** (under 2 %),
the sounds are back as stored. Factory map: K1–K4 = FILTER, RESO, DELAY, CHORUS on the selected track; USER faders =
FILTER, DELAY, DRIVE, CRUSH on the whole mix; PITCH = BEND; MOD = MORPH.

Pad colours: effects **blue** (loops), **orange** (filter, delay, drive, crush), **red** (breaks), white (all off);
bright = running, medium = latched. GLO: stored scenes in blue, the last one lighter. SELECT: tracks in green (red =
muted). ARP: purple. SAVE: kept sounds in amber. DRUMS: red, a pad lights while hit. CLIPS: by the sound (red drums,
green acid, pink USER, blue the others), bright while playing, blinking red while taking. MIDI: white.

## A, the live line

Turn SELECT past track 4: the line **A** under the tracks turns mint. The keys then play A's own sound, live over the
loops and not recorded (PRESETS picks it among the synth sounds). Tap **ARP** for an arpeggio of the keys held: UP,
DOWN, UP/DOWN, RANDOM, OFF. With A selected, the perform effects, CLEAR and the Creative Engine act on every track (no
UNDO / REDO on A: it is live only); with a track selected, on that track only. SELECT + 5th pad mutes the four tracks (A plays on) and lets A
go; SELECT + 6th pad is CLEAR. A loop let go plays to the end of its slice, then the music is back in time.

## Syncing the Roland T-8

1. A **type A TRS-MIDI** cable from the SMK-37's MIDI OUT to the T-8's MIDI in (the T-8 is type A).
2. On the T-8: *MIDI Clock Sync* to **AUTO** (or **MIDI**).
3. On the SMK-37: **SELECT + 9th pad** (or SELECT + 9th white key, D4): CLOCK OUT on.
4. PLAY: the T-8 starts with OpenVALive, at the same tempo; STOP stops it.

Track 2's notes go out on channel 2: the T-8's bass plays them if it listens there.

## Test plan (the Elite first)

Mark each line ✅, ❌ (and what happens) or ? (unclear). The French version ([20](../_FR/02-le-smk37.md)) has the same
table; the steps are:

| # | Step | Expected |
|---|---|---|
| 0 | `fm1_install.py --port SMK --info`, install, `--info` again | `SMK-37 Elite_8000300` after |
| 1 | power on; play a key; power on holding HOME | splash then the FM-1 screen; sound; "SAFE START" |
| 2 | every key soft then hard; OCT; pads PERFORM (hold, tap, pressure); MODE (top right pad) to DRUMS, a pad pressed harder (roll); SCENES (REC + pad 2, REC let go first, stores; pad 2 recalls; the bottom row's 2nd pad mutes track 1); CLIPS (REC + pad 1, play, pad 1: once (SINGLE); MODE + pad 1: LOOP on the beat, green / violet; a tap lets it go); DRUMS: REC held + pad 2 = kit 2; MIDI (channel 16, silent); faders; FADER BANK; wheels; pedal; the 8 encoders; SHIFT + K3; ENV / SHIFT + ENV; Bluetooth taps; button LEDs | as in the table above |
| 3 | REC; EDIT (CAPTURE); GLO + pads (scenes); GLO + PLAY (chain); ARP + pads | as on the FM-1 |
| 4 | T-8 on MIDI OUT, SELECT + pad 9, PLAY, change tempo, STOP | the T-8 follows |
| 5 | PARA (SAVE) + C#3 (SAVE ALL), PARA let go first; power off and on | the set comes back |
| 6 | a DAW: MIDI port, USB audio both ways | notes, clock and sound |
| 7 | 4 busy tracks | no crackles; CPU on screen under 80 % |
| 8 | (if needed) the workbench: *Switch to Open firmware* / *Restore factory V16* | M-VAVE's firmware back |

**For the Pro**: the same plan with the `-pro-` package. The Pro and the Elite share the board and the scanner in
M-VAVE's kit; the SDK handles the model differences.

## Building it yourself

```bash
bash ~/fm1-base/tools/get_toolchain.sh                    # JieLi's Linux toolchain (JIELI_TOOLCHAIN)
git clone https://github.com/m-vave/smk37_open ~/smk37_open
git -C ~/smk37_open checkout $(cat targets/smk37/SMK37_OPEN_COMMIT)
python3 targets/smk37/build.py --model elite --smk37-open ~/smk37_open --fm1-base ~/fm1-base
python3 targets/smk37/build.py --test --smk37-open ~/smk37_open --fm1-base ~/fm1-base   # no toolchain needed
```

The script does what M-VAVE's `sdk/Build.ps1` does on Windows: compiles our files and M-VAVE's scanner, links with
the SDK, and packs with the SDK's update loader (a package writer ported from `Package.cs`, MIT). It checks that the
SDK's heap keeps its 48 KiB (listing what takes the RAM otherwise) and that the interrupts and the panel scan sit in internal RAM. The package writer gives byte-identical files to M-VAVE's `Package.cs` (checked with mono on a test image, 17 Nov 2026).

How it is made: [targets/smk37/README.md](../../targets/smk37/README.md).
