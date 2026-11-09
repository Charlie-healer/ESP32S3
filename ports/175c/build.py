#!/usr/bin/env python3
"""Build MotoNav firmware for the Waveshare ESP32-S3-Touch-AMOLED-1.75C.

The complete source tree is kept at firmware/ (the StopWatch fork base).
Sources are copied to ports/175c/.build/, the 1.75C pin_config.h and the
ports/175c/overlay files are applied, and arduino-cli compiles the sketch.
The script only builds; it never flashes.
"""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

PORT_DIR = Path(__file__).resolve().parent
REPO_ROOT = PORT_DIR.parents[1]
DEFAULT_UPSTREAM = REPO_ROOT / "MotoNav_stopwatch"
BOARD_DESC = "Waveshare ESP32-S3-Touch-AMOLED-1.75C"
ESP32_CORE = "3.3.10"
# 1.75C has 32MB flash, but the Arduino core 3.3.10 only ships a 16MB layout
# for ESP32-S3. Keep the proven 16MB partition table; the upper half stays unused.
FQBN = ("esp32:esp32:esp32s3:"
        "FlashSize=32M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc")
FLASH_SIZE = "32MB"
PARTITION = "app3M_fat9M_16MB (16MB layout)"


def fail(message):
    print("\n[错误] " + message, file=sys.stderr)
    sys.exit(1)


def find_version(sketch):
    for rel in ("src/core/FirmwareConfig.h", "MotoNav.ino"):
        path = sketch / rel
        if not path.exists():
            continue
        match = re.search(r'^#define FW_VERSION "v([^"]+)"',
                          path.read_text(encoding="utf-8", errors="ignore"),
                          re.MULTILINE)
        if match:
            return match.group(1)
    fail("无法从固件源码读取 FW_VERSION")


def default_upstream():
    # New repository layout: firmware/ is at the repo root. Older checkouts
    # kept the whole StopWatch fork in MotoNav_stopwatch/.
    if (REPO_ROOT / "firmware/boards/175/MotoNav").is_dir() and \
       (REPO_ROOT / "firmware/libraries").is_dir():
        return REPO_ROOT
    return DEFAULT_UPSTREAM


def prepare(upstream, clean):
    source = upstream / "firmware/boards/175/MotoNav"
    libraries = upstream / "firmware/libraries"
    if not source.is_dir():
        fail("找不到上游 1.75 源码目录: " + str(source))
    if not libraries.is_dir():
        fail("找不到上游库目录: " + str(libraries))

    build_root = PORT_DIR / ".build"
    sketch = build_root / "MotoNav"
    if clean and build_root.exists():
        shutil.rmtree(build_root)
    if sketch.exists():
        shutil.rmtree(sketch)
    build_root.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, sketch)
    shutil.copy2(PORT_DIR / "pin_config.h", sketch / "pin_config.h")

    overlay = PORT_DIR / "overlay"
    if overlay.is_dir():
        for path in overlay.rglob("*"):
            if path.is_file():
                destination = sketch / path.relative_to(overlay)
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, destination)

    output = build_root / "output"
    output.mkdir(parents=True, exist_ok=True)
    return sketch, libraries, build_root / "build", output


def compile_sketch(sketch, libraries, build_path, output, cli, jobs):
    executable = shutil.which(cli) or cli
    prefix_map = "-ffile-prefix-map=" + str(Path.home()) + "=/builder"
    command = [
        executable, "compile",
        "--jobs", str(jobs),
        "--libraries", str(libraries),
        "--build-path", str(build_path),
        "--fqbn", FQBN,
        "--build-property", "compiler.c.extra_flags=" + prefix_map,
        "--build-property", "compiler.cpp.extra_flags=" + prefix_map,
        "--build-property", "compiler.c.elf.extra_flags=-Wl,--wrap=xTaskCreatePinnedToCore",
        "--output-dir", str(output),
        str(sketch),
    ]
    print("[build] " + " ".join(command))
    try:
        subprocess.run(command, check=True)
    except FileNotFoundError:
        fail("未找到 Arduino CLI: " + cli + "\n"
             "  macOS:  brew install arduino-cli\n"
             "  Linux:  https://arduino.github.io/arduino-cli/latest/installation/\n"
             "  然后安装 ESP32 核心:\n"
             "    arduino-cli core install esp32:esp32@" + ESP32_CORE +
             " --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json\n"
             "  国内网络如果从 GitHub 下载工具链超时，改用官方中国镜像（核心代码相同，只是工具链走 dl.espressif.cn）:\n"
             "    arduino-cli core install esp32:esp32@" + ESP32_CORE + "-cn" +
             " --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index_cn.json")
    except subprocess.CalledProcessError as exc:
        fail("arduino-cli 编译失败，退出码 " + str(exc.returncode))


def package(output, version):
    app_path = output / "MotoNav.ino.bin"
    merged_path = output / "MotoNav.ino.merged.bin"
    if not app_path.exists():
        fail("编译产物缺少 " + str(app_path))
    if not merged_path.exists():
        fail("编译产物缺少 " + str(merged_path) + "（需要 arduino-cli 的 --output-dir 合并镜像）")

    app = app_path.read_bytes()
    merged = merged_path.read_bytes()
    if not app or app[0] != 0xE9:
        fail("应用镜像不是有效的 ESP32 镜像")
    if merged[0x10000:0x10000 + len(app)] != app:
        fail("合并镜像中的 app 偏移不是 0x10000")

    dest = PORT_DIR / "dist"
    dest.mkdir(parents=True, exist_ok=True)
    label = "MotoNav_175c_v" + version + "_panel0"
    files = [
        ("app_0x10000", app),
        ("full_0x0", merged[:0x10000 + len(app)]),
    ]
    entries = []
    for suffix, data in files:
        name = label + "_" + suffix + ".bin"
        (dest / name).write_bytes(data)
        entries.append({
            "file": name,
            "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        })

    manifest = {
        "firmware": version,
        "board": BOARD_DESC,
        "upstream": "firmware/boards/175/MotoNav",
        "pinConfig": "ports/175c/pin_config.h",
        "esp32Core": ESP32_CORE,
        "flashSize": FLASH_SIZE,
        "partition": PARTITION,
        "verifiedApplicationOffset": "0x10000",
        "deviceTested": False,
        "warning": "16MB partition layout on 32MB flash; upper 16MB unused. Not tested on hardware.",
        "files": entries,
    }
    manifest_path = dest / (label + "_manifest.json")
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print("\n[产物] " + str(dest))
    for entry in entries:
        print("  " + entry["file"] + "  " + str(entry["bytes"]) + " bytes  " + entry["sha256"])
    print("  " + manifest_path.name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", default=None,
                        help="包含 firmware/ 的源码根目录（默认: 本仓库 firmware/，否则 MotoNav_stopwatch/）")
    parser.add_argument("--cli", default="arduino-cli", help="arduino-cli 可执行文件")
    parser.add_argument("--jobs", type=int, default=4, help="并行编译任务数 (1-32)")
    parser.add_argument("--clean", action="store_true", help="删除 .build 后重新编译")
    args = parser.parse_args()
    if not 1 <= args.jobs <= 32:
        parser.error("--jobs 必须在 1 到 32 之间")

    upstream = Path(args.upstream).resolve() if args.upstream else default_upstream()
    sketch, libraries, build_path, output = prepare(upstream, args.clean)
    version = find_version(sketch)
    print("[info] 上游: " + str(upstream))
    print("[info] 版本: " + version)
    print("[info] FQBN: " + FQBN)
    compile_sketch(sketch, libraries, build_path, output, args.cli, args.jobs)
    package(output, version)


if __name__ == "__main__":
    main()
