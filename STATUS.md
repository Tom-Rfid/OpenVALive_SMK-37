# STATUS — OpenVALive SMK-37

*Updated: 2026-11-17 — V0.3, its own repository*

## In one line

The OpenVALive app on the SMK-37 Elite and Pro (GPL-3.0, on M-VAVE's `smk37_open`): **builds in CI and passes the
host test (47 checks); not yet run on a real SMK-37.**

## What works

| Item | Verified |
|---|---|
| The app on the SDK: keys (factory velocity), pads PERFORM / PLAY with pressure, faders (levels / macros / USER), wheels via the CONTROL MAP, pedal, MASTER, MIDI OUT (T-8), storage A/B in Open's project slots | host test with a fake SDK and panel (47 checks) |
| Firmware packages for the Elite and the Pro, byte-identical package writer to M-VAVE's `Package.cs` | CI (monorepo, 17 Nov); mono check of the writer |
| Memory: app image 442 692 / 453 921 bytes, heap 55 564 bytes (≥ 49 152) | CI build |
| `va/` submodule and `scripts/va.sh` / `va.ps1` | va.sh on Linux |
| Boot text: "OpenVALive" and its version (`V_0.0.03`) in orange; the CONTROL MAP (Bluetooth held) shows them too | host test builds it |

## Not verified yet

- **Everything on hardware**: boot, controls, sound, the T-8 on MIDI OUT, saving, the way back to V16
  ([02 — test plan](docs/_EN/02-the-smk37.md#test-plan-the-elite-first)).
- The render from flash during flash writes (saving, updating).

## Decisions

- Route C (17 Nov 2026): a GPL-3.0 target on M-VAVE's `smk37_open`, Elite and Pro, calibration protected.
- Own repository since 17 Nov 2026 (history kept from OpenVALive); the app and the synth are OpenVALive
  VirtualAnalog (`va/`, MIT).
- `dev` by default, `main` for releases.
- The Firmware Hub number: `FM-1_037` (17 Nov). The packages keep M-VAVE's identity format, `SMK-37 Elite_8XXYYZZ` /
  `SMK-37 Pro_8XXYYZZ` (the version), which the keyboard requires.

## Next steps

1. Install on the owner's Elite and run the test plan; then the Pro.
2. Release `v0.0.3` (the OpenVALive family restarts at 0.0.3, 17 Nov) from `main` once the Elite test passes; then The Firmware Hub.
