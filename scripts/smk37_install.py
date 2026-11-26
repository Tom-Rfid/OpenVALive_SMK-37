#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Install an OpenVALive package (.fwsc) on an SMK-37 Elite / Pro over USB-MIDI.

fm1-base's fm1_install.py (beside this file) does the update; this adds what
the SMK-37's own firmware asks first. A keyboard running an SMK37 OPEN SDK
firmware (OpenVALive's too: "SMK-37 Elite_8xxxxxx") takes only a package it
has been told about, as M-VAVE's workbench does (sdk/WebUpdate.js): cmd 49
reads its update state ("FMC1", family, state, the installed build), cmd 50
arms it with [1, family, the image's length (32-bit LE), the build id (32 B)].
Unarmed, it reads the package's head and stops answering ("stopped answering
after 9 requests; nothing was written"). The build id is the package's
fm1_build_id, which build.py writes beside it: NAME.build_id (hex).

Its update loader then answers "ota-SMK37" + the build id's first 16 hex
digits (M-VAVE's own loader: "ota-FM-1_015"): both are taken as the loader.

  smk37_install.py PACKAGE.fwsc [--port NAME] [--yes] [--force] [--classic]
  smk37_install.py --info [--port NAME]
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fm1_install as F                                         # noqa: E402

SMK_OPEN = re.compile(r"SMK-37 (Pro|Elite)_8\d{6}|SMK37[PE]_OPEN_\d{3}")
SMK_LOADER = re.compile(r"ota-SMK37[0-9a-f]{16}")
FAMILY = {"Pro": 8, "Elite": 9}

_parse = F.parse_identity


def parse_identity(pkt):
    ident = _parse(pkt)
    if ident is not None:
        return ident
    if len(pkt) < 2 or pkt[0] != 0xF0 or pkt[-1] != 0xF7:
        return None
    d = F.unpack7(pkt[1:-1])
    if len(d) != 34 or d[:3] != b"\x00\x59\x11":
        return None
    txt = d[6:33].split(b"\0", 1)[0].decode("latin-1")
    return F.Identity(txt, "ota-SMK37", 0) if SMK_LOADER.fullmatch(txt) else None


def build_id_of(package):
    p = os.path.splitext(package)[0] + ".build_id"
    try:
        h = open(p).read().strip().lower()
    except OSError:
        raise F.InstallError("badpkg", f"{os.path.basename(p)} is missing beside the package: the SMK-37 needs its "
                                       "build id to take it (it comes in the same zip)")
    if not re.fullmatch(r"[0-9a-f]{64}", h):
        raise F.InstallError("badpkg", f"{os.path.basename(p)} is not a build id (64 hex digits)")
    return bytes.fromhex(h)


def arm(up, dev, image, product, build_id):
    """tell the SMK-37's firmware which package comes (cmd 49, then 50)"""
    cap = F.query(dev.link, 49)
    if not cap or len(cap) != 47 or cap[:4] != b"FMC1" or cap[5] not in (8, 9) or cap[7] != 1:
        dev.link.close()
        raise F.InstallError("stopped", f"{dev.id.text} did not tell its update state "
                                        f"({cap.hex() if cap else 'no reply'}); nothing was written")
    model = product.split("_")[0].split()[-1]
    if cap[5] != FAMILY.get(model):
        dev.link.close()
        raise F.InstallError("model", f"the keyboard is an SMK-37 {'Pro' if cap[5] == 8 else 'Elite'}, "
                                      f"the package is for {product}")
    if cap[6] != 0:
        dev.link.close()
        raise F.InstallError("stopped", f"{dev.id.text} is busy with another update (state {cap[6]}): "
                                        "switch it off and on, then run the install again")
    if cap[14:46] == build_id:
        dev.link.close()
        raise F.InstallError("same", f"{dev.id.text} already runs this very build")
    say(f"update state: family {cap[5]}, state {cap[6]}, installed build {cap[14:22].hex()}…")
    ack = F.query(dev.link, 50, bytes([1, cap[5]]) + len(image).to_bytes(4, "little") + build_id)
    if ack != b"\0":
        dev.link.close()
        raise F.InstallError("stopped", f"{dev.id.text} refused the package ({ack.hex() if ack else 'no reply'}); "
                                        "nothing was written")
    say(f"armed: the package's build {build_id[:8].hex()}…, {len(image)} B")


LOG = []


def say(line):
    """one line of the log: on the screen and in smk37_install.log (beside this script), to send back when it fails"""
    LOG.append(line)
    print("    " + line, flush=True)


_request = F.parse_request


def parse_request(pkt):
    r = _request(pkt)
    if r:
        LOG.append(f"request {len(LOG)}: {r[1]:#010x} +{r[2]}")
    return r


def save_log():
    try:
        with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "smk37_install.log"), "w") as f:
            f.write("\n".join(LOG) + "\n")
    except OSError:
        pass


def main(argv=None, **kw):
    argv = sys.argv[1:] if argv is None else argv
    F.parse_identity = parse_identity
    F.parse_request = parse_request
    pkg = next((a for a in argv if a.lower().endswith(".fwsc")), None)
    if pkg:
        build_id = build_id_of(pkg)
        install = F.Updater.install

        def smk_install(self, dev, image, product, step=lambda *a: None, digest=None, classic=False):
            say(f"smk37_install.py: {dev.id.text}" + (" (update loader)" if dev.id.loader else ""))
            try:
                if SMK_OPEN.fullmatch(dev.id.text) and not dev.id.loader:
                    arm(self, dev, image, product, build_id)
                return install(self, dev, image, product, step, digest, classic)
            except F.InstallError as e:
                LOG.append(f"error: {e}")
                raise
            finally:
                save_log()
        F.Updater.install = smk_install
    return F.main(argv, **kw)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except F.InstallError as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
