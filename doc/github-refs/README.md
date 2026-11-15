# GitHub 参考仓库快照

2026-11-11 首次抓取，2026-11-13 与 2026-11-14 两次增量。全部为**浅克隆**（`--depth 1 --no-single-branch`），只保留抓取当天的分支 tip，没有历史提交；目的是离线参考、防止上游删库或 404。

- 本地路径：`doc/github-refs/`
- 总量：**84 个仓库、约 5.2 GB**（含各仓库 `.git`）
- 2026-11-13 新增 16 个，清单见 §五；功能矩阵见 `doc/10-GitHub同类项目与StopWatch生态扩展调研.md`
- 2026-11-14 新增 42 个，清单见 §六；功能矩阵见 `doc/11-GitHub扩展调研-第二轮.md`
- 本目录已加入 `.gitignore`，不会误提交进主仓库；需要异地备份请单独打包
- 上游 `zyn950929/MotoNav` 已经 404，所以这里同时保留了多个镜像/分叉快照

## 一、MotoNav 生态（当前固件的上游）

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `Koo0527__MotoNav_StopWatch` | [Koo0527/MotoNav_StopWatch](https://github.com/Koo0527/MotoNav_StopWatch) | `e5ba682` 2026-11-04 | 自定义非商业 | 213M | `nongxl/MotoNav_stopwatch` 全分支备份，**本工程固件的直接上游**。默认分支是 `feat/board-m5stopwatch` |
| `Koo0527__MotoNav-143c` | [Koo0527/MotoNav-143c](https://github.com/Koo0527/MotoNav-143c) | `847cd9f` 2026-10-30 | 自定义非商业 | 254M | 1.43C 板型 v7.18.0-143c 成品固件 + iOS 0.15.1 + 读秒 APK（APK 已从 LFS 补全并校验 sha256） |
| `CharlesXu__MotoNav` | [CharlesXu/MotoNav](https://github.com/CharlesXu/MotoNav) | `3b93fac` 2026-10-16 | 自定义非商业 | 138M | 旧快照，App 8.3.19 / 固件 7.8.11，只作历史对照 |
| `loniper__MotoNav` | [loniper/MotoNav](https://github.com/loniper/MotoNav) | `c1ceb8b` 2026-10-29 | 自定义非商业 | 209M | iOS 0.15.1 的来源仓库 |
| `Lubywu__MotoNav` | [Lubywu/MotoNav](https://github.com/Lubywu/MotoNav) | `6a18c8d` 2026-10-27 | 自定义非商业 | 209M | 0.15.1 旧快照 |

### `Koo0527__MotoNav_StopWatch` 分支

| 分支 | commit | 日期 | 内容 |
|---|---|---|---|
| `feat/board-m5stopwatch` | `e5ba682` | 2026-11-04 | 活跃开发分支：任意角度倾角仪、双击缩放、页面 memcpy 平移、时区同步、12 款地图主题 |
| `main` | `3b37e44` | 2026-10-18 | 上游主线镜像 |
| `pr/music-player-redesign` | `f7099d7` | 2026-10-31 | 音乐播放器重构，**尚未合并进本工程** |
| `pr/navigation-stability` | `178f0e3` | 2026-10-31 | 导航稳定性 |
| `pr/ble-transfer-v2` | `a0fff8a` | 2026-10-31 | BLE 传输 v2，**尚未合并** |
| `pr/map-compression-delta` | `db46ba3` | 2026-10-31 | 地图压缩增量传输，**尚未合并** |
| `pr/android-battery-optimization` | `a57019d` | 2026-10-28 | Android 省电与动画 |
| `pr/board-stopwatch` | `3a2a209` | 2026-10-31 | StopWatch 板型移植 |
| `pr/build-script-detect` | `9c3ba9f` | 2026-10-31 | 构建脚本环境检测 |

## 二、Waveshare ESP32-S3-Touch-AMOLED-1.75C 同板型项目

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `waveshareteam__ESP32-S3-Touch-AMOLED-1.75C` | [waveshareteam/…-1.75C](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C) | `4bfdc80` 2026-10-20 | Apache-2.0 | 276M | 官方 ESP-IDF / Arduino 例程、BSP、硬件资料，调 AXP2101 / CO5300 的第一手参考 |
| `mx3353672833-debug__moto-gps-waveshare` | [moto-gps-waveshare](https://github.com/mx3353672833-debug/moto-gps-waveshare) | `2ba1737` 2026-10-29 | 自定义非商业 | 155M | 同板导航，iPhone + Android；**圆屏本地 OSM/Protomaps 地图**、导航网关、共享 C++ 导航核心 |
| `curisama__The-Badge` | [curisama/The-Badge](https://github.com/curisama/The-Badge) | `76172df` 2026-10-05 | MIT | 27M | 同板 1.75C；**AXP2101 未用电源轨 + light sleep，待机 11.9h→19.6h**；BLE 触控板、录音、游戏 |
| `xevrion__roundkit` | [xevrion/roundkit](https://github.com/xevrion/roundkit) | `3e53ffa` 2026-10-11 | MIT | 256K | 电池、倾角、设置持久化、手势的共用库 |
| `socquique__capsule-radar` | [socquique/capsule-radar](https://github.com/socquique/capsule-radar) | `6f51ce1` 2026-11-10 | MIT | 14M | 同板 1.75，ADS-B 飞机雷达，浏览器直接烧录 |
| `socquique__TamaPoke` | [socquique/TamaPoke](https://github.com/socquique/TamaPoke) | `1fff020` 2026-10-27 | MIT | 85M | 同板 1.75，宠物 + 一键网页安装器 |

## 三、M5Stack StopWatch 生态

硬件与 1.75C 接近（466×466 圆 AMOLED、ESP32-S3、双按键、IMU、ES8311）。

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `m5stack__M5StopWatch-UserDemo` | [m5stack/M5StopWatch-UserDemo](https://github.com/m5stack/M5StopWatch-UserDemo) | `81099c7` 2026-10-29 | MIT | 11M | 官方 ESP-IDF 5.5.4 示例 + HAL/BSP + FluidBox |
| `robustonian__M5StopWatch-Flux` | [M5StopWatch-Flux](https://github.com/robustonian/M5StopWatch-Flux) | `9f6e8d1` 2026-07-11 | MIT | 15M | 直接写 framebuffer + dirty-rect + IMU 后台任务，466×466 稳定 33fps |
| `Moskic__M5StopWatch-UserDemo-Arduino` | [M5StopWatch-UserDemo-Arduino](https://github.com/Moskic/M5StopWatch-UserDemo-Arduino) | `ff16c40` 2026-08-09 | MIT | 10M | 官方 Demo 的 Arduino/PlatformIO 重写，**工具链与本工程最接近** |
| `wararyo__M5StopWatch-wararyoLauncher` | [wararyoLauncher](https://github.com/wararyo/M5StopWatch-wararyoLauncher) | `4f50cf6` 2026-10-13 | 未声明 | 7.5M | 熄屏 20mW、抬腕唤醒、4 表盘、秒表/计时/计步；MultiFirm 宿主 |
| `wararyo__M5StopWatch-MultiFirm` | [M5StopWatch-MultiFirm](https://github.com/wararyo/M5StopWatch-MultiFirm) | `165f5c8` 2026-10-13 | MIT | 944K | 多固件共存方案（`ota_1`~`ota_3` 分区） |
| `78__xiaozhi-esp32` | [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) | `a73e9a7` 2026-11-04 | MIT | 1.3M | **稀疏克隆**，只含 `main/boards/m5stack/stopwatch/` 与 `main/boards/common/`；M5PM1 / M5IOE1 / CO5300 板级初始化 |
| `zqqf16__M5StopWatch-QEMU` | [M5StopWatch-QEMU](https://github.com/zqqf16/M5StopWatch-QEMU) | `81f0ae6` 2026-10-23 | MIT | 6.3M | QEMU 仿真 StopWatch，改 UI 不用刷机 |
| `styles01__hermes-watch` | [hermes-watch](https://github.com/styles01/hermes-watch) | `0764589` 2026-10-10 | 自定义 | 8.1M | 语音遥控、程序化头像、状态环 |
| `liptoxli__M5stopwatch-vibecoding` | [M5stopwatch-vibecoding](https://github.com/liptoxli/M5stopwatch-vibecoding) | `28d9cbc` 2026-10-19 | MIT | 319M | Codex 状态屏、BLE 语音控制器、macOS 虚拟麦克风 |
| `Yeq6X__m5stopwatch-ble-mouse` | [m5stopwatch-ble-mouse](https://github.com/Yeq6X/m5stopwatch-ble-mouse) | `951f55d` 2026-09-18 | MIT | 264K | BLE HID 鼠标 / 触控板 |
| `six-ddc__M5StopWatch-Kids` | [M5StopWatch-Kids](https://github.com/six-ddc/M5StopWatch-Kids) | `460a78b` 2026-08-30 | MIT | 53M | 儿童学习固件 |
| `tallboyzt__fmo-stopwatch` | [fmo-stopwatch](https://github.com/tallboyzt/fmo-stopwatch) | `36cdea8` 2026-09-06 | 未声明 | 16M | 业余无线电通联伴侣 |
| `Ocean-lhy__M5Stopwatch-DIY-Badge` | [M5Stopwatch-DIY-Badge](https://github.com/Ocean-lhy/M5Stopwatch-DIY-Badge) | `c462888` 2026-10-26 | 未声明 | 6.0M | 电子徽章制作工具 |

## 四、其他板型（2.06 圆屏，仅思路参考）

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `infinition__waveshare-watch-rs` | [waveshare-watch-rs](https://github.com/infinition/waveshare-watch-rs) | `5eb445b` 2026-08-27 | Apache-2.0 | 860K | Rust `no_std` 智能手表固件 |
| `MarcoRR__S3NTRY` | [MarcoRR/S3NTRY](https://github.com/MarcoRR/S3NTRY) | `a408d99` 2026-05-12 | 未声明 | 3.4M | **只有 README 和截图，未公开源码**；功能清单可参考：BLE 通知镜像、媒体、GB 模拟器 |

## 五、2026-11-13 增量（16 个）

### 5.1 1.75C 同板

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `soBigRice__soRound_os` | [soBigRice/soRound_os](https://github.com/soBigRice/soRound_os) | `1d388b5` 2026-11-04 | 未声明 | 185M | 1.75C 完整系统：26 App、62 表盘、双 OTA、BLE HID/遥控、股票/宠物、i18n |
| `fellow99__ESP32-S3-Touch-AMOLED-1.75C-brookesia` | [fellow99/…-1.75C-brookesia](https://github.com/fellow99/ESP32-S3-Touch-AMOLED-1.75C-brookesia) | `0d3790f` 2026-06-30 | MIT | 41M | 1.75C 类手机 UI：应用启动器、状态栏、手势、2048/Flappy/Snake、AI 聊天 |
| `NovikovVisuals__Pip-KeyPet` | [NovikovVisuals/Pip-KeyPet](https://github.com/NovikovVisuals/Pip-KeyPet) | `37bc39d` 2026-11-06 | MIT | 13M | 1.75C 宠物：45 情绪、15 App、ANCS、音乐、相机快门、游戏、网页刷机 |

### 5.2 同代板型（1.75 / 1.8 / 1.85B）

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `muki01__Smartwatch-ESP32` | [muki01/Smartwatch-ESP32](https://github.com/muki01/Smartwatch-ESP32) | `6d98ad4` 2026-10-01 | MIT | 23M | 1.8 完整手表 OS：抬腕、步数/睡眠、通知、AOD、闹钟、OTA、PC 预览；Arduino+NimBLE+QMI8658+AXP2101 |
| `charliejgallo__ESP32S3_AmoledOS` | [charliejgallo/ESP32S3_AmoledOS](https://github.com/charliejgallo/ESP32S3_AmoledOS) | `a8b2a3cf` 2026-11-10 | MIT | 301M | 1.8 动态 App：20 内置 + 40 个 `.so`、Lua、ANCS、ESP-NOW、离线地图、桌面模拟器 |
| `Pan-wh__esp32-car-dashboard` | [Pan-wh/esp32-car-dashboard](https://github.com/Pan-wh/esp32-car-dashboard) | `eaf16be` 2026-11-04 | 未声明 | 16M | 466×466 六表盘、BLE 遥测、iPhone 导航、照片/动图表盘、烧屏防护、地图窗口协议 |
| `Gnzlt__ESP32-S3-Touch-OBD2` | [Gnzlt/ESP32-S3-Touch-OBD2](https://github.com/Gnzlt/ESP32-S3-Touch-OBD2) | `5496bea` 2026-11-02 | MIT | 8.7M | 1.85B 摩托 OBD2：10 页、ELM327、TPMS、SD 日志、倾角/G、故障码、车型 profile |
| `g00gander__go-marker` | [g00gander/go-marker](https://github.com/g00gander/go-marker) | `ad6dd5f` 2026-11-09 | Apache-2.0 | 3.0M | 1.85B 圆屏导航、TPMS、BLE 协议、桌面模拟器 |
| `deanthecoder__SteedPilot` | [deanthecoder/SteedPilot](https://github.com/deanthecoder/SteedPilot) | `cf52e77` 2026-09-25 | MIT | 17M | iPhone + 1.85B 极简 HUD，导航状态机/路线/骑行信息 |
| `Edge3dSolutions__Blueline-One` | [Edge3dSolutions/Blueline-One](https://github.com/Edge3dSolutions/Blueline-One) | `a8c446c` 2026-08-31 | MIT | 4.3M | 圆屏 OBD/CAN 仪表：四合一、换挡灯、TPMS、MPG、计时器 |

### 5.3 导航 / MotoNav 专版

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `Koo0527__New-MotoNav` | [Koo0527/New-MotoNav](https://github.com/Koo0527/New-MotoNav) | `d83f935` 2026-11-11 | 自定义非商业 | 214M | MotoNav 最新上游：iOS 0.16.0、固件 7.12.10-pages.5、楼块连续性、隧道调光 |
| `Gyro-Seek__MotoNav_Watchcopy` | [Gyro-Seek/MotoNav_Watchcopy](https://github.com/Gyro-Seek/MotoNav_Watchcopy) | `b9d36bf` 2026-11-12 | 自定义非商业 | 213M | StopWatch 专版 MotoNav：毛玻璃/AA 矢量 UI、矢量时钟、抽屉、伴侣模式 |
| `iamsachin__bikepod` | [iamsachin/bikepod](https://github.com/iamsachin/bikepod) | `a7aa4f8` 2026-10-14 | 未声明 | 3.8M | iPhone + 圆屏导航：ANCS/AMS、地图窗口、雨/加油站/减速带、倾角/G、到达摘要 |

### 5.4 M5StopWatch 生态

| 目录 | 仓库 | commit | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|
| `m5stack__M5StopWatch-PowerDemo` | [m5stack/M5StopWatch-PowerDemo](https://github.com/m5stack/M5StopWatch-PowerDemo) | `a68d030` 2026-10-29 | MIT | 11M | 官方功耗实测：L0/L1/L2、IMU 唤醒、RTC 唤醒、Wi-Fi light sleep |
| `Trentct__m5stack-stopwatch-avatar` | [Trentct/m5stack-stopwatch-avatar](https://github.com/Trentct/m5stack-stopwatch-avatar) | `2049632` 2026-08-25 | AGPL-3.0 | 380K | 程序化头像：12 表情、60fps、dirty rect、触摸/倾斜/摇晃 |
| `uezo__M5StarScope` | [uezo/M5StarScope](https://github.com/uezo/M5StarScope) | `b89fb6c` 2026-09-21 | MIT | 8.4M | 星空望远镜：8,921 颗星、行星、IMU 指向、语音讲解 |

## 六、2026-11-14 增量（42 个）

### 6.1 1.75C / 1.75 同板（12 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|---|
| `DEFTileu__amoled-studio` | [DEFTileu/amoled-studio](https://github.com/DEFTileu/amoled-studio) | `a6d81b4` | 2026-11-13 | Apache-2.0 | 1.9M | 1.75C LVGL 模拟器 + ESP32-S3 固件模拟，HTTP 截图/触摸注入/WebSocket 推流 |
| `A0nameless0man__ESP32-S3-AMOLED-1.75-FW` | [A0nameless0man/…-1.75-FW](https://github.com/A0nameless0man/ESP32-S3-AMOLED-1.75-FW) | `ad6d5be` | 2026-09-03 | 未声明 | 61M | Brookesia phone UI；`AGENTS.md` 是同板最完整的 TE/QMI8658/Flash/CASET 实测知识库 |
| `PopaSpinka__HaloWatch` | [PopaSpinka/HaloWatch](https://github.com/PopaSpinka/HaloWatch) | `c925c5b` | 2026-10-31 | MIT | 5.1M | 自写 CO5300 QSPI + TE ISR、按 band 预测呈现时间、AOD/light sleep/弹簧动画 |
| `MikeCheek__CustomOS-ESP32-S3-Amoled` | [MikeCheek/CustomOS](https://github.com/MikeCheek/CustomOS-ESP32-S3-Amoled) | `9f94d53` | 2026-11-12 | 未声明 | 24M | Arduino+GFX 手表 OS：dirty-rect、通知中心、签名 OTA、Flutter 手机端 |
| `shhish__orb` | [shhish/orb](https://github.com/shhish/orb) | `ba6c568` | 2026-11-06 | MIT | 88M | ESP-IDF/LVGL 圆屏桌面 OS：TE band pacing、圆环手势、签名 OTA、语音 PTT |
| `scott987-cmd__ESP32-Round-Clock` | [ESP32-Round-Clock](https://github.com/scott987-cmd/ESP32-Round-Clock) | `c05c15f` | 2026-09-20 | Apache-2.0 | 38M | 中文圆屏 OS：18 App、通知中心、BLE HID、SenseVoice/Whisper 语音 |
| `cnshaohua__MotoNav-1.75C` | [cnshaohua/MotoNav-1.75C](https://github.com/cnshaohua/MotoNav-1.75C) | `1b996f4` | 2026-11-03 | 非商业 | 172M | **仅 README 改动**，固件/App 与上游 tag byte-identical |
| `allinnb__hermes-gadget-sdk` | [hermes-gadget-sdk](https://github.com/allinnb/hermes-gadget-sdk) | `82c079a` | 2026-10-24 | MIT | 82M | 1.75C 板级端口、AXP2101 驱动、实测电池曲线、band 渲染、WebSocket 协议 |
| `elModo7__LibreDeck-ESP32-Waveshare-1.75C-flasher` | [LibreDeck flasher](https://github.com/elModo7/LibreDeck-ESP32-Waveshare-1.75C-flasher) | `a38c294` | 2026-11-09 | 未声明 | 19M | esp-web-tools 网页刷机，merged image 写 0x0 |
| `LjfsGitHub__esp32-pager` | [esp32-pager](https://github.com/LjfsGitHub/esp32-pager) | `a3b011e` | 2026-10-26 | MIT | 387M | 1.75C 寻呼机：6 位配对码、bearer 哈希、L0-L3 风险分级、PTT→Whisper→Agent |
| `PascalAI2024__JarvisNano` | [JarvisNano](https://github.com/PascalAI2024/JarvisNano) | `ff8d2af` | 2026-11-05 | Apache-2.0 | 212M | Gemini Live 语音伴侣；QMI8658 抬腕唤醒、HTTP OTA、双快照 NVS |
| `PartiallyFrozen__tilt-a-tron` | [tilt-a-tron](https://github.com/PartiallyFrozen/tilt-a-tron) | `e859d48` | 2026-09-22 | MIT | 13M | 1.75 掌机：11 游戏、`.tat` 游戏包、Game API、网页安装 |

### 6.2 M5StopWatch 生态（12 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|---|
| `merlinmb__esp32_EX30_Speedometer_M5GPSStopWatch` | [EX30 Speedometer](https://github.com/merlinmb/esp32_EX30_Speedometer_M5GPSStopWatch) | `5351749` | 2026-11-07 | 未声明 | 2.9M | 车速表：5 表盘、速度滤波、太阳高度自动亮度、设置迁移 |
| `sunnysdady__grokwatch` | [grokwatch](https://github.com/sunnysdady/grokwatch) | `07c8aa5` | 2026-10-26 | MIT | 25M | xiaozhi 分支：骑行导航、0x11 扩展协议、双 PSRAM 快照滑动、待机 FSM |
| `wujin941005__stopwatch-hub` | [stopwatch-hub](https://github.com/wujin941005/stopwatch-hub) | `2e611f3` | 2026-10-13 | 未声明 | 2.4M | Mooncake 多 App 宿主：AppAbility 生命周期、资源归属契约、NUS JSON |
| `jacobe603__m5stack-stopwatch-reference` | [stopwatch-reference](https://github.com/jacobe603/m5stack-stopwatch-reference) | `a82cc6f` | 2026-08-16 | MIT | 31M | Arduino 多 App：静态注册表、圆环绝对表盘、振动语义、CSV 日志 |
| `cerary__OBD-HUB-WATCH` | [OBD-HUB-WATCH](https://github.com/cerary/OBD-HUB-WATCH) | `6b700ae` | 2026-10-03 | GPL-3.0 | 118M | 3190 行 ELM327 BLE 客户端、车型 profile、OBD 仪表（只参考） |
| `gpthimble__stopwatch-pedometer` | [stopwatch-pedometer](https://github.com/gpthimble/stopwatch-pedometer) | `4d61e4b` | 2026-10-26 | 未声明 | 7.6M | BMI270 硬件计步、1440 分钟桶、7 天历史、60 s 深睡补记 |
| `Ortizaw027__stp-smartwatch-esp32-idf` | [stp-smartwatch](https://github.com/Ortizaw027/stp-smartwatch-esp32-idf) | `e5b18cb` | 2026-10-27 | MIT | 269M | QMI8658 寄存器驱动、双阈值计步、LVGL tileview、秒表（240×240 demo） |
| `zjyl1994__m5swatch` | [m5swatch](https://github.com/zjyl1994/m5swatch) | `1fa9316` | 2026-11-13 | MIT | 40M | 完整 StopWatch OS：Launcher、流体/骰子、LittleFS 7.9 MB、充电配置 |
| `HomeVox__homevox-desk-clock` | [homevox-desk-clock](https://github.com/HomeVox/homevox-desk-clock) | `acd3d35` | 2026-11-12 | MIT | 55M | 5 语文字钟、指针/数字表盘、Wi-Fi 门户、MQTT/Home Assistant、声音反应 |
| `skfallin__StopWatch` | [StopWatch](https://github.com/skfallin/StopWatch) | `f6bdab6` | 2026-10-30 | MIT | 7.7M | iOS 伴生、Home Assistant、语音笔记、番茄钟、BLE 协议 v1、USB HID |
| `KeijiTokunaga__m5stack-stopwatch-utilities` | [stopwatch-utilities](https://github.com/KeijiTokunaga/m5stack-stopwatch-utilities) | `f668c6d` | 2026-09-21 | 未声明 | 520K | 固件内 Wi-Fi 设置、汇率、水族箱、番茄钟、天气、位置地图 |
| `Ganeesya__live2d-stream-m5stopwatch` | [live2d-stream](https://github.com/Ganeesya/live2d-stream-m5stopwatch) | `aebf674` | 2026-11-08 | MIT | 9.1M | Android 渲染 Live2D，JPEG 经 Wi-Fi 推到手表扬，约 26 fps |

### 6.3 同代板型 / 其他圆屏 OS（9 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|---|
| `Kolo-Naukowe-Informatykow-UR__WatchUR` | [WatchUR](https://github.com/Kolo-Naukowe-Informatykow-UR/WatchUR) | `f6f22d7` | 2026-05-21 | 未声明 | 628K | 2.06 bring-up：AXP2101 ALDO4→I²C 电平转换上电顺序、安全充电默认值 |
| `asmsaifs__S3Wear-Community` | [S3Wear-Community](https://github.com/asmsaifs/S3Wear-Community) | `429fd41` | 2026-10-19 | Apache-2.0 | 10M | 2.06 OS：抬腕、电源 FSM、face.json、5×5 烧屏偏移、BLE 协议 |
| `AmunM9__avocatOS` | [avocatOS](https://github.com/AmunM9/avocatOS) | `a8e5f86` | 2026-09-28 | MIT | 29M | Apple Watch 风格 OS：手势 DSP、ANCS/AMS/CTS、PIN 网页 OTA、蜂窝启动器 |
| `aviletek__wisp32-watch` | [wisp32-watch](https://github.com/aviletek/wisp32-watch) | `55f2e8d` | 2026-10-13 | GPL-3.0 | 1.8M | 单文件 Arduino：Gemini/OpenAI Realtime、约 50 条 Web 路由、i18n 表 |
| `joaquimorg__OLEDS3Watch` | [OLEDS3Watch](https://github.com/joaquimorg/OLEDS3Watch) | `d2c8d4f` | 2026-09-08 | GPL-3.0 | 7.4M | 2.06 + Brookesia：启动器/最近任务 UX、睡眠时序、SPIFFS JSON 设置 |
| `lumosmaximus__lumo-watch` | [lumo-watch](https://github.com/lumosmaximus/lumo-watch) | `a5881fc` | 2026-10-17 | Apache-2.0 | 48M | 1.75 + Zephyr：CST9217 完整协议、CO5300 QSPI 位扩展、8 行 band、60 fps |
| `linquant__jnkwatch` | [jnkwatch](https://github.com/linquant/jnkwatch) | `04dffbb` | 2026-10-21 | Apache-2.0 | 1.8M | 1.43 + Android Compose：完整 GATT 契约、BLE OTA 状态机、抬腕、计步 |
| `cyllas__hud-for-883` | [hud-for-883](https://github.com/cyllas/hud-for-883) | `21b50e9` | 2025-08-02 | MIT | 396K | 1.43 摩托 HUD：Sygic BLE UUID/帧解码；显示驱动与引脚不可用 |
| `hleserg__Attadipa` | [Attadipa](https://github.com/hleserg/Attadipa) | `5b86ee3` | 2026-10-02 | GPL-3.0 | 91M | 穿戴 OS：导航/网格/GNSS/LoRa MeshCore、桌面模拟器、l10n、数据新鲜度 |

### 6.4 摩托 / OBD / 骑行 / 模拟器（9 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 说明 |
|---|---|---|---|---|---|---|
| `ChickenPoptart__KTMComputer` | [KTMComputer](https://github.com/ChickenPoptart/KTMComputer) | `521b44f` | 2026-10-20 | PolyForm 非商业 | 12M | KTM CAN 解码、20 个 CAN ID 文档、行程统计、崩溃安全记录、日志轮转 |
| `cervellopl__EdgeESP` | [EdgeESP](https://github.com/cervellopl/EdgeESP) | `cf422ca` | 2026-08-07 | MIT | 1.2M | 从零 FIT 编码器 + 15 s checkpoint 续记、GPX、完整骑行摘要 |
| `ijlaal1610__AEZEL` | [AEZEL](https://github.com/ijlaal1610/AEZEL) | `3807d2e` | 2026-11-09 | Apache-2.0 | 5.9M | 多车驾驶舱：OBD PID 轮询、倾角/摔车、学习型续航、16 告警位、BLE JSON |
| `gabrielerandazzo__iKawa` | [iKawa](https://github.com/gabrielerandazzo/iKawa) | `79eb904` | 2026-07-28 | MIT | 33M | 川崎 K-line KWP2000、ECU 水温/电压/RPM/档位、ANCS/AMS、Sygic 导航 |
| `SimonKagstrom__radbuzz` | [radbuzz](https://github.com/SimonKagstrom/radbuzz) | `bc83426` | 2026-11-13 | MIT | 26M | VESC CAN、续航模型、三角面积优先轨迹抽稀、Qt 模拟器、Gadgetbridge JSON |
| `lilsteelan__cps3-dashboard` | [cps3-dashboard](https://github.com/lilsteelan/cps3-dashboard) | `3c87806` | 2026-08-21 | 未声明 | 12M | 同板 1.75：ISO-TP UDS、Audi DID、bus-off 恢复、QMI8658 横向 G、0-100 计时 |
| `babykeng__ESP32S3-1.85-ODB` | [ESP32S3-1.85-ODB](https://github.com/babykeng/ESP32S3-1.85-ODB) | `9156fec` | 2026-06-20 | 未声明 | 136M | 1.85B BLE ELM327 中心：PID 解析、自适应 EMA、RPM/车速估档、360° 弧仪表 |
| `Byte-17__MotoNav-Simulator` | [MotoNav-Simulator](https://github.com/Byte-17/MotoNav-Simulator) | `c273687` | 2026-11-12 | 非商业 | 152K | MotoNav 1.85B Windows 模拟器，只有二进制无源码 |
| `hnftxx__MiniMotoNav` | [MiniMotoNav](https://github.com/hnftxx/MiniMotoNav) | `324bf18` | 2026-10-22 | 未声明 | 39M | LVGL 9 摩托仪表/导航：SDL2 共享 UI 模拟器、12 字节 BLE 协议、合成矢量地图 |

## 关键文件索引（移植时直接看这里）

| 想做的事 | 看哪里 |
|---|---|
| 低功耗 / AXP2101 电源轨 / light sleep | `curisama__The-Badge/main/port_esp.c`、`main/launcher.c`、`main/main.c` |
| 圆屏本地地图 / 离线瓦片 | `mx3353672833-debug__moto-gps-waveshare/shared/offline_map/`、`platforms/esp32/main/` |
| 渲染管线 / dirty-rect | `robustonian__M5StopWatch-Flux/main/apps/common/fx/fx.cpp` |
| 多固件分区 / 启动器 | `wararyo__M5StopWatch-wararyoLauncher/src/multifirm/`、`wararyo__M5StopWatch-MultiFirm/` |
| StopWatch 板级初始化 | `78__xiaozhi-esp32/main/boards/m5stack/stopwatch/m5stack_stopwatch.cc` |
| Arduino 下的官方 Demo 结构 | `Moskic__M5StopWatch-UserDemo-Arduino/` |
| MotoNav 未合并功能分支 | `Koo0527__MotoNav_StopWatch`，切 `pr/music-player-redesign` 等分支 |
| 抬腕/步数/睡眠/AOD/通知/OTA | `muki01__Smartwatch-ESP32/Smartwatch/src/algorithms/wrist_gesture.h`、`step_counter.h`、`services/notifications.h`、`ui/system/aod.cpp`、`services/ota.cpp` |
| OBD/ELM327/TPMS/骑行摘要 | `Gnzlt__ESP32-S3-Touch-OBD2/main/obd/`、`main/elm/`、`main/tpms/`、`main/sensors/motion.h` |
| 矢量地图窗口 | `Pan-wh__esp32-car-dashboard/docs/ble.md`、`iamsachin__bikepod/firmware/pod/Map_Layer.h`、`g00gander__go-marker/proto/nav_protocol.h` |
| ANCS / AMS | `charliejgallo__ESP32S3_AmoledOS/components/aos_ble/aos_ancs.c`、`iamsachin__bikepod/firmware/pod/Phone_Notify.cpp` |
| 1.75C 多 App / 启动器 | `soBigRice__soRound_os/GeekTool-IDF/main/launcher.c`、`fellow99__…-brookesia/components/brookesia_phone/`、`NovikovVisuals__Pip-KeyPet/ui/` |
| 毛玻璃 / AA 矢量 UI | `Gyro-Seek__MotoNav_Watchcopy/firmware/boards/stopwatch/MotoNav/UiModern.h` |
| 动态 App / Lua / 模拟器 | `charliejgallo__ESP32S3_AmoledOS/apps/lua/`、`sim/`、`components/aos_ble/` |
| 官方功耗数据 | `m5stack__M5StopWatch-PowerDemo/README.md`、`tests/*/README.md` |
| iOS 0.16 / 楼块连续性 | `Koo0527__New-MotoNav/ios/MotoNav/`、`docs/RELEASE-IOS-0.16.0.md` |
| 程序化头像 / 星空 | `Trentct__m5stack-stopwatch-avatar/`、`uezo__M5StarScope/` |
| 1.75C 硬件真相（TE/QMI8658/Flash/CASET） | `A0nameless0man__ESP32-S3-AMOLED-1.75-FW/AGENTS.md` |
| TE band 排期 / 圆环手势 | `shhish__orb/firmware/components/orb_display/orb_display_geom.c`、`orb_nav/orb_nav.c` |
| dirty-rect / 偶对齐 / 独立 core 发送 | `MikeCheek__CustomOS-ESP32-S3-Amoled/firmware/hal_display.cpp` |
| 自写 QSPI+TE ISR / 按 band 预测呈现 | `PopaSpinka__HaloWatch/libraries/Halo/src/halo_frame.cpp`、`halo_display.cpp` |
| AOD / light sleep / 烧屏 / 抬腕 | `PopaSpinka__HaloWatch/HaloWatch/power.h`、`halo_power.cpp`；`asmsaifs__S3Wear-Community/firmware/components/svc_power/` |
| 手势 DSP / ANCS+AMS | `AmunM9__avocatOS/components/avo_core/src/avo_motion.c`、`avo_board/src/board_apple.c` |
| 通知中心 / 伴生协议 | `MikeCheek__CustomOS-ESP32-S3-Amoled/firmware/app_notifications.cpp`、`COMPANION_PROTOCOL.md`；`linquant__jnkwatch/docs/ble-protocol.md` |
| FIT 编码 / 断点续记 | `cervellopl__EdgeESP/src/ride/FitEncoder.cpp`、`RideCheckpoint.h`、`Recorder.cpp` |
| ISO-TP OBD / 同板 G 值 | `lilsteelan__cps3-dashboard/CAN_Driver.cpp`、`QMI8658.cpp` |
| SDL2 共享 UI 模拟器 | `hnftxx__MiniMotoNav/sim/`；`DEFTileu__amoled-studio/runtime/` |
| CST9217 / CO5300 协议 | `lumosmaximus__lumo-watch/drivers/input/input_cst9217.c`、`drivers/display/display_co5300_qspi.c` |
| `.tat` 游戏包 / 表盘引擎 | `PartiallyFrozen__tilt-a-tron/docs/GAME_API.md`；`asmsaifs__S3Wear-Community/firmware/components/watchfaces/` |

## 更新方式

```bash
cd doc/github-refs
for d in */ ; do
  [ -d "$d/.git" ] || continue
  git -C "$d" fetch --depth 1 origin || echo "拉取失败: $d"
done
# 单个仓库更新到远端当前分支：
git -C Koo0527__MotoNav_StopWatch reset --hard @{u}
```

`78__xiaozhi-esp32` 是部分克隆（`--filter=blob:none`），只保留已 checkout 的板级目录，扩大 sparse 范围时需要联网。

## 许可提醒

- **MotoNav 系**（`CharlesXu`、`loniper`、`Lubywu`、`Koo0527` 多个仓库、`Gyro-Seek`）是自定义「非商业、禁止转售」许可，参考可以，直接合入发布要保留原作者署名并遵守其条款。
- **未声明许可**：`soRound_os`、`Pan-wh/esp32-car-dashboard`、`iamsachin/bikepod`、`moto-gps-waveshare`、`hermes-watch`、`fmo-stopwatch`、`Ocean-lhy/DIY-Badge`。只能参考思路，不要直接复制代码；使用前先联系作者。
- **AGPL-3.0**：`Trentct/m5stack-stopwatch-avatar`。合入/分发需遵守 AGPL；当前 `AvatarEngine` 也已是 AGPL，注意边界。
- **GPL-3.0**：`joaquimorg/OLEDS3Watch`、`cerary/OBD-HUB-WATCH`、`aviletek/wisp32-watch`、`hleserg/Attadipa`；合入会传染，尽量只参考交互。
- **PolyForm 非商业**：`ChickenPoptart/KTMComputer`；商用需授权。
- **未声明（2026-11-14 新增）**：`A0nameless0man`、`MikeCheek/CustomOS`、`merlinmb`、`gpthimble`、`lilsteelan/cps3-dashboard`、`babykeng`、`hnftxx/MiniMotoNav`、`Kolo-Naukowe-Informatykow-UR/WatchUR`、`elModo7/LibreDeck flasher`、`wujin941005/stopwatch-hub`、`KeijiTokunaga/m5stack-stopwatch-utilities`。只能参考思路，不要直接复制代码。
- **注意**：`cnshaohua/MotoNav-1.75C` 与上游同源但为非商业许可；已核对它只有 README 改动，没有新固件功能。
- MIT / Apache-2.0 的项目（muki01、AmoledOS、brookesia、OBD2、Go Marker、SteedPilot、Blueline、Pip、StarScope、PowerDemo、The-Badge、roundkit、capsule-radar、TamaPoke、M5Stack 官方、Moskic、Flux、wararyo、xiaozhi、waveshare 官方）移植最干净，保留版权声明即可。
