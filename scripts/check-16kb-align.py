#!/usr/bin/env python3
"""Check that every arm64-v8a .so in an APK has 16 KB-aligned ELF LOAD segments.

Android 15/16 on a 16 KB page-size device refuses (or warns about) native
libraries whose PT_LOAD p_align is only 4 KB. This script reads the ELF
program headers directly from the APK zip; no ndk/readelf needed.

Usage:
  python3 scripts/check-16kb-align.py android/standard/app/build/outputs/apk/debug/app-debug.apk
  python3 scripts/check-16kb-align.py --min-align 16384 path/to/app.apk
"""

import argparse
import struct
import sys
import zipfile


def elf_load_aligns(data: bytes) -> list[int]:
    if len(data) < 64 or data[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    if data[4] != 2:
        raise ValueError("not ELF64")
    endian = "<" if data[5] == 1 else ">"
    e_phoff = struct.unpack_from(endian + "Q", data, 32)[0]
    e_phentsize = struct.unpack_from(endian + "H", data, 54)[0]
    e_phnum = struct.unpack_from(endian + "H", data, 56)[0]
    aligns = []
    for i in range(e_phnum):
        off = e_phoff + i * e_phentsize
        if off + 56 > len(data):
            break
        p_type = struct.unpack_from(endian + "I", data, off)[0]
        if p_type != 1:  # PT_LOAD
            continue
        p_align = struct.unpack_from(endian + "Q", data, off + 48)[0]
        aligns.append(p_align)
    return aligns


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("apk")
    ap.add_argument("--min-align", type=int, default=16384,
                    help="minimum p_align (default 16384 = 16 KB)")
    args = ap.parse_args()

    bad = []
    checked = 0
    with zipfile.ZipFile(args.apk) as z:
        names = sorted(n for n in z.namelist()
                       if n.startswith("lib/arm64-v8a/") and n.endswith(".so"))
        if not names:
            print("no arm64-v8a .so in " + args.apk)
            return 1
        for name in names:
            data = z.read(name)
            try:
                aligns = elf_load_aligns(data)
            except ValueError as e:
                bad.append((name, str(e)))
                print(f"FAIL {name}: {e}")
                continue
            checked += 1
            worst = min(aligns) if aligns else 0
            ok = bool(aligns) and worst >= args.min_align
            print(f"{'OK  ' if ok else 'FAIL'} {name}: LOAD p_align={worst:#x}")
            if not ok:
                bad.append((name, f"p_align {worst:#x} < {args.min_align:#x}"))

    print(f"\n{checked - len(bad)}/{checked} arm64 libs meet "
          f"{args.min_align // 1024} KB alignment")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
