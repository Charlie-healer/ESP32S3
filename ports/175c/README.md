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

1.75C 的 MCLK 已按原理图改为 GPIO16（原版 1.75 为 GPIO42）；当前固件不使用 ES8311 放音。

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

修改 `ports/175c/overlay/` 后，运行以下命令同步顶层 1.75C 板型（否则 `firmware/build.py --board 175c` 仍用旧代码）：

```bash
python3 ports/175c/sync_board.py
```

## overlay 与功能

`overlay/` 镜像 `MotoNav` 目录结构，构建时在复制 175 源码后覆盖。当前包含：

- `DestHistory.h` / `DestDrawer.h`：目的地状态、星标、抽屉、`#DEST` 协议；
- `SettingsOverlay.h`：亮度、旋转、主题、睡眠、HOME、MUSIC 极简、AOD 开关；
- `LeanMeter.h`：QMI8658 倾角/G 仪表、GRAV 重力旋转；
- `MusicCover.h`：专辑封面 type 6（180×180 RLE+Deflate）；
- `HomeClock.h` / `RtcClock.h`：`#TIME` 同步和 ESP32-S3 内部 RTC 跨复位恢复；
- `HomeVariant.h` / `FullStandby.h` / `FullStandbyUi.h`：RIDER / VICE 双主页、毛玻璃/AA 矢量；
- `ScreenState.h` / `CpuGovern.h` / `PwrKeyScreen.h`：PWR 短按熄屏、80/240MHz 动态调频；
- `src/power/WristWake.h` / `src/power/Aod.h`：抬腕/落腕、假 AOD、1 Hz 时钟、烧屏偏移；
- `src/pages/HomePage.h`：双主页分流、骑手动画、时钟、星标/齿轮、通知铃铛；
- `src/pages/IncomingCall.h`：触摸统一映射、PWR/CPU 每圈轮询、熄屏/AOD 跳过触摸；
- `src/pages/PageNavigation.h`：音乐/天气/指南针/通知/BIKE 页、熄屏跳过动画；
- `src/pages/NotifyCenter.h`：`#N|app|title|body`、20 条通知、横幅、第 4 页列表；
- `src/pages/PhoneStatus.h`：`#PB/#RAIN/#FUEL/#BUMP` 叠加、`#RIDE` 摘要；
- `src/pages/BikePage.h`：`#OBD/#TP`、转速/车速/水温/电压/档位/油耗/胎压；
- `src/protocol/CommandReceiver.h`：`#DEST`、`#b` 亮度、`#N/#PB/#RAIN/#FUEL/#BUMP/#RIDE/#OBD/#TP`；
- `src/core/DeviceContext.h`：`IMG_SLOTS` 扩到 21；
- `src/protocol/TextBitmapReceiver.h`：13–20 号目的地图片槽；
- `src/protocol/BulkReceiver.h`：封面 type 6、雷达 type 5。

## 1.75C 特性

- RIDER（骑手）/ VICE（完整待机）双主页，设置页 `HOME` 切换并持久化；
- 8 项目的地抽屉（4 项可视、上下滑滚动）、中文名称图片；
- 音乐封面/暗化背景、左右滑切歌、双击点赞、播放进度、循环模式、音量、极简 MIN/RICH；
- 满屏倾角/G 矢量仪表、MAX G/L/R、ZERO；伴侣模式对倾斜/G 有触控同款跟随；持续快速运动才触发眩晕 2 秒；
- 设置页 SOUND 音量（ES8311 0/25/50/75/100%）、AOD 开关；
- 伴侣模式 AvatarEngine + KK 程序化表情，震动全部替换为 ES8311 声音反馈；
- RTC 离线时钟：`#TIME` 基准存 RTC_NOINIT，重启后按 RTC 计数补时间；
- PWR 短按熄屏/亮屏，长按仍由 AXP2101 硬件关机；抬腕/落腕/触摸也可唤醒；
- 假 AOD：面板 18/255 低亮、1 Hz 时钟、2 px/2 min 烧屏偏移；15 s 空闲自动进入；
- 通知中心：主页横幅/铃铛、第 4 页列表、20 条、清除/翻页；
- 手机扩展数据：`#PB` 电量、`#RAIN` 降雨、`#FUEL` 加油站、`#BUMP` 减速带；到达显示 `#RIDE` 摘要；
- 第 5 页 BIKE：`#OBD` 转速/车速/水温/电压/负荷/油耗、`#TP` 前后胎压；低胎压/高温/低电压红色；
- 地图自动 heading-up；双击拖动缩放；单指长按 450 ms 后上下拖缩放、左右拖手动旋转；手动旋转时右下角 `AUTO` 恢复自动；
- 页面顺序：音乐 / 主页 / 天气 / 指南针 / 通知 / BIKE，6 个圆点；
- 亮屏固定 240MHz、QSPI 80MHz、fast flush 16KB 分块、主循环 1ms，熄屏/AOD 降到 80MHz；
- 可配置睡眠关机，电池供电且空闲时生效；
- 默认亮度 50%（对齐 MotoNav-main 初始 128/255）。

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
  write_flash 0x10000 dist/MotoNav_175c_v7.12.11-175c.1_panel0_app_0x10000.bin
```

首次完整安装才写 `0x0`。**不要额外加 `--flash_mode qio`**，镜像头是 DIO/80MHz/32MB，强制 QIO 会启动循环。

## 已知限制

- 1.75C 使用 32MB Flash，但 Arduino Core 3.3.10 只有 16MB 分区布局，当前只用前 16MB；后 16MB 闲置。
- `pin_config.h` 的 MCLK 已改为 GPIO16；当前未启用音频。
- PWR/CPU 功能来自 MotoNav-main 的 1.75C 参考实现；亮屏 240MHz、熄屏 80MHz，熄屏/亮屏重绘路径已自检，AXP2101 实体 PWR 短按待按键复测。
- 本工程不包含 Android/iOS 签名文件、高德 Key、和风 Key。
- 上游代码为非商业使用许可，`AvatarEngine` 为 AGPL-3.0；公开分发需保留 `LICENSE.md`、`licenses/` 和来源说明。
