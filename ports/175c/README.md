# MotoNav 1.75C 移植层

把 MotoNav StopWatch 分支的 **1.75 固件**适配到 **微雪 ESP32-S3-Touch-AMOLED-1.75C**。

现在上游源码已经完整放在仓库根目录：

- 固件基线：`firmware/boards/175/MotoNav`
- 依赖库：`firmware/libraries`
- 本目录：1.75C 引脚、overlay 和构建脚本

构建时把 `firmware/boards/175/MotoNav` 复制到 `ports/175c/.build/`，应用 `pin_config.h` 和 `overlay/`，再调用 arduino-cli。**不修改根目录源码，也不自动烧录。**

## 和 1.75 的硬件差异

依据微雪官方 1.75C 的 `pin_config.h`：

| 信号 | 1.75C | 上游 1.75 | 本目录 |
|---|---|---|---|
| LCD SDIO0–3 | 4/5/6/7 | 4/5/6/7 | 不变 |
| LCD SCLK / CS | 38 / 12 | 38 / 12 | 不变 |
| **LCD RESET** | **1** | **39** | 已改 |
| I2C SDA / SCL | 15 / 14 | 15 / 14 | 不变 |
| 触摸 INT | 11 | 11 | 不变 |
| **触摸 RESET** | **2** | **40** | 已改 |
| microSD | 无 | GPIO 1/2/3/41 | 不使用 |
| Flash | 32 MB | 16 MB | FQBN 32M，分区仍 16MB |
| 电源 / IMU / 触摸 | AXP2101 / QMI8658 / CST9217 | 相同 | 不变 |

1.75C 的 MCLK 是 GPIO16（原版 1.75 为 GPIO42）。当前固件不使用 ES8311 放音，代码暂时保留 42；以后启用音频时应改为 16。

## 构建

需要 Arduino CLI 和 ESP32 Arduino Core **3.3.10**（或 `3.3.10-cn` 镜像版）。

```bash
cd ports/175c
python3 build.py --clean
```

Windows：

```powershell
.\build.ps1 -Jobs 8
```

常用参数：

- `--clean`：删除 `.build/` 后重编；
- `--jobs N`：并行任务数，默认 4；
- `--upstream PATH`：指定另一份含 `firmware/` 的源码根目录；
- `--cli PATH`：指定 arduino-cli。

默认优先使用仓库根目录的 `firmware/`；找不到时才回退到旧的 `MotoNav_stopwatch/` 快照。

## overlay 与功能

`overlay/` 镜像 `MotoNav` 目录结构，构建时在复制 175 源码后覆盖。当前包含：

- `DestHistory.h` / `DestDrawer.h`：目的地状态、星标、抽屉、`#DEST` 协议；
- `SettingsOverlay.h`：亮度、旋转、主题、睡眠、HOME 皮肤；
- `LeanMeter.h`：QMI8658 倾角/G 仪表；
- `MusicCover.h`：专辑封面 type 6（180×180 RLE+Deflate）；
- `HomeClock.h` / `RtcClock.h`：`#TIME` 同步和 ESP32-S3 内部 RTC 跨复位恢复；
- `HomeVariant.h` / `FullStandby.h` / `FullStandbyUi.h`：RIDER / VICE 双主页；
- `ScreenState.h` / `CpuGovern.h` / `PwrKeyScreen.h`：PWR 短按熄屏、80/240MHz 动态调频；
- `src/pages/HomePage.h`：双主页分流、骑手动画、时钟、星标/齿轮；
- `src/pages/IncomingCall.h`：触摸统一映射、PWR/CPU 每圈轮询、熄屏跳过触摸；
- `src/pages/PageNavigation.h`：音乐/天气页、熄屏跳过动画；
- `src/protocol/CommandReceiver.h`：`#DEST`、`#b` 亮度、PWR/CPU 头文件；
- `src/core/DeviceContext.h`：`IMG_SLOTS` 扩到 17；
- `src/protocol/TextBitmapReceiver.h`：13–16 号目的地图片槽；
- `src/protocol/BulkReceiver.h`：封面 type 6。

## 1.75C 特性

- RIDER（骑手）/ VICE（完整待机）双主页，设置页 `HOME` 切换并持久化；
- 目的地抽屉、中文名称图片、音乐封面、点赞/音量；
- 倾角/G、ZERO 校准；
- RTC 离线时钟：`#TIME` 基准存 RTC_NOINIT，重启后按 RTC 计数补时间；
- PWR 短按熄屏/亮屏，长按仍由 AXP2101 硬件关机；
- 熄屏或空闲等待页 CPU 降到 80MHz，触摸/BLE/翻页时回到 240MHz；
- 可配置睡眠关机，电池供电且空闲时生效。

## 烧录

输出在 `ports/175c/dist/`：

| 文件 | 地址 | 用途 |
|---|---|---|
| `*_app_0x10000.bin` | `0x10000` | 只升级应用，保留 NVS/配对 |
| `*_full_0x0.bin` | `0x0` | 首次安装/救砖，覆盖设置 |
| `*_manifest.json` | — | 版本、大小、SHA256 |

只升级应用：

```bash
"$ESPTOOL" --chip esp32s3 -p /dev/cu.usbmodem2101 \
  write_flash 0x10000 dist/MotoNav_175c_v7.12.10-pages.4_panel0_app_0x10000.bin
```

首次完整安装才写 `0x0`。**不要额外加 `--flash_mode qio`**，镜像头是 DIO/80MHz/32MB，强制 QIO 会启动循环。

## 已知限制

- 1.75C 使用 32MB Flash，但 Arduino Core 3.3.10 只有 16MB 分区布局，当前只用前 16MB；后 16MB 闲置。
- `pin_config.h` 的 MCLK 仍为 42；未启用音频，暂无影响。
- PWR/CPU 功能来自 MotoNav-main 的 1.75C 参考实现，已编译；设备断开期间未能完成最终实机按键复测。
- 本工程不包含 Android/iOS 签名文件、高德 Key、和风 Key。
- 上游代码为非商业使用许可，`AvatarEngine` 为 AGPL-3.0；公开分发需保留 `LICENSE.md`、`licenses/` 和来源说明。
