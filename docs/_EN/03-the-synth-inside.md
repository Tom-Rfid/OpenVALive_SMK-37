# 03 — The synth inside: OpenVALive CORE, and how it is wired

This repository holds the **SMK-37 half** of OpenVALive. The synth, the Live Core and the app's screens are
[OpenVALive CORE](https://github.com/Tom-Rfid/OpenVALive_CORE) (MIT), included here as a git submodule in `va/`. The same app runs on the FM-1
([OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1)). This page is the short version; the full one is in OpenVALive CORE
([architecture](https://github.com/Tom-Rfid/OpenVALive_CORE/blob/dev/docs/_EN/03-architecture.md)).

## The layers

```
 SMK-37 hardware    37 keys, 16 RGB pads + pressure, 4 faders, wheels, pedal, encoders, screen, MIDI OUT, flash
      │
 M-VAVE's SDK       smk37_open (GPL-3.0 + linking exception, binary library): start-up, USB, update, flash, RTOS
      │                                                                      ── this repository: targets/smk37/
 board/             M-VAVE's scanner (unchanged), panel_ols.c (events, screen, RGB), audio_ols.c (the interrupt)
 smk37_app.c        the platform the app expects (fm1-base's plat.h API) + the SMK-37's extras + includes the app
 app.h              name, splash, APP_TOUCH, APP_OVERLOAD, host checks; includes va/app/openvalive_app.h
      │                                                                      ── the submodule: va/ (MIT)
 va/app             the instrument: pages, layers, MAP CONTROL, SOUND EDIT, saving, MIDI   (one unit, -Oz)
 va/core/live       the Live Core: clock, 4 tracks, REC, CAPTURE, undo, scenes, Morph, Creative Engine
 va/core/engines    VA (2 osc + sub + noise, filter, 2 env, 2 LFO, mod matrix, FX) · DRUM (808 family)
 va/core/dsp        PolyBLEP, ZDF SVF, ADSR, LFO, delay, chorus, phaser    (va/app/ols_dsp.c, -O2)
```

## One note, from the key to the speaker

| Step | Where | When |
|---|---|---|
| 1. M-VAVE's scanner reads a key, its velocity from the factory calibration (read only) | `board/scan.c` (M-VAVE) | scan interrupt |
| 2. `panel_ols.c` passes the event on; `smk37_app.c` maps it: a note, a pad, a fader, a layer slot... | `board/panel_ols.c`, `smk37_app.c` | control task |
| 3. The app pushes an `ols_event_t` into the lock-free queue (the UI never touches an engine) | `va/app/src/app_events.c` | control task |
| 4. The audio interrupt asks the app's render for 128 frames | `board/audio_ols.c` → `va/app/src/app_audio.c` | every 2.9 ms |
| 5. The Live Core plays the loops to the sample and sends notes to the 4 engines | `va/core/live/live.c` | inside the render |
| 6. Each engine renders; modulation and expensive maths every 32 samples | `va/core/engines/va/` | inside the render |
| 7. Out as int32 Q23 stereo, mixed with USB audio; MIDI also leaves by the TRS jack (a T-8) | `board/audio_ols.c` | inside the render |

## What the SMK-37 adds

| What | Where |
|---|---|
| Pads PERFORM / PLAY, faders (levels, macros, USER), wheels, pedal, MASTER, Bluetooth button = TAP / MAP CONTROL | `smk37_app.c` |
| Two hooks into the shared app: `APP_KEY_LAYER(k)` (pads as layer keys), `APP_MAP_SLOTS 10` (faders and wheels in the MAP CONTROL) | `smk37_app.c`, before including the app |
| Storage: 5 objects, 2 copies each with a CRC, in Open's project slots (never the calibration, never the pairing) | `smk37_app.c` |
| Package per model (`SMK-37 Elite_8XXYYZZ` / `Pro`), byte-identical to M-VAVE's `Package.cs` | `build.py` |

## Changing the synth

A change to the sound, the Live Core or the screens is made **in OpenVALive CORE**, tested there (and on the
FM-1), then this repository moves its pin:

```bash
sh scripts/va.sh status            # which version is pinned
sh scripts/va.sh update            # va/ to the latest dev    (or: use v0.4.0 · use <commit> to go back)
FM1_BASE=~/fm1-base SMK37_OPEN=~/smk37_open sh tests/run_tests.sh
git add va && git commit -m "va: v0.4.0"
```

Windows: `.\scripts\va.ps1 status | update | use v0.4.0 | reset`. Rules: [18](18-rules-git-and-publishing.md).
