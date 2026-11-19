# SMK-37 target (Elite and Pro)

**Licence: GPL-3.0-only WITH GPL-3.0-linking-exception** (this folder), because it is built on M-VAVE's
[`smk37_open`](https://github.com/m-vave/smk37_open). The app and the synth ([OpenVALive CORE](https://github.com/Tom-Rfid/OpenVALive_CORE),
MIT, the `va/` submodule) are used as they are; fm1-base's files (MPL-2.0) keep their licence.

**Status: builds in CI (Elite and Pro), host-tested (47 checks), not yet run on hardware.** User guide and test
plan: [docs/_EN/02-the-smk37.md](../../docs/_EN/02-the-smk37.md) · [docs/_FR/02-le-smk37.md](../../docs/_FR/02-le-smk37.md).

## How it is put together

```
M-VAVE SDK (sdk/lib, binary)      start-up, USB MIDI + audio, firmware update, flash, RTOS
  │  calls groove_screen_init / groove_screen_control, fm1_app_midi_received, fm1_audio_start ...
  ▼
smk37_sdk.c                       those hooks, answered by OpenVALive (M-VAVE's instrument is never asked to render)
board/scan.c (M-VAVE, unchanged)  keys, pads + pressure, encoders, faders, wheels, pedal; reads the calibration
board/panel_ols.c                 scanner events -> the app, screen (M-VAVE's lcd.inc), pad RGB, button LEDs
board/audio_ols.c                 ALNK0 interrupt: the app's render (through a pointer: it lives in the flash), USB audio mix
app.h                             the app's settings for this target (includes va/app/openvalive_app.h)
smk37_app.c                       the platform the app expects (fm1-base's plat.h) + the app (va/app) + the SMK-37 extras
../../va/app/ols_dsp.c            the core at -O2 (the same as the FM-1's)
build.py                          compile, link with the SDK, pack (.fwsc) with the SDK's update loader
```

The app needs two hooks only: `ols_config.h` lets a target set the sample rate, and `ui_input.c` asks
`APP_KEY_LAYER(k)` which layer a key belongs to (the pads in PERFORM mode stand for the white keys with FX held).

## The SMK-37's extras (smk37_app.c)

| | |
|---|---|
| 37 keys | notes, velocity from the factory calibration; the white keys are the layers' 16 slots while a layer button is held |
| 16 pads | PERFORM (perform effects, pressure = depth) or PLAY (notes; drums from C2; pressure = aftertouch); a layer's slots while its button is held |
| faders | track levels, the 4 macros, or USER (the CONTROL MAP): FADER BANK goes round; the first reading only learns the position |
| wheels | through the CONTROL MAP: by default pitch bend ±2 and MORPH (or aftertouch) |
| pedal | sustain |
| encoders | K1–K4, SELECT, PAGE (tempo), PRESETS (sound), MASTER; SHIFT + K = LIVE page |
| buttons | the FM-1's; ENV = UNDO, SHIFT + ENV = REDO; PAD/STEP, FADER BANK; Bluetooth tapped = TAP, held = CONTROL MAP |
| MIDI OUT | everything sent over USB also goes to the TRS jack (clock + transport for a T-8) |
| CONTROL MAP | 10 slots (K1–K4, 4 faders, 2 wheels) in the shared `va/app/src/control_map.c` (MIT) |
| storage | 5 objects (no FM-1 panel object), 2 copies each (A/B, CRC, written head last), in Open's project slots 6–16 |

## Memory

The SDK leaves about 450 KB of flash for the whole app image and needs 48 KiB of heap. `build.py` prints the app
image against its limit and the heap, and fails with a list of the biggest RAM pieces if the heap runs short. Savings
made for the SMK-37: the canvas is 240 × 80 (the tallest band drawn), fonts trimmed and packed, the UI at `-Oz`.

## Safety

- The calibration is only read (VM records 51–54, by M-VAVE's scanner). Nothing here calls `smk37_syscfg_write`.
- Flash writes only through the SDK's `fm1_flash_*` (user partition), which pauses audio (`fm1_audio_suspend`).
- Bluetooth off at boot (`fm1_bluetooth_configure(0)`); no pairing written.
- The package carries M-VAVE's own update loader (`sdk/tools/pack/ota.bin`) and the model's update identity
  (`SMK-37 Elite_8xxxxxx` / `SMK-37 Pro_8xxxxxx`): the keyboard refuses the other model's package.
- Way back: M-VAVE's SMK37 OPEN workbench (Restore factory V16 / Switch to Open firmware).

Known risk to watch on hardware: the render runs from the flash (as on the FM-1), M-VAVE's own instrument ran from
RAM. The SDK pauses audio around its flash writes; a freeze while saving or updating would point here.
