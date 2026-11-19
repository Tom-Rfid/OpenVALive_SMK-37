# SPDX-License-Identifier: GPL-3.0-only WITH GPL-3.0-linking-exception
# Copyright (c) 2026 Techmelec (OpenVALive).
"""OpenVALive SMK-37 without Bluetooth: M-VAVE's engine object with every call from the rest of the SDK into the
Bluetooth stack (classic and BLE: controller, host, A2DP/SBC audio, BLE MIDI) pointed at one function of ours that
returns 0 (smk37_sdk.c: groove_midi_panel_reset). Nothing reaches the stack any more, so the linker's --gc-sections
leaves it out (~180 KB of flash). Bluetooth *data* is not touched (data is never called).

The object is M-VAVE's (smk37_open, sdk/lib/libfm1_engine.a): it is only patched in our build directory, never
redistributed. Pure Python, no tool: an ELF32 little-endian relocatable, its .rela sections rewritten in place.

  python3 no_bluetooth.py libfm1_engine.a out.o      (prints what it cut)"""
import re
import struct
import sys
from pathlib import Path

STUB = "groove_midi_panel_reset"
# the Bluetooth code: its section families, and the .text.* functions that belong to it
BT = re.compile(r"^\.(bt_|classic_|ble_|hci_|uECC|crypto|hmac|link_bulk|link_task|lmp|bd_base|tws|a2dp|sdp_|avctp|avdtp|"
                r"bredr)|^\.text\.(.*bluetooth|ble_|.*_ble_|.*sbc|a2dp|avdtp|avctp|l2cap|att_|gatt|hci_|sdp_|"
                r"btstack|bredr|lmp|role_|central_|peripheral|connectivity_platform_ble|set_discoverable|cbk_|rfcomm|"
                r"spp_|classic_|dut_|disconnected|midi_packet_send|link_conn|decoder_|audio_app_a2dp|get_buffer_vaild|"
                r"check_connetion|bt_)|^\.rodata\.(sbc|fm1_io_start\.backend$)", re.I)
# calls that must be among the cut ones (else M-VAVE's object changed: stop rather than guess)
EXPECTED = {(".text.fm1_io_start", "connectivity_wl82_bluetooth_start"), (".text.fm1_port_ready", "ble_midi_output_ready"),
            (".lp_target", "btstack_lowpower_idle_query")}
SHF_EXECINSTR = 0x4


def object_of(archive: bytes) -> bytes:
    """the one member of M-VAVE's archive (engine_startup.S.o)"""
    if archive[:8] != b"!<arch>\n":
        raise SystemExit("no_bluetooth: not an ar archive")
    at, members, longnames = 8, [], b""
    while at + 60 <= len(archive):
        head = archive[at:at + 60]
        size = int(head[48:58].decode().strip())
        name = head[:16].decode().strip()
        data = archive[at + 60:at + 60 + size]
        if name == "//":
            longnames = data                             # (GNU ar: the long names)
        elif name.startswith("/") and name[1:].isdigit():
            off = int(name[1:])
            members.append((longnames[off:longnames.index(b"\n", off)].decode(), data))
        elif not name.startswith("/"):
            members.append((name, data))
        at += 60 + size + (size & 1)
    objs = [d for n, d in members if n.endswith(".o") or n.endswith(".o/")]
    if len(objs) != 1:
        raise SystemExit(f"no_bluetooth: expected one object in the archive, found {len(objs)}")
    return objs[0]


def patch(elf: bytearray):
    if elf[:6] != b"\x7fELF\x01\x01":
        raise SystemExit("no_bluetooth: not an ELF32 little-endian object")
    shoff, = struct.unpack_from("<I", elf, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", elf, 0x2E)
    sh = [struct.unpack_from("<10I", elf, shoff + i * shentsize) for i in range(shnum)]
    shstr = sh[shstrndx][4]

    def cstr(off):
        return elf[off:elf.index(b"\0", off)].decode()

    names = [cstr(shstr + s[0]) for s in sh]
    symtab = next(i for i, s in enumerate(sh) if s[1] == 2)
    strtab = sh[sh[symtab][6]][4]
    nsym = sh[symtab][5] // 16
    syms = []
    for i in range(nsym):
        st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from("<IIIBBH", elf, sh[symtab][4] + 16 * i)
        syms.append((cstr(strtab + st_name), st_shndx, st_info))
    stub = [i for i, (n, ndx, info) in enumerate(syms) if n == STUB and ndx == 0]
    if len(stub) != 1:
        raise SystemExit(f"no_bluetooth: {STUB} is not an undefined symbol of the object")
    stub = stub[0]

    def bt(ndx):
        return 0 < ndx < shnum and bool(BT.search(names[ndx]))

    def code(ndx):
        return 0 < ndx < shnum and bool(sh[ndx][2] & SHF_EXECINSTR)

    cut = []
    for i, s in enumerate(sh):
        if s[1] != 4:                                   # SHT_RELA
            continue
        frm = s[7]
        if names[frm] == ".eh_frame" or bt(frm):
            continue                                    # (unwind tables keep nothing; Bluetooth may call itself)
        for k in range(s[5] // 12):
            at = s[4] + 12 * k
            info, = struct.unpack_from("<I", elf, at + 4)
            sym = info >> 8
            ndx = syms[sym][1]
            if bt(ndx) and code(ndx):
                struct.pack_into("<I", elf, at + 4, (stub << 8) | (info & 0xFF))
                cut.append((names[frm], syms[sym][0] or names[ndx]))
    missing = EXPECTED - set(cut)
    if missing:
        raise SystemExit(f"no_bluetooth: M-VAVE's object changed (not found: {sorted(missing)}): not patched")
    return cut


def main():
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    data = src.read_bytes()
    elf = bytearray(object_of(data) if data[:8] == b"!<arch>\n" else data)
    cut = patch(elf)
    out.write_bytes(elf)
    print(f"no_bluetooth: {len(cut)} calls into the Bluetooth stack now go to {STUB}")
    for frm, to in sorted(set(cut)):
        print(f"  {frm} -> {to}")


if __name__ == "__main__":
    main()
