#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""scripts/smk37_install.py against a simulated SMK-37 running an SMK37 OPEN SDK firmware: it reads the package's
head and stops unless cmd 50 armed it with the image's length and the build id (sdk/WebUpdate.js), then its loader
answers "ota-SMK37" + 16 hex digits. Reuses fm1-base's fake device (tests/install_test.py's FakeFM1).
  FM1_BASE=~/fm1-base python3 tests/install_test.py"""
import io
import os
import sys
import tempfile
import threading
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = Path(os.environ["FM1_BASE"])
sys.path.insert(0, str(BASE / "tools"))
sys.path.insert(0, str(ROOT / "scripts"))
import fm1_install as I  # noqa: E402
import smk37_install as S  # noqa: E402

src = (BASE / "tests/install_test.py").read_text()
fake = types.ModuleType("fake")
fake.__file__ = str(BASE / "tests/install_test.py")
exec(src[:src.index("\nwire()\n")], fake.__dict__)      # the fake device and helpers, not fm1-base's own run
I.DELAY.update(fake.I.DELAY)
failed = 0
BUILD = bytes(range(32))
PRODUCT = "SMK-37 Elite_8000004"


def ok(cond, what):
    global failed
    print(f"{what:<64} {'ok' if cond else 'FAIL'}")
    failed += not cond


class FakeSMK(fake.FakeFM1):
    def __init__(self, image, family=9, installed=bytes(32), busy=0, **kw):
        self.family, self.installed, self.busy = family, installed, busy
        super().__init__(image, identity="SMK-37 Elite_8000003", name="USB-Midi", after_write=PRODUCT, **kw)

    def ident(self, text):                       # 27 bytes of text: the loader's name is 25 long (no JEDEC ID after it)
        b = text.encode() + bytes(27 - len(text))
        self.tx(b"\xF0" + I.pack7(bytes([0, 0x59, 0x11, 27, 0, 0]) + b + bytes([~sum(b) & 0xFF])) + b"\xF7")

    def rx(self, d):
        u = I.unpack7(d[1:-1])
        if self.identity.startswith("SMK") and len(u) >= 7 and u[:2] == b"\x00\x59" and u[2] in (49, 50):
            data = u[6:-1]
            if u[2] == 49:
                out = (b"FMC1" + bytes([1, self.family, self.busy, 1]) + (512).to_bytes(2, "little")
                       + (1 << 20).to_bytes(4, "little") + self.installed + b"\0")
            else:
                self.arms.append(bytes(data))
                self.armed = (len(data) == 38 and data[:2] == bytes([1, self.family])
                              and int.from_bytes(data[2:6], "little") == len(self.image) and data[6:] == BUILD)
                out = b"\0" if self.armed else b"\x01"
            self.tx(b"\xF0" + I.pack7(bytes([0, 0x59, u[2], len(out), 0, 0]) + out + bytes([~sum(out) & 0xFF]))
                    + b"\xF7")
            return
        if d == I.UPGRADE and self.identity.startswith("SMK") and not self.armed:
            self.queue = [(0, 64)] * 9          # the head, then silence: "stopped answering after 9 requests"
            self.next()
            return
        if self.waiting and self.waiting[0] == I.FINISH_CHECK:
            self.waiting = None
            self.served += 1
            threading.Timer(0.05, self.boot, ("ota-SMK37" + BUILD.hex()[:16], "USB-Midi")).start()
            return
        super().rx(d)


def run(args, dev):
    out, err = io.StringIO(), io.StringIO()
    old, sys.stderr = sys.stderr, err
    try:
        rc = S.main(args, backend=dev, out=out, ask=lambda _p: True)
    except I.InstallError as e:
        rc, err = -1, io.StringIO(str(e))
    finally:
        sys.stderr = old
    return rc, out.getvalue(), err.getvalue()


tmp = Path(tempfile.mkdtemp(prefix="smk37-install-"))
raw = fake.package(PRODUCT, marker=False)
pkg = tmp / "openvalive-smk37-elite-0.0.4.fwsc"
pkg.write_bytes(raw)
image = I.logical_image(raw)

dev = FakeSMK(image)
rc, out, err = I.main([str(pkg), "--yes", "--force"], backend=dev, out=io.StringIO(), ask=lambda _p: True), "", ""
ok(rc == 4 and dev.served == 9 and not dev.arms, "fm1_install.py alone: stops after 9 requests (the bug)")

rc, out, err = run([str(pkg), "--yes", "--force"], FakeSMK(image))
ok(rc == -1 and ".build_id is missing" in err, "no .build_id beside the package: refused before anything")

pkg.with_suffix(".build_id").write_text(BUILD.hex() + "\n")
dev = FakeSMK(image)
rc, out, err = run([str(pkg), "--yes", "--force"], dev)
ok(rc == 0 and len(dev.arms) == 1 and dev.bad == 0 and f"runs {PRODUCT}" in out,
   "armed (family 9, length, build id), loader ota-SMK37…, installed")
dev = FakeSMK(image, family=8)
rc, out, err = run([str(pkg), "--yes", "--force"], dev)
ok(rc != 0 and "SMK-37 Pro" in err and not dev.arms and dev.upgrades == 0, "a Pro answering: refused, nothing sent")
dev = FakeSMK(image, installed=BUILD)
rc, out, err = run([str(pkg), "--yes", "--force"], dev)
ok(rc != 0 and "already runs" in err and dev.upgrades == 0, "the same build already there: said, nothing sent")
dev = FakeSMK(image, busy=1)
rc, out, err = run([str(pkg), "--yes", "--force"], dev)
ok(rc != 0 and "busy" in err and dev.upgrades == 0, "busy with another update: said, nothing sent")
rc, out, err = run(["--info"], FakeSMK(image))
ok(rc == 0 and "SMK-37 Elite_8000003" in out, "--info")
print(f"INSTALL TESTS FAILED ({failed})" if failed else "install tests passed")
sys.exit(1 if failed else 0)
