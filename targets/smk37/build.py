#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
# Copyright (c) 2026 Tom-Rfid (OpenVALive). The package writer is a port of smk37_open sdk/Package.cs
# (MIT: Copyright (c) 2023 Andrey Grigoryev (kagaimiq), 2026 M-VAVE; see THIRD_PARTY notes in README.md).
"""OpenVALive for the SMK-37 Elite / Pro: build the firmware package (.fwsc), on Linux.

    python3 targets/smk37/build.py --model elite --smk37-open DIR --fm1-base DIR [--version 0.3.0] [--out DIR]
    python3 targets/smk37/build.py --check ...      the app's C through the host compiler only (no toolchain)

It does what smk37_open's sdk/Build.ps1 does on Windows, with JieLi's Linux toolchain (JIELI_TOOLCHAIN, the
same PI32v2 clang 4.0.1 as fm1-base): our sources and M-VAVE's panel scanner are compiled, linked with the SDK
(sdk/lib), and packed with the SDK's update loader. The calibration is never written: the scanner only reads it.
"""
import argparse
import hashlib
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
USER_BYTES = 536576
FAMILY = {"pro": 8, "elite": 9}
MODEL_NAME = {"pro": "Pro", "elite": "Elite"}


def run(cmd, **kw):
    r = subprocess.run([str(c) for c in cmd], capture_output=True, text=True, **kw)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit(f"build: {Path(str(cmd[0])).name} failed")
    return r.stdout


# ------------------------------------------------------------------ the package (port of sdk/Package.cs)

AREA, ENTRY_POINT, APP_DATA = 0x4000, 0x02000120, 0x120
FLASH_LIMIT, BT_SIZE, MIN_VM = 0xFF000, 4096, 32768
PUBLIC_CODE = 0x980F


def _crc_table():
    t = []
    for i in range(256):
        v = i << 8
        for _ in range(8):
            v = ((v << 1) ^ (0x1021 if v & 0x8000 else 0)) & 0xFFFF
        t.append(v)
    return t


CRC_TABLE = _crc_table()


def crc16(b, begin=0, length=None):
    v = 0
    end = len(b) if length is None else begin + length
    for i in range(begin, end):
        v = ((v << 8) & 0xFFFF) ^ CRC_TABLE[(v >> 8) ^ b[i]]
    return v


def cipher(b, begin, length, state):
    for i in range(begin, begin + length):
        b[i] ^= state & 0xFF
        state = ((state << 1) ^ (0x1021 if state & 0x8000 else 0)) & 0xFFFF


def round_up(n, a):
    return (n + a - 1) & -a


class Image:
    def __init__(self, size, fill):
        self.b = bytearray([fill]) * size

    def number(self, off, value, width):
        for n in range(width):
            self.b[off + n] = (value >> (8 * n)) & 0xFF

    def copy(self, off, data):
        self.b[off:off + len(data)] = data

    def text(self, off, s):
        self.copy(off, s.encode("ascii"))

    def checksum(self, field, off, size):
        self.number(field, crc16(self.b, off, size), 2)

    def record(self, off, name, crc, address, size, attributes, last, extra):
        self.b[off:off + 32] = bytes(32)
        self.number(off + 2, crc, 2)
        self.number(off + 4, address & 0xFFFFFFFF, 4)
        self.number(off + 8, size & 0xFFFFFFFF, 4)
        self.number(off + 12, attributes, 2)
        self.number(off + 14, last, 2)
        for n in range(16, 32):
            self.b[off + n] = 255
        self.text(off + 16, name)
        self.b[off + 16 + len(name)] = 0
        if extra is not None:
            self.number(off + 28, extra, 4)
        self.checksum(off, off + 2, 30)


def read_resource(d, name, size):
    b = (d / name).read_bytes()
    if len(b) != size:
        raise SystemExit(f"build: unexpected SDK resource {name}")
    return b


def encode_boot(plain):
    result = bytearray(plain)
    banks = struct.unpack_from("<H", plain, 0)[0]
    if banks < 1 or banks > len(plain) // 16:
        raise SystemExit("build: invalid boot directory")
    prev_end = banks * 16
    for i in range(banks):
        h = i * 16
        length = struct.unpack_from("<H", plain, h + 2)[0]
        start = struct.unpack_from("<i", plain, h + 8)[0]
        if (length == 0 or start < prev_end or start > len(plain) - length
                or crc16(plain, h, 14) != struct.unpack_from("<H", plain, h + 14)[0]
                or crc16(plain, start, length) != struct.unpack_from("<H", plain, h + 12)[0]):
            raise SystemExit("build: invalid boot bank")
        cipher(result, h, 16, 0xFFFF)
        cipher(result, start, length, 0xFFFF)
        prev_end = start + length
    return bytes(result)


def build_flash(app, d, end, vm_end):
    boot = read_resource(d, "uboot.boot", 14488)
    config = read_resource(d, "boot-config.bin", 721)
    tool = read_resource(d, "cfg_tool.bin", 383)
    image, settings = Image(end, 255), Image(34 + len(config), 0)
    for bit in range(16):
        settings.b[bit] = 16
        settings.b[16 + bit] = 16 ^ (255 if ((PUBLIC_CODE >> bit) & 1) == 0 else 0)
    settings.checksum(32, 0, 32)
    settings.copy(34, config)
    image.copy(0, bytes([0, 0, 0x20, 2, 0x2f, 0x10, 0x4c, 0xc9, 0, 0xf0, 0x0f, 0, 0x10, 0x10, 0xa4, 0xff,
                         0x4e, 0x5d, 0x0b, 0x41, 0xe0, 0xcd, 0x78, 0x1d, 0xc8, 0x56, 0x41, 0x7f, 0xb3, 0x67, 0xcf, 0x9f]))
    image.checksum(0, 2, 30)
    config_at = 160 + len(boot)
    image.record(32, "uboot.boot", crc16(boot), 160, len(boot), 0, 0, None)
    image.record(64, "isd_config.ini", crc16(settings.b), config_at, len(settings.b), 0x8002, 0, None)
    image.record(96, "app_dir_head", 65535, AREA, 0xFFFFFFFF, 0xff81, 0, None)
    image.record(128, "key_mac", 65535, FLASH_LIMIT, 4096, 0x0112, 65535, None)
    for p in range(0, 160, 32):
        cipher(image.b, p, 32, 0xFFFF)
    image.copy(160, encode_boot(boot))
    image.copy(config_at, settings.b)
    payload = APP_DATA + len(app) + len(tool)
    at = AREA + APP_DATA
    image.copy(at, app)
    image.copy(at + len(app), tool)
    image.record(AREA + 32, "app.bin", crc16(app), APP_DATA, len(app), 0xff82, 0, None)
    image.record(AREA + 64, "cfg_tool.bin", crc16(tool), APP_DATA + len(app), len(tool), 0xff82, 0, None)
    image.record(AREA + 96, "VM", 65535, end, vm_end - end, 0x8112, 0, MIN_VM)
    image.record(AREA + 128, "PRCT", 65535, 0, end, 0x8292, 0, 0x45444f43)
    image.record(AREA + 160, "BTIF", 65535, vm_end, BT_SIZE, 0x8192, 1, 0x4f545541)
    image.record(AREA, "app_area_head", crc16(image.b, AREA + 32, payload - 32), ENTRY_POINT, payload, 0xff83, 0, None)
    reserved = AREA + payload
    image.record(reserved + 32, "USER", 65535, FLASH_LIMIT - USER_BYTES, USER_BYTES, 0x8192, 1, 0x4f545541)
    image.record(reserved, "EXT_RESERVED", crc16(image.b, reserved + 32, 32), 32, 64, 0xff93, 1, None)
    for p in range(0, payload + 64, 32):
        cipher(image.b, AREA + p, min(32, payload + 64 - p), PUBLIC_CODE ^ (p >> 2))
    return image


def wrap(flash, loader, product):
    if not 1 <= len(loader) < 32768:
        raise SystemExit("build: invalid update loader")
    loader_at = 1024 + round_up(len(flash.b), 32)
    total = loader_at + round_up(len(loader), 32)
    ufw = Image(total, 255)
    ufw.b[0:224] = bytes(224)
    files, names = [bytes(flash.b), loader], ["flash.bin", "ota.bin"]
    for i, f in enumerate(files):
        rec = 64 + i * 80
        ufw.number(rec, 0 if i == 0 else 100, 2)
        ufw.number(rec + 2, i, 2)
        ufw.number(rec + 4, crc16(f), 2)
        ufw.number(rec + 8, 1024 if i == 0 else loader_at, 4)
        ufw.number(rec + 12, len(f), 4)
        ufw.number(rec + 16, round_up(len(f), 32), 4)
        ufw.text(rec + 64, names[i])
        cipher(ufw.b, rec, 80, 0xFFFF)
    ufw.number(4, total, 4)
    ufw.number(8, 2, 2)
    ufw.number(10, 4, 2)
    ufw.number(12, 512, 4)
    ufw.text(16, "AC791N")
    ufw.checksum(2, 64, 160)
    ufw.checksum(0, 2, 62)
    cipher(ufw.b, 0, 64, 0xFFFF)
    ufw.copy(1024, files[0])
    ufw.copy(loader_at, loader)
    out = bytearray()
    for src in range(total):
        out.append(ufw.b[src])
        if src < 940 and (src + 1) % 47 == 0:
            idx = src // 47
            out.append((ord(product[idx]) + idx + 1) & 0xFF if idx < len(product) else 125)
    return bytes(out)


def make_package(app, resources, product):
    if not re.fullmatch(r"SMK-37 (?:Pro|Elite)_8[0-9]{6}", product):
        raise SystemExit("build: invalid SMK-37 update identity")
    limit = FLASH_LIMIT - USER_BYTES - BT_SIZE - MIN_VM - AREA - APP_DATA - 383 - 64
    if not app or len(app) > limit:
        raise SystemExit(f"build: the app image is {len(app)} bytes, {limit} fit")
    image_size = round_up(AREA + APP_DATA + len(app) + 383 + 64, 4096)
    user_at = FLASH_LIMIT - USER_BYTES
    vm_end = user_at - BT_SIZE
    if image_size > vm_end - MIN_VM:
        raise SystemExit("build: no room left for the settings (VM)")
    flash = build_flash(app, resources, image_size, vm_end)
    return wrap(flash, (resources / "ota.bin").read_bytes(), product), image_size, limit


# ------------------------------------------------------------------ compile and link

CFLAGS = ["-target", "pi32v2", "-integrated-as", "-mcpu=r3", "-mfprev1", "-Wuninitialized", "-Wno-invalid-noreturn",
          "-fno-common", "-fallow-pointer-null", "-fprefer-gnu-section", "-femulated-tls",
          "-Wno-shift-negative-value", "-Wframe-larger-than=2560", "-fms-extensions", "-fno-unwind-tables",
          "-ffunction-sections", "-fdata-sections", "-fmessage-length=0", "-cl-single-precision-constant",
          "-mllvm", "-pi32v2-large-program=true", "-D__GCC_PI32V2__", "-fwrapv", "-fno-fast-math",
          "-ffp-contract=off", "-fno-lto"]


def includes(sdk, base, gen):
    return [sdk / "sdk/include", sdk / "sdk/include/libc", sdk / "sdk/config", sdk / "app", sdk / "app/board",
            sdk / "app/synth", base / "include", base / "lib", base / "device", gen, REPO / "va/app", HERE]


def sources(sdk):
    """(file, optimisation, extra flags): the panel scanner as smk37_open builds it, ours, the DSP (va/)"""
    return [(sdk / "app/board/scan.c", "-O2", ["-fno-pic"]),
            (HERE / "board/panel_ols.c", "-O2", ["-fno-pic"]),
            (HERE / "board/audio_ols.c", "-O2", []),
            (HERE / "smk37_sdk.c", "-O2", []),
            (HERE / "smk37_app.c", "-Oz", []),
            (REPO / "va/app/ols_dsp.c", "-O2", [])]


def generate_fonts(base, gen):
    gen.mkdir(parents=True, exist_ok=True)
    # each glyph only its lit box, coded (fm1.toml font_trim / font_pack): the SDK leaves little flash, fonts are big
    env = {**os.environ, "FM1_FONT_FULL": "", "FM1_FONT_TRIM": "1", "FM1_FONT_PACK": "1"}
    r = subprocess.run([sys.executable, str(base / "tools/gen_font.py"), str(gen / "felucca_font.h")],
                       capture_output=True, text=True, env=env)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit("build: gen_font.py failed")


def product_id(model, version):
    major, minor, patch = (int(x) for x in (version.split("-")[0].split(".") + ["0", "0"])[:3])
    return f"SMK-37 {MODEL_NAME[model]}_8{major:02d}{minor:02d}{patch:02d}"


def check_host(args, gen):
    """the app's C with the host compiler, no toolchain: what CI's tests run (the board files need the chip's)"""
    cc = os.environ.get("CC", "cc")
    inc = [f"-I{p}" for p in includes(args.smk37_open, args.fm1_base, gen) if not str(p).endswith("libc")]
    for src in (HERE / "smk37_app.c", HERE / "smk37_sdk.c"):
        run([cc, "-std=gnu11", "-fsyntax-only", "-DFM1_HOST_TEST", "-Wall", "-Wno-unused-function",
             "-Wno-unused-variable", "-Wno-unused-but-set-variable", "-Wno-missing-braces", *inc, str(src)])
    print("smk37: the app compiles (host check)")


def host_test(args, gen):
    """the app on the host with a fake SDK and panel (tests/host_test.c): plays the controls, checks the result"""
    cc = os.environ.get("CC", "cc")
    inc = [f"-I{p}" for p in includes(args.smk37_open, args.fm1_base, gen) if not str(p).endswith("libc")]
    exe = args.out / "smk37_host_test"
    args.out.mkdir(parents=True, exist_ok=True)
    run([cc, "-std=gnu11", "-O1", "-DFM1_HOST_TEST", "-Wall", "-Wno-unused-function", "-Wno-unused-parameter",
         "-Werror", *inc, HERE / "tests/host_test.c", REPO / "va/app/ols_dsp.c", "-o", exe, "-lm"])
    sys.stdout.write(run([exe]))


# what must run from internal RAM (smk37_open Build.ps1 checks the same): the interrupts that run while the flash
# is busy, and the panel scan's tables
MUST_BE_IN_RAM = ["fm1_audio_irq", "scan_isr", "scan_soft_isr", "smk37_adc_irq", "key_scan", "encoder_scan",
                  "pad_prc", "keymap", "encodermap", "pad_velocity", "fm1_engine_render"]


def check_ram_symbols(tc, elf):
    """the symbols above inside internal SRAM (0x01C00000..0x01C80000), with the toolchain's nm"""
    nm = next((p for p in (tc / "pi32v2/bin/llvm-nm", tc / "common/bin/llvm-nm", tc / "common/bin/nm") if p.exists()), None)
    objdump = tc / "common/bin/objdump"
    where = {}
    if nm:
        for line in run([nm, elf]).splitlines():
            parts = line.split()
            if len(parts) == 3:
                where[parts[2]] = int(parts[0], 16)
    elif objdump.exists():                      # "01c01234 l     F .volatile_ram_code 00000040 scan_isr"
        for line in run([objdump, "-t", elf]).splitlines():
            parts = line.split()
            if len(parts) >= 4 and re.fullmatch(r"[0-9a-fA-F]{8}", parts[0]):
                where[parts[-1]] = int(parts[0], 16)
    else:
        print("smk37: no nm / objdump in the toolchain, RAM placement not checked")
        return
    for name in MUST_BE_IN_RAM:
        if name not in where:
            raise SystemExit(f"build: {name} is missing from the firmware")
        if not 0x01C00000 <= where[name] < 0x01C80000:
            raise SystemExit(f"build: {name} is outside internal RAM (0x{where[name]:08x})")
    print(f"smk37: {len(MUST_BE_IN_RAM)} interrupt / scan symbols checked in internal RAM")


def ram_report(mapfile, top=12):
    """the biggest pieces of RAM (.pool, .bss, .data) the map shows: what to shrink when the heap runs short"""
    lines = mapfile.read_text(errors="replace").splitlines()
    pieces, pending = [], None
    for line in lines:
        m = re.match(r"^ (\.(?:pool|bss|data|sbss)[^\s]*)(?:\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S+))?\s*$", line)
        if m and m.group(2):
            pieces.append((int(m.group(3), 16), m.group(1), m.group(4)))
            pending = None
        elif m:
            pending = m.group(1)
        elif pending:
            m2 = re.match(r"^\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\S+)", line)
            if m2:
                pieces.append((int(m2.group(2), 16), pending, m2.group(3)))
            pending = None
    pieces.sort(reverse=True)
    for size, name, obj in pieces[:top]:
        print(f"  ram {size:7d}  {name}  ({Path(obj).name})")


def flash_report(objdump, elf, top=40):
    """the biggest pieces of the app image (code and constants): what to shrink when it does not fit"""
    if not objdump.exists():
        for other in (objdump.parent.parent.parent / "pi32v2/bin/llvm-objdump", objdump.parent.parent.parent / "pi32v2/bin/objdump",
                      objdump.parent / "llvm-objdump"):
            if other.exists():
                objdump = other
                break
        else:
            print(f"  (no objdump at {objdump}: no flash report)")
            return
    pieces = []
    for line in run([objdump, "-t", elf]).splitlines():
        f = line.split()
        if len(f) >= 6 and (f[-3].startswith((".text", ".rodata", ".data")) or f[-3] in (".text", ".data")):
            try:
                pieces.append((int(f[-2], 16), f[-1], f[-3]))
            except ValueError:
                pass
    pieces.sort(reverse=True)
    for size, name, section in pieces[:top]:
        print(f"  flash {size:7d}  {name}  ({section})")


def build(args, gen):
    tc = Path(os.environ.get("JIELI_TOOLCHAIN", ""))
    clang, ld = tc / "pi32v2/bin/clang", tc / "pi32v2/bin/ld"
    plugin, objcopy = tc / "pi32v2/bin/LLVMgold.so", tc / "common/bin/objcopy"
    if not clang.exists():
        raise SystemExit("build: JIELI_TOOLCHAIN must point at JieLi's Linux toolchain (fm1-base tools/get_toolchain.sh)")
    sdk, out = args.smk37_open, args.out / args.model
    obj = out / "obj"
    obj.mkdir(parents=True, exist_ok=True)
    flags = CFLAGS + [f"-DSMK37_MODEL_{args.model.upper()}=1", f"-DOLS_VERSION=\"{args.version}\""]
    if not args.bluetooth:
        flags.append("-DOLS_NO_BLUETOOTH=1")
    flags += [f"-I{p}" for p in includes(sdk, args.fm1_base, gen)]
    objects = []
    for i, (src, opt, extra) in enumerate(sources(sdk)):
        o = obj / f"{i}_{src.stem}.o"
        print(f"compile {src.relative_to(src.parents[2]) if len(src.parents) > 2 else src}")
        run([clang, *flags, opt, *extra, "-c", src, "-o", o])
        objects.append(o)

    product = product_id(args.model, args.version)
    digest = hashlib.sha256()                   # the build id: every source and header of ours, CORE's included
    for src in [s for s, _, _ in sources(sdk)] + sorted(p for d in (HERE, REPO / "va/app", REPO / "va/core")
                                                       for p in d.rglob("*") if p.suffix in (".c", ".h")
                                                       and "build" not in p.relative_to(d).parts):
        digest.update(src.read_bytes())
    digest.update(product.encode())
    ident = obj / "build_identity.c"
    ident.write_text('#include "layout.h"\n'
                     f"const struct fm1_firmware_layout fm1_layout = {{{USER_BYTES}U,{FAMILY[args.model]}U}};\n"
                     "const unsigned char fm1_build_id[32] = {" + ",".join(f"0x{b:02x}" for b in digest.digest()) + "};\n"
                     f'const unsigned char fm1_update_identity[27] = "{product}";\n')
    run([clang, *flags, "-O2", "-c", ident, "-o", obj / "build_identity.o"])
    objects.append(obj / "build_identity.o")

    # the SDK's linker script, as Build.ps1 fills it, plus fm1-base's .pool (the app's big buffers) in RAM0's BSS
    script = (sdk / "sdk/config/linker.ld.in").read_text()
    if args.bluetooth:
        script = script.replace("@SBC_DECODER@", "sbc_decoder\n").replace("@ACL_TX_EXTRA@", " + 1024")
        script = script.replace("@ACL_RX_EXTRA@", " + 4096")
    else:
        # no Bluetooth: no SBC decoder forced in, no ACL pools, and its tables no longer KEPT (they go with the stack)
        script = script.replace("@SBC_DECODER@", "").replace("@ACL_TX_EXTRA@", "").replace("@ACL_RX_EXTRA@", "")
        for table in (".tws_func_stub", ".a2dp_sink_media_codec", ".a2dp_event_handler", ".sdp_record_item", ".bt_sleep"):
            keep = f"KEEP(*({table}))"
            if keep not in script:
                raise SystemExit(f"build: the SDK's linker script changed (no {keep})")
            script = script.replace(keep, f"*({table})")
    if "        *(.bss .bss.*)\n" not in script:
        raise SystemExit("build: the SDK's linker script changed (no .bss line to put .pool before)")
    script = script.replace("        *(.bss .bss.*)\n", "        *(.pool .pool.*)\n        *(.bss .bss.*)\n", 1)
    (out / "linker.ld").write_text(script)

    elf, mapfile = out / "openvalive-smk37.elf", out / "openvalive-smk37.map"
    if args.bluetooth:
        engine = ["--whole-archive", sdk / "sdk/lib/libfm1_engine.a", "--no-whole-archive"]
        keep_bt = ["--undefined=connectivity_wl82_bluetooth_start", "--undefined=audio_app_bluetooth_open",
                   "--undefined=bt_get_local_name", "--undefined=bt_get_mac_addr"]
    else:
        # M-VAVE's engine with its calls into the Bluetooth stack pointed at a stub: the stack is left out
        # (named as M-VAVE's: the linker script places "*startup.S.o*" first, at the fixed ROM entry)
        (obj / "nobt").mkdir(exist_ok=True)
        engine, keep_bt = [obj / "nobt" / "engine_startup.S.o"], []
        run([sys.executable, HERE / "no_bluetooth.py", sdk / "sdk/lib/libfm1_engine.a", engine[0]])
    run([ld, "--plugin", plugin, "-o", elf, *objects,
         "--plugin-opt=-pi32v2-always-use-itblock=false", "--plugin-opt=-enable-ipra=true",
         "--plugin-opt=-pi32v2-merge-max-offset=4096", "--plugin-opt=-pi32v2-enable-simd=true",
         "--plugin-opt=mcpu=r3", "--plugin-opt=-global-merge-on-const", "--plugin-opt=-inline-threshold=5",
         "--plugin-opt=-inline-max-allocated-size=32", "--plugin-opt=-inline-normal-into-special-section=true",
         "--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar",
         "--plugin-opt=-pi32v2-enable-rep-memop", "--plugin-opt=-emulated-tls", "--sort-common",
         "--plugin-opt=-pi32v2-large-program=true", "--plugin-opt=-pi32v2-csync-before-rti=true",
         "--start-group", *engine, sdk / "sdk/lib/libc.a", sdk / "sdk/lib/libcompiler_rt.a", "--end-group",
         "--gc-sections", "-T", out / "linker.ld", f"-M={mapfile}", *keep_bt])

    e = elf.read_bytes()
    if e[:6] != b"\x7fELF\x01\x01" or struct.unpack_from("<I", e, 24)[0] != ENTRY_POINT:
        raise SystemExit("build: the firmware's entry is not the fixed ROM entry")
    check_ram_symbols(tc, elf)
    m = re.search(r"0x([0-9a-fA-F]+)[^\r\n]*_MALLOC_SIZE[ ]*=", mapfile.read_text(errors="replace"))
    heap = int(m.group(1), 16) if m else 0
    if not 55296 <= heap <= 56320:
        ram_report(mapfile)
        raise SystemExit(f"build: the SDK's heap would be {heap} bytes; tried on the keyboard: 55788 starts, 63980 stays "
                         "stuck on the logo without USB-MIDI, about 52 KB answers no firmware update")
    app = b""
    for section in (".text", ".data", ".ram0_data", ".cache_ram_data"):
        b = out / f"{section[1:]}.bin"
        run([objcopy, "-O", "binary", "-j", section, elf, b])
        app += b.read_bytes()
        b.unlink()
    try:
        package, image_size, limit = make_package(app, sdk / "sdk/tools/pack", product)
    except SystemExit:
        flash_report(tc / "common/bin/objdump", elf)
        raise
    name = f"openvalive-smk37-{args.model}-{args.version}.fwsc"
    (out / name).write_bytes(package)
    # the keyboard's firmware takes only a package it is told about (cmd 50: its build id): scripts/smk37_install.py
    (out / name).with_suffix(".build_id").write_text(digest.hexdigest() + "\n")
    print(f"smk37 {args.model}: app image {len(app)} B of {limit}; flash {image_size} B; SDK heap {heap} B; "
          f"identity {product}; {out / name} {len(package)} B")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--model", choices=["elite", "pro"], default="elite")
    ap.add_argument("--smk37-open", type=Path, default=Path(os.environ.get("SMK37_OPEN", "")),
                    help="M-VAVE's smk37_open (github.com/m-vave/smk37_open)")
    ap.add_argument("--fm1-base", type=Path, default=Path(os.environ.get("FM1_BASE", "")))
    ap.add_argument("--version", default=(REPO / "VERSION").read_text().strip())
    ap.add_argument("--out", type=Path, default=HERE / "build")
    ap.add_argument("--check", action="store_true", help="host compiler syntax check only")
    ap.add_argument("--bluetooth", action="store_true",
                    help="link M-VAVE's Bluetooth stack (off by default: no_bluetooth.py leaves it out, ~180 KB)")
    ap.add_argument("--test", action="store_true", help="the host test (fake SDK and panel), no toolchain")
    args = ap.parse_args()
    for p, what in ((args.smk37_open / "sdk/lib/libfm1_engine.a", "--smk37-open"),
                    (args.fm1_base / "include/plat.h", "--fm1-base")):
        if not p.exists():
            raise SystemExit(f"build: {what} does not point at it ({p} missing)")
    gen = args.out / "gen"
    generate_fonts(args.fm1_base, gen)
    if args.check:
        check_host(args, gen)
    elif args.test:
        host_test(args, gen)
    else:
        build(args, gen)


if __name__ == "__main__":
    main()
