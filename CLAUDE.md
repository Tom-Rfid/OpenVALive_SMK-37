# CLAUDE.md — guide for agents (and busy humans)

OpenVALive SMK-37: the live-first firmware for the M-VAVE SMK-37 Elite / Pro, **GPL-3.0 with smk37_open's linking
exception**. The synth, the Live Core and the app are **OpenVALive CORE**, the git submodule `va/` (MIT);
this repository holds the SMK-37 target only. The FM-1 is OpenVALive_FM-1.

## Commands

```bash
git clone --recursive https://github.com/Tom-Rfid/OpenVALive_SMK-37   # or: sh scripts/va.sh init
git clone https://github.com/m-vave/smk37_open ~/smk37_open && git -C ~/smk37_open checkout $(cat targets/smk37/SMK37_OPEN_COMMIT)
FM1_BASE=~/fm1-base SMK37_OPEN=~/smk37_open sh tests/run_tests.sh    # VA tests + SMK-37 host test (47 checks)
python3 targets/smk37/build.py --model elite --smk37-open ~/smk37_open --fm1-base ~/fm1-base   # needs JIELI_TOOLCHAIN
sh scripts/va.sh status | tags | update [BRANCH] | use REF | reset  # the va/ submodule (va.ps1 on Windows)
```

Before any commit: `tests/run_tests.sh` ends with "All tests pass.".

## Repository map

- `targets/smk37/` `app.h` (includes `va/app/openvalive_app.h`), `smk37_app.c` (fm1-base's `plat.h` on the SDK, the
  SMK-37's extras, includes `va/app/openvalive_app.c`), `smk37_sdk.c` (the SDK's `groove_*` hooks), `glue.h`,
  `board/` (panel, audio), `build.py`, `tests/host_test.c`, `SMK37_OPEN_COMMIT` (the pinned SDK).
- `va/` OpenVALive CORE at a pinned commit. **Never edit or commit inside it from here.**
- `scripts/` `va.sh` / `va.ps1`, `smk37_install.py` (fm1-base's installer + the arming the SMK-37's firmware asks:
  cmd 49 / 50 with the build id, `NAME.build_id` beside the package; `tests/install_test.py`),
  `windows/FLASH-SMK37.bat`, `windows/flash-smk37.ps1`.

## Branches

`dev` is the default (feature branches from it, PRs to it); `main` only receives releases (PR `dev` → `main`, then
Actions > build > Run workflow with "release" ticked). A release pins a **tag** of OpenVALive CORE.

## Rules

- **Never write the calibration** (factory `0xC1000`, VM records 51–54) **or the Bluetooth pairing**. Flash writes
  only through the SDK's `fm1_flash_*`, inside Open's project slots.
- The heap must keep ≥ 61440 bytes (kept as a margin: "stopped after 9 requests" was in fact the missing arming, see
  `scripts/smk37_install.py`) and the app image fit the SDK's limit: `build.py` checks both; interrupts and
  the panel scan must sit in internal RAM (checked).
- Changes to the sound, the Live Core or the screens go to OpenVALive CORE; the app takes device specifics
  through hooks (`APP_KEY_LAYER`, `APP_MAP_SLOTS`), defined here before including it.
- The code rules (C99, float, no libc, no malloc, control rate, bit-identical refactors) are OpenVALive
  VirtualAnalog's CLAUDE.md. GPL code from `smk37_open` may be used **here** (never in OpenVALive CORE).
- Docs: EN and FR together; only what is done or in progress; every "tested" says where.
- Rules for people, git and The Firmware Hub: `docs/_EN/18-rules-git-and-publishing.md`.
- `docs/_*/21-…` (FM-1 ↔ SMK-37 controls) is a copy of OpenVALive CORE's doc 19: keep it the same. The FM-1 sets the
  common functions; a gesture the FM-1 cannot have is SMK-37 only, listed there, and told to Thomas.

## Known pitfalls

- The JieLi toolchain has no `nm`: `build.py` falls back on `objdump -t`.
- The package identity is fixed by M-VAVE's format: `SMK-37 Elite_8XXYYZZ` / `SMK-37 Pro_8XXYYZZ` (the version).
- The render runs from the flash (M-VAVE's instrument ran from RAM): a freeze while saving would point there.
- No Bluetooth by default: `no_bluetooth.py` patches M-VAVE's engine object (its calls into the stack go to
  `groove_midi_panel_reset`, ours) in the build directory; if smk37_open changes, it stops rather than guess.

## Bluetooth

Bluetooth is dropped for good on the SMK-37 (and the FM-1): the firmware is built without M-VAVE's stack;
the `--bluetooth` link is no longer maintained or tested. Do not develop Bluetooth features.
