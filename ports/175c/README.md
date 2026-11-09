# MotoNav 175c 移植层

把上游 `MotoNav_stopwatch` 的 **1.75 固件**适配到 **微雪 ESP32-S3-Touch-AMOLED-1.75C**。

上游代码保持原样，本目录只做三件事：

1. 提供 1.75C 的 `pin_config.h`；
2. 构建时把 `MotoNav_stopwatch/firmware/boards/175/MotoNav` 复制到 `.build/`，覆盖引脚配置后再编译；
3. 打包出可烧录的完整镜像和应用镜像。

上游来源：`https://github.com/Charlie-healer/MotoNav_stopwatch`，分支 `feat/board-m5stopwatch`，提交 `b3c9929`（2026-11-03）。

## 和 1.75 的硬件差异

依据微雪官方 1.75C 的 [`pin_config.h`](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/blob/main/examples/arduino/libraries/board_config/pin_config.h)：

| 信号 | 1.75C | 上游 1.75 | 本目录 |
|---|---|---|---|
| LCD SDIO0–3 | 4/5/6/7 | 4/5/6/7 | 不变 |
| LCD SCLK / CS | 38 / 12 | 38 / 12 | 不变 |
| **LCD RESET** | **1** | **39** | 已改 |
| I2C SDA / SCL | 15 / 14 | 15 / 14 | 不变 |
| 触摸 INT | 11 | 11 | 不变 |
| **触摸 RESET** | **2** | **40** | 已改 |
| microSD | 无 | GPIO 1/2/3/41 | 删除 SDMMC 定义 |
| Flash | 32 MB | 16 MB | FQBN 用 32M，分区仍为 16MB 布局 |
| 电源 / IMU / 触摸 | AXP2101 / QMI8658 / CST9217 | 相同 | 不变 |

官方 `01_HelloWorld` 示例里 CO5300 的窗口偏移也是 `6`，与上游一致；触摸地址上游用 `0x5A`，需要上机确认。

## 前置环境

需要 Arduino CLI 和 ESP32 Arduino Core **3.3.10**（上游固件就是用它构建的）。

macOS：

```bash
brew install arduino-cli
arduino-cli core update-index \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.10 \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Linux / Windows 的 arduino-cli 安装方式见官方文档，核心安装命令相同。也可以用 `--cli` 指定 arduino-cli 的绝对路径。

如果从 GitHub 下载工具链时超时（国内网络常见），改用官方中国镜像索引安装同版本的 CN 变体：

```bash
arduino-cli core install esp32:esp32@3.3.10-cn \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index_cn.json
```

`3.3.10-cn` 的核心代码与 `3.3.10` 相同，只是工具链地址指向 `dl.espressif.cn`，FQBN 不变。

## 构建

```bash
cd ports/175c
./build.sh
```

或：

```bash
python3 build.py --jobs 8
```

Windows：

```powershell
.\build.ps1 -Jobs 8
```

常用参数：

- `--clean`：删除 `.build/` 后重新编译；
- `--jobs N`：并行任务数，默认 4；
- `--upstream PATH`：指定另一份 `MotoNav_stopwatch`；
- `--cli PATH`：指定 arduino-cli。

构建不会修改 `MotoNav_stopwatch/`，也不会烧录设备。

## overlay 与已移植功能

`overlay/` 镜像 `MotoNav` 的目录结构，构建时在复制上游 175 源码后覆盖。当前包含：

- `DestHistory.h`：目的地状态（最多 8 项）；
- `DestDrawer.h`：首页星标、目的地抽屉、`#DEST` 解析、`T,DEST_Q` / `T,DEST_GO`；
- `SettingsOverlay.h`：圆屏设置页（亮度/旋转/主题/睡眠）；
- `LeanMeter.h`：倾角/G 仪表（QMI8658 0x6B）；
- `IdlePowerOff.h`：可配置空闲关机时间；
- `MusicCover.h`：专辑封面接收（type 6 / 180×180 / RLE+Deflate）；
- `HomeClock.h`：`#TIME` 同步并写入系统 RTC；
- `RtcClock.h`：开机从 ESP32 内部 RTC 恢复离线时钟；
- `src/protocol/BulkReceiver.h`：新增 type 6 封面通道；
- `src/core/DeviceContext.h`：`IMG_SLOTS` 扩到 17，新增目的地名称图片槽；
- `src/protocol/TextBitmapReceiver.h`：13–16 号图片槽收齐后触发抽屉重绘；
- `src/pages/HomePage.h`：绘制星标/齿轮/抽屉/设置，面板打开时暂停骑手动画；
- `src/pages/IncomingCall.h`：首页轻点转给 `settingsTap()` / `destTap()`，触摸统一走 `touchVectorToCanvas()`；
- `src/protocol/CommandReceiver.h`：`#DEST` 命令入口。

上游 `MotoNav_stopwatch/firmware` 仍然不改，新增功能通过 overlay 叠加。

## 构建验证

2026-11-08 在本机实际编译通过（macOS arm64、Arduino CLI 1.5.1、ESP32 Core 3.3.10-cn）：

- Sketch：1,303,631 bytes（41%），上限 3,145,728 bytes
- 全局变量：92,588 bytes（28%）
- app 镜像：1,303,888 bytes
- full 镜像：1,369,424 bytes
- 镜像头 Flash size code = 5（32MB），chip id = 0x9（ESP32-S3）
- 分区表：nvs / otadata / app0 / app1 / ffat / coredump，确认为 `app3M_fat9M_16MB`

已烧录 app 分区到实机，BLE 广播 `MotoNav-D111`，手机 App 后台 BLE 连接正常。已确认抽屉、触摸、中文名称图片和滑动方向；点选后 App 打开路线规划页。

## 实机记录

2026-11-08，对一台 ESP32-S3-Touch-AMOLED-1.75C（USB MAC `c4:9e:7e:0b:d1:10`，32 MB Flash，8 MB PSRAM）烧录 `full_0x0.bin`：

- esptool 写入完成并回读校验通过；
- 第一次误加了 `--flash_mode qio`，导致 `TG0WDT_SYS_RST` / `ets_loader.c 79` 重启循环；改回镜像头的 DIO 后消失；
- 串口连续 10 秒无复位日志；
- BLE 扫描到设备广播 `MotoNav-D111`，说明应用已启动。

屏幕、触摸、导航数据链路和电源管理仍需逐项确认，因此 manifest 里的 `deviceTested` 保持 `false`。

## 电池诊断

`diag/battery_probe/battery_probe.ino` 可以打印 AXP2101 的状态、电池电压和 `0xA4` 电量百分比，用来区分“电池真的没电”和“电量计读数不对”。

2026-11-08 在 1.75C 上实测：

```
S1=0x38 S2=0x33 present=1 vbusGood=1 chgState=1 battMv=4207 pct=20..27
```

- `battMv=4207`：4.207 V，4.2 V 锂电池的满电电压；
- `S2=0x33`：正在充电，充电状态为 CV（恒压）；
- `pct=0xA4=20..27`：电量计百分比明显偏低，与电压不一致。

结论：电池和充电通路本身正常，屏幕上的 7% 是 AXP2101 电量计的 SOC 估算未校准/未学习，不是电池真实电量。MotoNav 读取的 `0x18`/`0x30`/`0x68`/`0xA4` 与 XPowersLib、微雪官方示例一致，不是移植层的寄存器错误。可先检查电池连接，再做一次完整充放电让电量计学习；如果只想让显示稳定，可以在移植层改成按电压估算百分比。

## 产物

输出在 `ports/175c/dist/`：

| 文件 | 写入地址 | 用途 |
|---|---|---|
| `MotoNav_175c_v<版本>_panel0_full_0x0.bin` | `0x0` | 完整镜像，首次刷写；会覆盖原有设置和数据 |
| `MotoNav_175c_v<版本>_panel0_app_0x10000.bin` | `0x10000` | 仅应用，分区一致时保留 NVS/配对信息 |
| `..._manifest.json` | — | 版本、SHA256、分区和“未上机验证”标记 |

烧录示例（先确认串口，macOS 一般是 `/dev/tty.usbmodem*`）：

```bash
python3 -m esptool --chip esp32s3 -p /dev/tty.usbmodemXXXX -b 921600 \
  write_flash 0x0 dist/MotoNav_175c_v7.12.10-pages.4_panel0_full_0x0.bin
```

只升级应用：

```bash
python3 -m esptool --chip esp32s3 -p /dev/tty.usbmodemXXXX -b 921600 \
  write_flash 0x10000 dist/MotoNav_175c_v7.12.10-pages.4_panel0_app_0x10000.bin
```

没有 esptool 时可用 `python3 -m pip install --user esptool`，或用 Arduino IDE 的“烧录”功能选择生成的 bin。**完整镜像会擦除设置，刷之前先备份。**

> 不要额外加 `--flash_mode qio`。本镜像的 bootloader 头是 **DIO / 80 MHz / 32 MB**；强制 QIO 会导致 `TG0WDT_SYS_RST` 启动循环（ROM 反复打印 `ets_loader.c 79`）。直接用上面的 `write_flash` 命令即可，esptool 会读取镜像头里的参数。

## 首次上机验证清单

1. **屏幕**：能点亮、无花屏、466×466 无偏移；重点确认 CO5300 的 `offset_x=6` 是否适合 1.75C。
2. **触摸**：能响应点击/滑动；CST9217 地址按 `0x5A` 处理，不响应时先查这里。
3. **电源**：AXP2101 电量读数、USB 供电、电池充放电是否正常。
4. **无线**：Wi-Fi/BLE 能正常初始化，手机 App 能扫描并连接。
5. **交互**：屏幕旋转、亮度、待机/关机逻辑。
6. **音频**：1.75C 板载 ES8311 + 扬声器；如果用到放音，确认 I2S 引脚和 PA 46 正常。
7. **稳定性**：连续运行、重启、断连重连各测一轮。

## 已知限制

- **没有在真实 1.75C 上验证过**，manifest 里固定 `deviceTested: false`。
- 1.75C 是 32 MB Flash，但 Arduino Core 3.3.10 对 ESP32-S3 只提供到 16 MB 的分区方案。本移植沿用 `app3M_fat9M_16MB`，**只使用前 16 MB**，后 16 MB 闲置；固件本身也没有使用 ffat/SD。
- 上游 1.75 固件使用 16 MB Flash 构建；本移植只把 FQBN 的 `FlashSize` 改成 32M，分区表仍是 16 MB 布局。
- 上游 1.75 固件不使用 SD，因此删除 `SDMMC_*` 定义不影响编译；1.75C 也没有 SD 卡槽。
- **StopWatch 专属功能不会出现**：伴侣模式、倾角/G 值仪表、双实体键、M5PM1 关机、RX8130 RTC 都只在上游 `boards/stopwatch` 里，且依赖 M5Stack 硬件。
- 官方 1.75C 示例使用 Arduino-ESP32 3.3.11；本移植固定 3.3.10。若编译或运行异常，先对齐核心版本再排查。
- 上游许可为**非商业使用条款**，`AvatarEngine` 为 **AGPL-3.0**。本移植只供个人非商业使用；公开分发需保留上游 `LICENSE.md`、`licenses/LICENSE-AGPL-3.0.txt` 和来源说明。

## 同步上游

`MotoNav_stopwatch/` 是快照，没有 `.git`。更新时重新复制上游工作树即可，本目录唯一覆盖的文件是 `pin_config.h`。如果上游修改了 1.75 的引脚或新增了宏，需要同步更新 `ports/175c/pin_config.h`。
