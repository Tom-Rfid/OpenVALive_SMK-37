# Licence of OpenVALive SMK-37

Copyright (c) 2026 Tom-Rfid.

This repository's code is licensed under the **GNU General Public License version 3 only** ([LICENSE](LICENSE)),
**with the linking exception** of M-VAVE's `smk37_open` SDK: you may link or combine it with the SMK37 Open binary
SDK (`sdk/lib/libfm1_engine.a` and the binary components listed in its `sdk/manifest.json`) and convey the
result. The exception's full text, and M-VAVE's own terms, are in [targets/smk37/LICENSE.md](targets/smk37/LICENSE.md)
(a copy of `smk37_open`'s).

Parts with their own licences:

| Part | Licence |
|---|---|
| `va/`: OpenVALive CORE (the synth, the Live Core, the app) | MIT |
| fm1-base's files used by the build (`gfx.c`, `touch.h`, fonts...) | MPL-2.0 |
| `smk37_open` (M-VAVE): the panel scanner, the SDK | GPL-3.0 + linking exception; the SDK binaries under its THIRD_PARTY.md |

The firmware as a whole is distributed under GPL-3.0 with the linking exception. Every release links to this
repository's source at the tag it was built from.
