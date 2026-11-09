#!/usr/bin/env python3
"""Regenerate firmware/boards/175c/MotoNav from the 175 base + this overlay."""

import shutil
from pathlib import Path

PORT = Path(__file__).resolve().parent
ROOT = PORT.parents[1]
BASE = ROOT / "firmware/boards/175/MotoNav"
TARGET = ROOT / "firmware/boards/175c/MotoNav"


def main():
    if not BASE.is_dir():
        raise SystemExit("missing 175 base: " + str(BASE))
    if TARGET.exists():
        shutil.rmtree(TARGET)
    shutil.copytree(BASE, TARGET)
    shutil.copy2(PORT / "pin_config.h", TARGET / "pin_config.h")
    for path in sorted((PORT / "overlay").rglob("*")):
        if path.is_file():
            dest = TARGET / path.relative_to(PORT / "overlay")
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)
    print("synced " + str(TARGET))


if __name__ == "__main__":
    main()
