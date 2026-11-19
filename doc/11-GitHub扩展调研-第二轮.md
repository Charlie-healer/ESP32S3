# 11-GitHub 扩展调研（第二轮，2026-11-14）

> 2026-11-15 更新：当前代码已经是 8 个玩具（新增 REC/PET），ES7210 用于录音/回放；本文“6 个玩具 / ES7210 未用”的基线已被 [12-五项功能落地记录.md](12-五项功能落地记录.md) 超越。

本轮在 `doc/10` 的 42 个仓库之外，又用 62 组关键词扫了 GitHub，去重后得到 1127 个相关仓库，筛出并浅克隆 **42 个新仓库**。`doc/github-refs` 现在共 **84 个仓库、约 5.2 GB**。

与上一轮相比，这次的重点不是“再找几个表盘/宠物”，而是找四类真正能补工程短板的东西：

1. **1.75C 同板的硬件/时序真相**（TE、CASET、QMI8658 FIFO、32 MB Flash、PSRAM DMA）；
2. **不依赖 LVGL 的渲染与低功耗实现**（dirty-rect、band pacing、AOD、light sleep、抬腕）；
3. **完整的手机伴生协议**（通知中心、ANCS/AMS、设置同步、签名 OTA）；
4. **骑行数据链路**（FIT/GPX、断电续记、OBD/TPMS、行程摘要）。

同时修正了 `doc/10` 的一个过时结论：当前工作区其实已经有 `src/map/VectorMapReceiver.h`、`src/nav/HeadingFusion.h` 和 6 个玩具，`doc/10` 里“矢量地图窗口缺失”的描述不再准确。

---

## 0. 结论

本轮最值得做的五件事，按性价比排序：

| # | 事情 | 最佳参考 | 为什么现在做 |
|---|---|---|---|
| 1 | **TE 同步/band pacing 审计 + dirty-rect** | `A0nameless0man/AGENTS.md`、`shhish/orb`、`MikeCheek/CustomOS`、`PopaSpinka/HaloWatch` | 同板实测数据现成：TE 周期 16.95 ms、40 MHz 全帧 21.7 ms、CASET 必须偶对齐、每 band 各自等 TE 会把全帧拖到 170 ms。这些直接决定地图页流畅度 |
| 2 | **抬腕/落腕 + AOD + light sleep** | `HaloWatch/HaloWatch/power.h`、`S3Wear` `power_fsm.c`/`wf_shift.c`、`avocatOS` `avo_motion.c` | 已有可直接照抄的状态机、亮度、烧屏偏移和唤醒源组合；比 `doc/10` 时只有算法片段更完整 |
| 3 | **通知中心 + 伴生协议** | `CustomOS` `app_notifications.cpp`/`COMPANION_PROTOCOL.md`、`avocatOS` `board_apple.c`、`jnkwatch` `docs/ble-protocol.md` | 从存储、去重、免打扰到 ANCS/AMS、Android GATT 契约都有完整实现；我们只有来电 |
| 4 | **FIT/GPX + 断电续记 + 行程摘要** | `cervellopl/EdgeESP` `FitEncoder.cpp`/`RideCheckpoint.h`、`KTMComputer` `trip_stats.h` | 手机已有 `RideRecorder` 写 GPX，但设备端没有 FIT、没有断点续记、到达摘要字段也不够 |
| 5 | **桌面模拟器（SDL2，共享 UI）** | `hnftxx/MiniMotoNav` `sim/sim_platform.cpp`、`DEFTileu/amoled-studio` | 现在改页面必须刷机；这两个项目证明 466×466/320×240 的 UI 可以在 macOS 上直接跑，还能注入触摸和导出 PNG |

不建议做的事（本轮再次确认）：

- **整机换 ESP-IDF/LVGL/Brookesia/Zephyr**：`A0nameless`、`orb`、`S3Wear`、`lumo-watch` 都证明了可行性，但会把现有 Arduino 单头文件架构、地图管线和手机协议全部推翻。
- **直接抄 GPL 代码**：`cerary/OBD-HUB-WATCH`、`aviletek/wisp32-watch`、`joaquimorg/OLEDS3Watch`、`hleserg/Attadipa` 都是 GPL-3.0；只能看交互和协议字段。
- **追 `cnshaohua/MotoNav-1.75C`**：已核对，它相对上游 tag 只改了 README 两行，固件/App 树 byte-identical，README 里的“BOOT 调亮度”并未实现。
- **M5StopWatch 的驱动**：M5PM1、BMI270、RX8130、马达、A/B 键与 1.75C 全部不同，只能抄 UI/算法/协议。

---

## 1. 搜索方法与新增仓库

### 1.1 本轮查询（62 组）

```text
esp32-s3 amoled watch / esp32s3 smartwatch / esp32 smartwatch lvgl
esp32 watch face lvgl / esp32-s3 round display / esp32 round lcd watch
waveshare amoled esp32 / esp32-s3 touch amoled / m5stack stopwatch / m5stopwatch
stopwatch c152 / esp32 motorcycle navigation / esp32 motorcycle hud / esp32 bike computer
esp32 gps speedometer / esp32 obd2 display / esp32 tpms display / esp32 can bus dashboard
esp32-s3 dashboard motorcycle / esp32 wearable os / lilygo t-watch s3
lilygo t-display s3 amoled / cst9217 / co5300 esp32 / sh8601 esp32 / axp2101 watch
qmi8658 watch / esp32-s3 466x466 / esp32 cycling computer / esp32 gps hud
esp32 navigation display ble / esp32 scooter dashboard / esp32 e-bike display
esp32-s3 amoled 1.43 / esp32-s3 amoled 2.06 / m5stack watch firmware / esp32 watch ota
esp32-s3 smart watch arduino / esp32 hud turn-by-turn
1.75C esp32 / esp32-s3-touch-amoled / M5StopWatch in:name / org:m5stack / org:waveshareteam
in:readme 圆屏 esp32 / in:readme 摩托车 导航 esp32 / in:readme 智能手表 esp32 / in:readme 码表 esp32
gadgetbridge esp32 / ancs esp32 / esp32 watch aod / esp32-s3 notification watch
esp32 amoled notification / esp32 watch simulator / esp32 touch amoled 1.75c / waveshare 1.75c
```

筛选标准与上一轮一致：同板优先、同代次之、有源码且能看出完整交互；只有截图/二进制的不克隆，只在表里标注。

### 1.2 1.75C / 1.75 同板（12 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 定位 |
|---|---|---|---|---|---|---|
| `DEFTileu__amoled-studio` | [DEFTileu/amoled-studio](https://github.com/DEFTileu/amoled-studio) | `a6d81b4` | 2026-11-13 | Apache-2.0 | 1.9M | 1.75C 的 LVGL 桌面模拟器 + ESP32-S3 固件模拟器，HTTP 截图/触摸注入/WebSocket 推流 |
| `A0nameless0man__ESP32-S3-AMOLED-1.75-FW` | […-1.75-FW](https://github.com/A0nameless0man/ESP32-S3-AMOLED-1.75-FW) | `ad6d5be` | 2026-09-03 | 未声明 | 61M | Brookesia phone UI；**`AGENTS.md` 是同板最完整的实测知识库**（TE/QMI8658/Flash/CASET） |
| `PopaSpinka__HaloWatch` | [PopaSpinka/HaloWatch](https://github.com/PopaSpinka/HaloWatch) | `c925c5b` | 2026-10-31 | MIT | 5.1M | 自写 CO5300 QSPI 驱动 + TE ISR + 按 band 渲染；AOD/light sleep/弹簧动画 |
| `MikeCheek__CustomOS-ESP32-S3-Amoled` | [CustomOS](https://github.com/MikeCheek/CustomOS-ESP32-S3-Amoled) | `9f94d53` | 2026-11-12 | 未声明 | 24M | Arduino + Arduino_GFX 手表 OS：dirty-rect、通知中心、签名 OTA、Flutter 手机端 |
| `shhish__orb` | [shhish/orb](https://github.com/shhish/orb) | `ba6c568` | 2026-11-06 | MIT | 88M | ESP-IDF/LVGL 圆屏桌面 OS：TE band pacing、圆环手势、签名 OTA、语音 PTT |
| `scott987-cmd__ESP32-Round-Clock` | [ESP32-Round-Clock](https://github.com/scott987-cmd/ESP32-Round-Clock) | `c05c15f` | 2026-09-20 | Apache-2.0 | 38M | 中文圆屏 OS：18 个 App、通知中心、BLE HID 遥控、SenseVoice/Whisper 语音 |
| `cnshaohua__MotoNav-1.75C` | [MotoNav-1.75C](https://github.com/cnshaohua/MotoNav-1.75C) | `1b996f4` | 2026-11-03 | 非商业 | 172M | **仅 README 改动**；固件/App 与 `firmware-v7.12.10-175c-bootkey.1` 相同 |
| `allinnb__hermes-gadget-sdk` | [hermes-gadget-sdk](https://github.com/allinnb/hermes-gadget-sdk) | `82c079a` | 2026-10-24 | MIT | 82M | 1.75C 板级端口、AXP2101 驱动、实测电池曲线、band 渲染、WebSocket 协议 |
| `elModo7__LibreDeck-ESP32-Waveshare-1.75C-flasher` | [LibreDeck flasher](https://github.com/elModo7/LibreDeck-ESP32-Waveshare-1.75C-flasher) | `a38c294` | 2026-11-09 | 未声明 | 19M | esp-web-tools 网页刷机，merged image 写 0x0 |
| `LjfsGitHub__esp32-pager` | [esp32-pager](https://github.com/LjfsGitHub/esp32-pager) | `a3b011e` | 2026-10-26 | MIT | 387M | 1.75C 寻呼机：6 位配对码、bearer 哈希、L0-L3 风险分级、PTT→Whisper→Agent |
| `PascalAI2024__JarvisNano` | [JarvisNano](https://github.com/PascalAI2024/JarvisNano) | `ff8d2af` | 2026-11-05 | Apache-2.0 | 212M | Gemini Live 语音伴侣；QMI8658 抬腕唤醒、HTTP OTA、双快照 NVS |
| `PartiallyFrozen__tilt-a-tron` | [tilt-a-tron](https://github.com/PartiallyFrozen/tilt-a-tron) | `e859d48` | 2026-09-22 | MIT | 13M | 1.75 掌机：11 个游戏、`.tat` 游戏包、Game API、网页安装 |

### 1.3 M5StopWatch 生态（12 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 定位 |
|---|---|---|---|---|---|---|
| `merlinmb__esp32_EX30_Speedometer_M5GPSStopWatch` | [EX30 Speedometer](https://github.com/merlinmb/esp32_EX30_Speedometer_M5GPSStopWatch) | `5351749` | 2026-11-07 | 未声明 | 2.9M | StopWatch 车速表：5 套表盘、速度滤波、太阳高度自动亮度、设置迁移 |
| `sunnysdady__grokwatch` | [grokwatch](https://github.com/sunnysdady/grokwatch) | `07c8aa5` | 2026-10-26 | MIT | 25M | xiaozhi 分支：骑行导航、0x11 扩展协议、双 PSRAM 快照滑动、待机 FSM |
| `wujin941005__stopwatch-hub` | [stopwatch-hub](https://github.com/wujin941005/stopwatch-hub) | `2e611f3` | 2026-10-13 | 未声明 | 2.4M | Mooncake 多 App 宿主：AppAbility 生命周期、资源归属契约、NUS JSON |
| `jacobe603__m5stack-stopwatch-reference` | [stopwatch-reference](https://github.com/jacobe603/m5stack-stopwatch-reference) | `a82cc6f` | 2026-08-16 | MIT | 31M | Arduino 单 sketch 多 App：静态注册表、圆环绝对表盘、振动语义、CSV 日志 |
| `cerary__OBD-HUB-WATCH` | [OBD-HUB-WATCH](https://github.com/cerary/OBD-HUB-WATCH) | `6b700ae` | 2026-10-03 | **GPL-3.0** | 118M | 3190 行 ELM327 BLE 客户端、车型档位/轮胎/油温 profile、OBD 仪表 |
| `gpthimble__stopwatch-pedometer` | [stopwatch-pedometer](https://github.com/gpthimble/stopwatch-pedometer) | `4d61e4b` | 2026-10-26 | 未声明 | 7.6M | BMI270 硬件计步、1440 个分钟桶、7 天历史、60 s 深睡补记 |
| `Ortizaw027__stp-smartwatch-esp32-idf` | [stp-smartwatch](https://github.com/Ortizaw027/stp-smartwatch-esp32-idf) | `e5b18cb` | 2026-10-27 | MIT | 269M | QMI8658 寄存器驱动、双阈值计步、LVGL tileview、秒表（240×240 demo） |
| `zjyl1994__m5swatch` | [m5swatch](https://github.com/zjyl1994/m5swatch) | `1fa9316` | 2026-11-13 | MIT | 40M | 完整 StopWatch OS：Launcher、流体/骰子、LittleFS 7.9 MB、充电配置 |
| `HomeVox__homevox-desk-clock` | [homevox-desk-clock](https://github.com/HomeVox/homevox-desk-clock) | `acd3d35` | 2026-11-12 | MIT | 55M | 5 语文字钟、指针/数字表盘、Wi-Fi 配置门户、MQTT/Home Assistant、声音反应 |
| `skfallin__StopWatch` | [StopWatch](https://github.com/skfallin/StopWatch) | `f6bdab6` | 2026-10-30 | MIT | 7.7M | iOS 伴生 App、Home Assistant、语音笔记、番茄钟、BLE 协议 v1、USB HID |
| `KeijiTokunaga__m5stack-stopwatch-utilities` | [stopwatch-utilities](https://github.com/KeijiTokunaga/m5stack-stopwatch-utilities) | `f668c6d` | 2026-09-21 | 未声明 | 520K | 固件内 Wi-Fi 设置、汇率、水族箱、番茄钟、天气、位置地图、A/B 切换 |
| `Ganeesya__live2d-stream-m5stopwatch` | [live2d-stream](https://github.com/Ganeesya/live2d-stream-m5stopwatch) | `aebf674` | 2026-11-08 | MIT | 9.1M | Android 渲染 Live2D，JPEG 经 Wi-Fi 推到手表，约 26 fps |

### 1.4 同代板型 / 其他圆屏 OS（9 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 定位 |
|---|---|---|---|---|---|---|
| `Kolo-Naukowe-Informatykow-UR__WatchUR` | [WatchUR](https://github.com/Kolo-Naukowe-Informatykow-UR/WatchUR) | `f6f22d7` | 2026-05-21 | 未声明 | 628K | 2.06 板级 bring-up：AXP2101 ALDO4→I²C 电平转换上电顺序、安全充电默认值 |
| `asmsaifs__S3Wear-Community` | [S3Wear-Community](https://github.com/asmsaifs/S3Wear-Community) | `429fd41` | 2026-10-19 | Apache-2.0 | 10M | 2.06 手表 OS：抬腕检测、电源 FSM、声明式 face.json、5×5 烧屏偏移、BLE 协议 |
| `AmunM9__avocatOS` | [avocatOS](https://github.com/AmunM9/avocatOS) | `a8e5f86` | 2026-09-28 | MIT | 29M | 2.06 Apple Watch 风格 OS：手势 DSP、ANCS/AMS/CTS、带 PIN 的网页 OTA、蜂窝启动器 |
| `aviletek__wisp32-watch` | [wisp32-watch](https://github.com/aviletek/wisp32-watch) | `55f2e8d` | 2026-10-13 | **GPL-3.0** | 1.8M | 2.06 单文件 Arduino：Gemini/OpenAI Realtime、约 50 条 Web 路由、i18n 表 |
| `joaquimorg__OLEDS3Watch` | [OLEDS3Watch](https://github.com/joaquimorg/OLEDS3Watch) | `d2c8d4f` | 2026-09-08 | **GPL-3.0** | 7.4M | 2.06 + Brookesia：启动器/最近任务 UX、睡眠时序、SPIFFS JSON 设置 |
| `lumosmaximus__lumo-watch` | [lumo-watch](https://github.com/lumosmaximus/lumo-watch) | `a5881fc` | 2026-10-17 | Apache-2.0 | 48M | 1.75 + Zephyr：**CST9217 完整协议**、CO5300 QSPI 位扩展、8 行 band、60 fps |
| `linquant__jnkwatch` | [jnkwatch](https://github.com/linquant/jnkwatch) | `04dffbb` | 2026-10-21 | Apache-2.0 | 1.8M | 1.43 + Android Compose：**完整 Android GATT 契约**、BLE OTA 状态机、抬腕、计步 |
| `cyllas__hud-for-883` | [hud-for-883](https://github.com/cyllas/hud-for-883) | `21b50e9` | 2025-08-02 | MIT | 396K | 1.43 摩托 HUD：Sygic BLE UUID/帧解码；显示驱动和引脚不可用，只能看协议 |
| `hleserg__Attadipa` | [Attadipa](https://github.com/hleserg/Attadipa) | `5b86ee3` | 2026-10-02 | **GPL-3.0** | 91M | 穿戴 OS 平台：导航/网格/GNSS/LoRa MeshCore、桌面模拟器、l10n、数据新鲜度模型 |

### 1.5 摩托 / OBD / 骑行 / 模拟器（9 个）

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 定位 |
|---|---|---|---|---|---|---|
| `ChickenPoptart__KTMComputer` | [KTMComputer](https://github.com/ChickenPoptart/KTMComputer) | `521b44f` | 2026-10-20 | **PolyForm 非商业** | 12M | KTM 690 CAN 解码、20 个 CAN ID 文档、行程统计、崩溃安全记录、日志轮转 |
| `cervellopl__EdgeESP` | [EdgeESP](https://github.com/cervellopl/EdgeESP) | `cf422ca` | 2026-08-07 | MIT | 1.2M | Garmin Edge 风格码表：**从零 FIT 编码器 + 15 s checkpoint 续记**、GPX、完整摘要 |
| `ijlaal1610__AEZEL` | [AEZEL](https://github.com/ijlaal1610/AEZEL) | `3807d2e` | 2026-11-09 | Apache-2.0 | 5.9M | 多车驾驶舱：OBD PID 轮询、倾角/摔车、学习型续航、16 个告警位、BLE JSON |
| `gabrielerandazzo__iKawa` | [iKawa](https://github.com/gabrielerandazzo/iKawa) | `79eb904` | 2026-07-28 | MIT | 33M | 川崎 K-line KWP2000、ECU 水温/电压/RPM/档位、ANCS/AMS、Sygic 导航 |
| `SimonKagstrom__radbuzz` | [radbuzz](https://github.com/SimonKagstrom/radbuzz) | `bc83426` | 2026-11-13 | MIT | 26M | VESC CAN、续航模型、**三角面积优先的轨迹抽稀**、Qt 模拟器、Gadgetbridge JSON |
| `lilsteelan__cps3-dashboard` | [cps3-dashboard](https://github.com/lilsteelan/cps3-dashboard) | `3c87806` | 2026-08-21 | 未声明 | 12M | **同板 1.75**：ISO-TP UDS、Audi DID 表、总线 bus-off 恢复、QMI8658 横向 G、0-100 计时、466×466 tile |
| `babykeng__ESP32S3-1.85-ODB` | [ESP32S3-1.85-ODB](https://github.com/babykeng/ESP32S3-1.85-ODB) | `9156fec` | 2026-06-20 | 未声明 | 136M | 1.85B BLE ELM327 中心：PID 解析、自适应 EMA、RPM/车速估档、360° 弧仪表 |
| `Byte-17__MotoNav-Simulator` | [MotoNav-Simulator](https://github.com/Byte-17/MotoNav-Simulator) | `c273687` | 2026-11-12 | 非商业 | 152K | MotoNav 1.85B Windows 模拟器，**只有二进制无源码**；证明 host shim 思路可行 |
| `hnftxx__MiniMotoNav` | [MiniMotoNav](https://github.com/hnftxx/MiniMotoNav) | `324bf18` | 2026-10-22 | 未声明 | 39M | LVGL 9 摩托仪表/导航：SDL2 共享 UI 模拟器、12 字节 BLE 协议、合成矢量地图 |

---

## 2. 十个最有价值的技术发现

### 2.1 1.75C 的 TE 语义和时序（必须按实测，不要按直觉）

`A0nameless0man__ESP32-S3-AMOLED-1.75-FW/AGENTS.md` 是同板最值得读的一份文档，关键结论：

- **GPIO13 TE：空闲低，rising = VBlank 起点，falling = VBlank 结束 ≈ 可见扫描（行 0）起点。** 40 MHz/4 线/16 bpp 全帧传输 21.7 ms > Tvdl 13 ms，因此适配器 `periods=2`，实际锚定 **falling**；TE 周期 16.95 ms。
- **TE_SYNC 模式下每个 flush（每个 band）各自等一个 TE 脉冲**，全帧延迟 ≈ band 数 × 16.95 ms；50 行 band ≈ 10 个 band ≈ 170 ms。**加大 band 高度是全帧延迟最有效的杠杆**，不是提高 SPI 时钟。
- `psram_dma_direct=true` 不能删：CO5300 驱动 2.1.0 没有它，每次 flush 会从内部 DMA 堆 bounce，`ESP_ERR_NO_MEM` 必现（这是 IDF 侧，Arduino 侧对应的是 PSRAM 直传/DMA 缓冲配置）。
- **CASET 要求 x 偶对齐**，奇数窗口会让面板卡行（条纹、错位先查窗口奇偶）。
- 帧中切换显示模式/条带数会丢帧，必须放帧末。
- **QMI8658 FIFO 读法**：STATUS(0x16) 单字节判空 → COUNT(0x15-0x16) 两字节 → CTRL9 发 `REQ_FIFO(0x05)` → 从 DATA(0x17) 突发读 → 重写 FIFO_CTRL 退出。**严禁从 0x15 连读 3 字节**，第 3 字节会落到 DATA 口，导致永远判空。
- **Xtensa -O2 下 `float` 表达式 + `(uint8_t)` 截断会产出错误通道值**（板上实测），颜色插值用 Q16 整数：`c_a + ((c_b-c_a)*t16 + 32768)/65536`。
- 32 MB Flash 实测为 **GD25Q256E（JEDEC c8:4019）**，WaveShare 概览页写 16 MB 是错的；本项目分区只用前 16 MB，后 16 MB 仍可规划。

**对我们的直接动作**：先量当前地图页的全帧/换页延迟，确认 `DisplayPipeline`/`RowWorker` 的 band 高度和 TE 等待策略；如果现在是“每 band 等一次 TE”，把 band 加大或改成“提前算好、只等一次”会比继续抠 SPI 频率收益大。CASET 偶对齐和 QMI8658 FIFO 读法可以直接对照现有驱动检查。

### 2.2 不依赖 LVGL 的 band pacing（`shhish/orb`）

`firmware/components/orb_display/orb_display_geom.c` 和 `orb_display_pacing.h` 是纯 C、可单测的调度器：

- 按 TE 网格给每个 band 排期，拒绝 8–40 ms 之外的周期；
- 宁可**跳过过期帧**也不排队，避免越积越慢；
- 圆屏 band 按弦长计算宽度，角落像素根本不画/不发；行按 CO5300 要求偶对齐。

这套逻辑与 UI 框架无关，值得把“按 TE 排期 + 丢旧帧 + 圆屏弦长裁剪”三条抄进 `RowWorker`/`DisplayPipeline`。

### 2.3 dirty-rect 发送器（`MikeCheek/CustomOS`）

`firmware/hal_display.cpp`：

- `send_frame` 从每两行的两端各比较两个像素（按 32 位字），合并行间 gap，矩形按 CO5300 偶对齐扩边（`:219`）；
- 发送放在独立 core 任务（`:345`），有自定义 QSPI fast path；
- `ui.cpp:189 ui_render_offscreen` + 软件 360° 旋转（`:254`），不依赖 PPA。

对我们是“同一块屏、同一类 GFX”的直接参考。注意它的前提是有 PSRAM framebuffer 池；我们现在是行缓冲/分块渲染，不要照搬整帧双缓冲。

### 2.4 按 band 预测呈现时间（`PopaSpinka/HaloWatch`）

`libraries/Halo/src/halo_frame.cpp:298`：只渲染变化的 16 行 band，逐帧决定“追在扫描线前面”还是“跟着扫描线走”，并用**预测的呈现时间**作为动画时钟（`:425`），而不是用渲染开始时间。`halo_display.cpp:357` 是自写 CO5300 QSPI 驱动 + GPIO13 TE ISR（`:249`），面板初始化先 40 MHz 再升频。

这套“以呈现时间为时钟”的做法能消掉换页动画的相位抖动；我们现在的 `QuarterTurnBlit`/换页动画可以对照。

### 2.5 AOD + light sleep 的完整配方（`HaloWatch` + `S3Wear`）

`HaloWatch/HaloWatch/power.h` 是一份可以直接照抄的状态机：

- 15 s 无触摸/无运动 → 0.25 s 淡出 → AOD（亮度 40/255），只画日期+时钟；
- light sleep，每分钟醒一次画一帧；**面板自己保持图像**，所以不需要持续刷新；
- 烧屏：8 个位置在 7 px 圆上轮换（`SX/SY` 表）；`S3Wear` 的 `wf_shift.c` 是 5×5 网格、2 px 步进、48 分钟周期；
- 唤醒源：触摸、BOOT、拿起（运动）、定时；**唤醒的那次触摸不触发点击**（`halo_boot_click()` 吞掉）；
- 音乐播放/ USB 供电时保持不睡（否则 USB 会掉线、无法刷机）；
- `halo_power.cpp:163` 用 `gpio_wakeup_enable` + `esp_light_sleep_start`，醒来后恢复触摸/IMU 的边沿中断。

`S3Wear` 的 `svc_power/power_fsm.c` 多了一层 ACTIVE→DIM→AOD/SLEEP/SAVER/WATCH_ONLY 和低电回退；`watch_only.c` 处理闹钟唤醒。两者结合就是我们缺的 AOD/待机体系。

### 2.6 抬腕/落腕的手势 DSP（`avocatOS` + `jnkwatch` + `JarvisNano`）

- `AmunM9__avocatOS/components/avo_core/src/avo_motion.c`：抬腕（`:336`，单位向量历史 + 1 s 回看）、手腕甩动（`:94`）、双击（`:43`）、自相关计步（`:173`），常数来自真实录音。
- `linquant__jnkwatch/core/wrist.c`：更简单的抬腕 + 20° 滞回，阈值刻意低于计步。
- `PascalAI2024__JarvisNano/main/power.c:112`：QMI8658 INT1（GPIO21）wake-on-motion，触摸 GPIO11 + 定时器；如果 INT1 已经高则拒绝抬腕唤醒（`:134`），避免“拿起就亮”的误触发。

我们已经有 QMI8658 和 `HeadingFusion`，抬腕是纯算法增量，不需要新硬件。

### 2.7 通知中心的完整分层（`CustomOS` + `avocatOS` + `jnkwatch`）

- `CustomOS/firmware/app_notifications.cpp:23`：PSRAM 环形队列 + NVS 持久化、按 App 静音、同 ID 替换、Toast 队列、未读角标。
- `avocatOS/components/avo_board/src/board_apple.c:28-38,156-262`：ANCS + AMS + CTS 走 NimBLE，状态机式请求流水线，完整消息在 PSRAM 组装；纯解析在 `avo_core/src/avo_apple.c`。
- `jnkwatch/docs/ble-protocol.md:1-90`：Android 侧 GATT 契约（Settings/Time/NotifPush/NotifClear/ActionCfg/ActionEvent/Status/Weather/Probe/OtaCtrl+OtaData），MTU、字节序、版本规则都写清楚了。
- `scott987` 的 `notification_center.c:23`：用调用方 key 的 SHA-256 去重，NVS blob 持久化，角标 + 卡片。

我们 Android 已有 `AmapNotifyListener`，扩展成白名单 + `#N` 文本协议即可；iOS 侧参考 `board_apple.c` 或 `bikepod/Phone_Notify.cpp`。

### 2.8 FIT/GPX 与断电续记（`EdgeESP`）

`cervellopl/EdgeESP/src/ride/FitEncoder.cpp` 从零实现 FIT：file_id/record/lap/session 定义，文件头在 finalize 时回填，CRC 流式计算（`:31-46,267-299`）。更关键的是 `RideCheckpoint.h`：

- checkpoint 结构：magic `EDGE`、version、stateSize、FIT data size、路径、`RideState`、末尾 CRC；
- 每 15 s flush + checkpoint（`Recorder.cpp:140-149`）；
- 启动扫描 `/rides/*.rst`，校验后 `FitEncoder::resume` 接着写（`:207-236`）；
- 同时写 GPX（含心率/踏频/温度扩展）。

这套是 MIT、Arduino 风格，最适合我们。设备端没有 SD，可以改成 LittleFS/Flash 分区；也可以只存摘要，FIT/GPX 留在手机。

### 2.9 OBD/TPMS 的正确边界（`cps3` + `AEZEL` + `babykeng` + `cerary`）

- `cps3-dashboard` 是**同板 1.75**（466×466 CO5300 + CST9217 + QMI8658），`CAN_Driver.cpp:409` 有完整 ISO-TP（单帧/首帧/流控/连续帧/`0x78` pending），`QMI8658.cpp:34` 是横向 G 的 EMA/零偏/去皮。许可证未声明，**只能参考**。
- `AEZEL` 的 OBD PID 集（`0x0C/0D/05/11/04`，请求 `0x7DF`，响应 `0x7E8/7EA`，40 ms 轮询）和字段分类（档位 PRND/CVT、16 个告警位）适合做手机侧转发的数据模型。
- `babykeng` 的 `obd_data_cache.c` 有自适应 EMA（大跳变快车道、300 ms 落零、dt 钳制）和 RPM/车速估档。
- `cerary` 的 ELM327 客户端最完整（AT 初始化、协议 1-11 自动探测、7DF/7E0/18DB33F1 寻址），但 **GPL-3.0，不能抄**；可以照着字段重写。

结论仍然是 `doc/10` 的：**1.75C 走手机转发**（Android 连 ELM327，把 RPM/水温/电压/档位/故障码打包进现有 BLE 文本协议），不要在同一颗 NimBLE 上同时做手机外设 + ELM327 主机。

### 2.10 桌面模拟器的两条可行路线

- **共享 UI + SDL2**：`hnftxx__MiniMotoNav` 的 `CMakeLists.txt:44-62` 把 `src/app`、`src/ui` 同时编进模拟器和固件；`sim/sim_platform.cpp:41-76` 用 RGB565 流式纹理、鼠标转触摸、60 FPS、`SDL_RenderReadPixels` 截图；`sim/main.cpp:10-46` 有 `--bench` 和 SPI 时钟/缓冲/抖动模型。macOS 可直接用。
- **LVGL 模拟器 + HTTP 控制**：`DEFTileu/amoled-studio` 的 `runtime/src/http_api.rs` 提供 `/v1/frame` PNG、WebSocket 1–30 fps 推流、`/v1/touch` 注入、bearer token/Origin 校验；`emulation/esp32sim/waveshare_175c.rs:97` 还精确模拟了 CST9217 的坐标镜像和 IRQ 脉冲。

`Byte-17/MotoNav-Simulator` 证明“把生产 UI 编到 host”可行，但只有 Windows 二进制、无源码，不能直接用。

---

## 3. 当前工程基线修正（相对 doc/10）

`doc/10` 写于 2026-11-13，当前工作区在 2026-11-12 之后又落了以下代码，报告和后续计划要按这个基线：

| 能力 | 当前状态 | 证据 |
|---|---|---|
| 矢量地图窗口 | **已实现接收/解析**（不是“缺失”） | `firmware/boards/175c/MotoNav/src/map/VectorMapReceiver.h`（`parseMap`、位置预测）、`BulkReceiver.h` type 1/2/4/5 |
| 航向融合 | 已实现 | `src/nav/HeadingFusion.h` |
| 玩具 | 已有 Bricks/Pinball/Marble/Pop/Water/ADS-B 6 个 | `src/toys/ToyMode.h`、`src/map/PlaneRadar.h` |
| 行程统计 | 已有 `#O`（距离/分钟/最高车速） | `src/protocol/CommandReceiver.h:382` |
| 骑行记录 | 手机端 `RideRecorder` 写 JSONL + GPX | `android/.../RideRecorder.kt` |
| BLE 文本协议 | 35+ 条 `#` 命令，未知命令安全忽略 | `src/protocol/CommandReceiver.h:31-395` |
| 通知 | 只有来电 `#i` | 同上 `:320` |
| 唤醒 | 只有 PWR 短按 | `src/power/PwrKeyScreen.h` |
| AOD | 只有 RIDER/VICE 待机页，不是 1 Hz 常显 | `src/pages/HomePage.h` |

因此 `doc/10` 功能矩阵里“矢量地图窗口 P1”应改为“**已落地，待实机验证与协议收敛**”；本轮新增的重点转向渲染时序、通知、AOD/抬腕、FIT/断点续记和模拟器。

---

## 4. 功能矩阵 v2

工作量：S ≈ 1–2 天，M ≈ 3–5 天，L ≈ 1–2 周，XL ≈ 2 周以上。优先级按“对骑行主线的价值 / 风险”排。

| 功能 | 当前 | 最佳参考 | 工作量 | 优先级 |
|---|---|---|---|---|
| TE/band pacing + dirty-rect 审计 | 有 fast flush，未按 TE 网格排期 | `orb` `orb_display_geom.c`；`CustomOS` `hal_display.cpp`；`HaloWatch` `halo_frame.cpp` | S/M | **P0** |
| 抬腕/落腕/触摸唤醒 | 只有 PWR | `HaloWatch` `power.h`；`avocatOS` `avo_motion.c`；`jnkwatch` `wrist.c`；`JarvisNano` `power.c` | S | **P0** |
| 通用通知中心 | 只有来电 | `CustomOS` `app_notifications.cpp`；`avocatOS` `board_apple.c`；`jnkwatch` `docs/ble-protocol.md` | M | **P0** |
| 设置 schema/迁移 | 分散的 Preferences/命令 | `S3Wear` `settings_schema.h`；`merlinmb` `settings.cpp` | S | **P0** |
| 手机扩展数据（电量/雨/油/减速带） | 天气/限速/电子眼 | `bikepod` 的 `PHONEBAT/RAIN/FUELNEXT/BUMP` | S | **P0** |
| AOD + 烧屏防护 | RIDER/VICE 待机页 | `HaloWatch` `power.h`；`S3Wear` `wf_shift.c`；`CustomOS` `aod.cpp` | M | P1 |
| 闹钟/计时/秒表 | 无 | `muki01` services；`Ortizaw` `stopwatch.c`；`skfallin` 计时/番茄钟 | M | P1 |
| FIT/GPX + 断点续记 | 手机写 GPX；设备无 FIT | `EdgeESP` `FitEncoder.cpp`/`RideCheckpoint.h` | M | **P1** |
| 骑行摘要增强 | `#O` 只有 3 个字段 | `KTMComputer` `trip_stats.h`；`EdgeESP` `RideComputer.h`；`radbuzz` 轨迹抽稀 | S/M | **P1** |
| OBD-II 车辆遥测 | 无 | `cps3` ISO-TP；`AEZEL` PID/字段；`babykeng` EMA/估档；`Gnzlt`（已有快照） | L | P1 |
| TPMS | 无 | `Gnzlt` `tpms_*`（已有快照）；`go-marker` | M/L | P1 |
| 桌面模拟器 | 无 | `MiniMotoNav` `sim/`；`amoled-studio`；`Attadipa` `sim/` | M | **P1** |
| 签名 OTA / 网页刷机 | USB 烧录 | `CustomOS` `hal_fwupdate.cpp`；`orb` `orb_ota`；`avocatOS` `board_portal.c`；`LibreDeck` manifest | L | P1 |
| 应用启动器 + `.tat` 包 | 页面式 | `tilt-a-tron` `.tat`/Game API；`jacobe603` `launcher/apps.h`；`CustomOS` `app_menu.cpp` | L | P2 |
| 表盘引擎（face.json） | 无 | `S3Wear` `wf_decl.h`/`wf_json.c`/`wf_engine.c` | M | P2 |
| 语音助手/PTT | ES7210 未用 | `JarvisNano`；`esp32-pager`；`scott987` `voice_input.c` | XL | P2 |
| 步数/睡眠 | 无 | `gpthimble` 分钟桶/深睡补记；`muki01` 算法；`Ortizaw` 双阈值 | M | P2 |
| i18n | 中文图片 + 英文界面 | `Attadipa` `l10n/`；`wisp32` 表（GPL，仅思路） | S/M | P2 |
| 游戏扩展 | 6 个玩具 | `tilt-a-tron` 11 游戏；`brookesia` 2048/Flappy/Snake | S/M | P2 |
| 多固件共存 | 无 | `wararyo/MultiFirm`（已有快照） | L | P2 |

---

## 5. 推荐批次

### 批次 0（半天，只测量不改功能）

1. 用当前固件量地图页/换页动画的**全帧延迟、band 数、TE 等待次数**；对照 `AGENTS.md` 的 16.95 ms/band 模型，确认瓶颈是 SPI、TE 还是 UI 布局。
2. 检查 CASET/RASET 是否偶对齐；检查 QMI8658 是否从 0x15 连读 3 字节；检查 PSRAM→DMA 是否直传。
3. 确认 32 MB Flash 型号与分区使用范围（当前前 16 MB）。

验收：拿到一份当前帧率/延迟基线，后续改动可对比；不引入任何行为变化。

### 批次 A（1–2 天，低风险，最像“手表”）

1. **抬腕/落腕/触摸唤醒**：`HaloWatch power.h` + `avocatOS avo_motion.c`；与 `PwrKeyScreen.h` 的熄屏状态机合并，避免两套状态打架；亮屏后手臂放下 600 ms 息屏。
2. **AOD**：15 s 空闲 → 0.25 s 淡出 → 亮度 40/255；每分钟一帧；5×5/2 px/48 min 烧屏偏移（`wf_shift.c`）；面板保持图像，不进 light sleep 也能先做“假 AOD”。
3. **设置 schema**：把亮度、页面顺序、皮肤、AOD、抬腕灵敏度收敛到一个带版本号的 NVS blob，支持迁移和范围钳制（`S3Wear settings_schema.h`、`merlinmb settings.cpp`）。

验收：抬腕 10 次亮 10 次、放下 5 次灭 5 次；AOD 1 小时无残影；旧设置升级不丢。

### 批次 B（3–5 天，通知与数据）

1. **通知中心**：Android 白名单（微信/短信/QQ/邮件）→ `#N,<app>,<title>,<body>`；固件 20 条环形队列 + 横幅 + 列表页 + 免打扰；iOS 参考 `board_apple.c` 做 ANCS/AMS。
2. **手机扩展数据**：`#PB`（手机电量/充电）、`#RAIN`、`#FUEL`、`#BUMP`；字段缺失保留旧值，`-1` 表示无数据。
3. **骑行摘要**：`#RIDE,<km>,<min>,<avg>,<max>,<leanL>,<leanR>,<maxG>`，到达时显示 5 秒摘要页；数据来自 Android `RideRecorder`，iOS 参考 0.16 的 `RideBridge`/`GRecorder`。

验收：20 条通知不丢、不卡导航；摘要页不阻塞到达动画；老客户端不受影响（未知 `#` 忽略）。

### 批次 C（1–2 周，差异化）

1. **FIT/GPX + 断点续记**：照 `EdgeESP` 的 `FitEncoder` + `RideCheckpoint`，设备端写 LittleFS 或手机端写 FIT；15 s checkpoint，启动恢复。
2. **OBD/TPMS 手机转发**：Android 连 ELM327/TPMS，按 §2.9 的数据模型发 `#OBD`/`#TP`；固件新增 `BIKE` 页（RPM 弧、速度、档位、水温、电压、瞬时油耗）和低胎压/高温/高转速告警。
3. **模拟器**：先做 `MiniMotoNav` 式 SDL2 共享 UI（466×466、RGB565、鼠标转触摸、PNG 截图），再加 `--bench`；不追求 1:1 时序，只求改 UI 不刷机。

验收：断电后 FIT 可恢复到最后 15 s；OBD 数据不阻塞导航；模拟器能复现 6 个页面和换页动画。

### 批次 D（长期，生态）

- `.tat` 式游戏包（`tilt-a-tron`）比 `.so`/Lua 更适合无 SD 的 1.75C：包放 Flash 资源分区，启动器按 manifest 加载，失败回滚。
- 表盘引擎（`S3Wear face.json`）+ AOD 变体，替换 RIDER/VICE 的硬编码。
- 语音：先做 PTT 语音笔记（ES7210 + ES8311），再接云端；`JarvisNano`/`esp32-pager` 的配对和安全模型可复用。
- i18n：`Attadipa l10n/` 的键表 + 字体子集；不要直接抄 GPL 代码。
- 多固件/OTA：先解决 32 MB 分区，再谈双 OTA + 回滚。

---

## 6. 代码索引（按用途）

| 想做的事 | 文件 |
|---|---|
| 1.75C 硬件真相（TE/QMI8658/Flash/CASET） | `doc/github-refs/A0nameless0man__ESP32-S3-AMOLED-1.75-FW/AGENTS.md` |
| TE band 排期（纯 C） | `shhish__orb/firmware/components/orb_display/orb_display_geom.c`、`orb_display_pacing.h` |
| dirty-rect + 偶对齐 + 独立 core 发送 | `MikeCheek__CustomOS-ESP32-S3-Amoled/firmware/hal_display.cpp:219,273,345` |
| 按 band 预测呈现时间 / 自写 QSPI+TE ISR | `PopaSpinka__HaloWatch/libraries/Halo/src/halo_frame.cpp:298,425`、`halo_display.cpp:249,357` |
| AOD/light sleep/烧屏/唤醒 | `PopaSpinka__HaloWatch/HaloWatch/power.h`、`libraries/Halo/src/halo_power.cpp:151-178` |
| 烧屏 5×5 偏移 | `asmsaifs__S3Wear-Community/firmware/components/watchfaces/wf_shift.c` |
| 电源 FSM / watch-only 深睡 | `asmsaifs__S3Wear-Community/firmware/components/svc_power/power_fsm.c`、`watch_only.c`、`bsp_s3w/bsp_sleep.c` |
| 抬腕/甩腕/双击/计步 DSP | `AmunM9__avocatOS/components/avo_core/src/avo_motion.c:43,94,173,336` |
| 抬腕简化版（20° 滞回） | `linquant__jnkwatch/core/wrist.c` |
| QMI8658 wake-on-motion | `PascalAI2024__JarvisNano/main/power.c:112,134` |
| 通知中心（PSRAM+NVS+免打扰） | `MikeCheek__CustomOS-ESP32-S3-Amoled/firmware/app_notifications.cpp:23` |
| ANCS/AMS/CTS | `AmunM9__avocatOS/components/avo_board/src/board_apple.c:28-38,156-262`、`avo_core/src/avo_apple.c` |
| Android GATT 契约 + BLE OTA | `linquant__jnkwatch/docs/ble-protocol.md:1-90`、`firmware-esp32/core/ota_proto.c:18-148` |
| 签名 OTA + 回滚 | `MikeCheek__CustomOS-ESP32-S3-Amoled/firmware/hal_fwupdate.cpp:211,268`；`shhish__orb/firmware/components/orb_ota/README.md`；`AmunM9__avocatOS/components/avo_board/src/board_portal.c:1-15,200-240` |
| 网页刷机 manifest | `elModo7__LibreDeck-ESP32-Waveshare-1.75C-flasher/manifest.json`（merged image 写 0x0） |
| 配对/安全（6 位码、bearer 哈希） | `LjfsGitHub__esp32-pager/bridge/security.py`、`bridge/discovery.py` |
| FIT 编码器 + 断点续记 | `cervellopl__EdgeESP/src/ride/FitEncoder.cpp:31-46,267-299`、`RideCheckpoint.h`、`Recorder.cpp:140-149,207-236` |
| 行程统计/日志轮转 | `ChickenPoptart__KTMComputer/lib/dash_logic/src/trip_stats.h:29-90`、`log_rotation.cpp:41-55` |
| ISO-TP UDS / QMI8658 横向 G（同板） | `lilsteelan__cps3-dashboard/CAN_Driver.cpp:409-470,607-615`、`QMI8658.cpp:34-84` |
| OBD PID/估档/EMA | `babykeng__ESP32S3-1.85-ODB/components/esp32s3_obd/obd_data_cache.c:8-35,59-96,302-320`；`ijlaal1610__AEZEL/src/managers/CanManager.cpp:6-13,56-95` |
| 轨迹抽稀/续航模型 | `SimonKagstrom__radbuzz/src/trip_computer/trip_log.cc:144-190`、`trip_computer.cc:13-60,192-210` |
| SDL2 共享 UI 模拟器 | `hnftxx__MiniMotoNav/CMakeLists.txt:44-62`、`sim/sim_platform.cpp:41-76,143-215`、`sim/main.cpp:10-46` |
| LVGL 模拟器 + HTTP 截图/触摸 | `DEFTileu__amoled-studio/runtime/HTTP_API.md`、`runtime/src/http_api.rs:523,678`、`emulation/esp32sim/waveshare_175c.rs:97` |
| CST9217 完整协议（Zephyr） | `lumosmaximus__lumo-watch/drivers/input/input_cst9217.c:1-60` |
| CO5300 QSPI 位扩展/8 行 band | `lumosmaximus__lumo-watch/drivers/display/display_co5300_qspi.c:1-40`、`apps/watch/remote/src/main.c:1-50` |
| 圆环手势/返回栈 | `shhish__orb/firmware/components/orb_nav/orb_nav.c`、`orb_nav/README.md` |
| 双 PSRAM 快照滑动 | `sunnysdady__grokwatch/firmware/board/gw_slide.h`、`gw_slide.c`；`gw_sbsm.h`、`gw_sbsm.c` |
| 静态 App 注册表/圆环表盘 | `jacobe603__m5stack-stopwatch-reference/launcher/apps.h`、`launcher.ino:49-59,152-232` |
| AppAbility 生命周期/资源契约 | `wujin941005__stopwatch-hub/firmware/app_codex/app_codex.h:18`、`docs/architecture.md` |
| `.tat` 游戏包 / Game API | `PartiallyFrozen__tilt-a-tron/docs/GAME_API.md`、`games/`、`site/` |
| 声明式表盘 face.json | `asmsaifs__S3Wear-Community/firmware/components/watchfaces/wf_decl.h`、`wf_json.c`、`wf_engine.c` |
| 设置 schema/迁移 | `asmsaifs__S3Wear-Community/firmware/sys_core/include/settings_schema.h`；`merlinmb__.../src/settings.cpp:84-140` |
| 速度滤波/太阳高度亮度 | `merlinmb__.../src/speed_filter.cpp:43-69`、`src/brightness.cpp:11-46` |
| 分钟桶计步/深睡补记 | `gpthimble__stopwatch-pedometer/StopWatchPedometer/ped_core.h:36-113,519-580`、`ped_sleep.h:149-280` |
| 字钟/多语言/Wi-Fi 门户 | `HomeVox__homevox-desk-clock/main/`、`README.md` |
| iOS 伴生 + Home Assistant | `skfallin__StopWatch/ios/`、`home_assistant/`、`docs/` |
| 手机渲染→设备显示（JPEG/Wi-Fi） | `Ganeesya__live2d-stream-m5stopwatch/firmware/`、`android/`、`docs/spec.md` |

---

## 7. 许可证与风险

### 7.1 许可分层

| 类型 | 仓库 | 处理 |
|---|---|---|
| **MIT** | `HaloWatch`、`orb`、`jacobe603`、`Ortizaw`、`m5swatch`、`HomeVox`、`skfallin`、`tilt-a-tron`、`Ganeesya`、`avocatOS`、`hud-for-883`、`EdgeESP`、`iKawa`、`radbuzz`、`hermes-gadget-sdk`、`esp32-pager`、`TamaPoke`、`roundkit` 等 | 可移植，保留版权/许可声明 |
| **Apache-2.0** | `amoled-studio`、`ESP32-Round-Clock`、`S3Wear-Community`、`JarvisNano`、`AEZEL`、`lumo-watch`、`jnkwatch`、`Attadipa`（GPL 见下）、`waveshareteam/*` | 可移植，保留 NOTICE/许可 |
| **GPL-3.0（传染）** | `cerary/OBD-HUB-WATCH`、`aviletek/wisp32-watch`、`joaquimorg/OLEDS3Watch`、`hleserg/Attadipa` | **只看交互/协议，不复制代码**；`wisp32` 的 `es8311.c` 单独 Apache-2.0 |
| **PolyForm 非商业** | `ChickenPoptart/KTMComputer` | 只能参考算法，商用需授权 |
| **自定义非商业** | `cnshaohua/MotoNav-1.75C`（同源）、`Byte-17/MotoNav-Simulator` | 与当前工程同源，合入保留署名/条款 |
| **未声明** | `A0nameless`、`CustomOS`（只有 vendored 许可）、`merlinmb`、`gpthimble`、`cps3`、`babykeng`、`MiniMotoNav`、`WatchUR`、`LibreDeck flasher`、`stopwatch-hub`、`Keiji utilities` | **只参考思路，不复制代码**；使用前联系作者 |

`CustomOS` 特别说明：仓库里没有根 LICENSE，只有 vendored 库的许可；它的 dirty-rect/通知/OTA 代码质量很高，但在作者明确授权前只能“照着实现”，不能整段复制。

### 7.2 工程风险

- **1.75C 无 SD、无马达、无双实体键**：所有依赖这些的功能都要做 Flash/触摸/声音替代。
- **32 MB Flash 只用前 16 MB**：OTA/多固件/`.tat` 包之前先改分区表；不要套用 1.85B/StopWatch 的分区。
- **NimBLE 双角色**：手机外设 + ELM327 主机同时跑风险高；优先手机转发。
- **AMOLED 烧屏**：AOD/表盘必须亮度封顶、像素偏移、自动息屏；不要长时间纯白高亮。
- **light sleep**：`doc/09` 已分析 PSRAM/BLE/USB 风险；`HaloWatch` 的配方是“面板保持图像 + 每分钟醒一帧”，先做假 AOD，再评估真 light sleep。
- **`LibreDeck` 式 0x0 merged image**：会覆盖 bootloader/分区表/出厂数据；只能作为“救砖/整机重刷”，不能当默认升级方式。
- **安全**：OBD/TPMS/导航只做展示，不替代原车仪表；骑行中通知默认静默或只显示来电。

---

## 8. 不建议做 / 本轮排除

| 方向 | 原因 |
|---|---|
| 整机换 Brookesia / LVGL / Zephyr / Attadipa OS | 会推翻现有 Arduino 单头文件、地图管线、电源和手机协议；收益不等于重写成本 |
| 动态 `.so` / Lua App | 1.75C 无 SD；`tilt-a-tron` 的 `.tat` 包 + Flash 资源分区是更轻的替代 |
| 直接移植 M5StopWatch 驱动 | M5PM1/BMI270/RX8130/马达/A+B 键全部不同 |
| 追 `cnshaohua/MotoNav-1.75C` | 已核对：只有 README 改动，没有新固件功能 |
| 照抄 `Byte-17/MotoNav-Simulator` | 只有 Windows 二进制、无源码；只能学 host shim 思路 |
| 照抄 `cps3-dashboard` | 同板、技术价值高，但仓库无许可证；只能参考 |
| 以 `LibreDeck` 0x0 镜像作为默认刷机 | 会清空分区表和设置；只适合救砖 |
| 在 1.75C 上做 LoRa/GNSS/录音硬件 | 没有对应硬件/分区；`Attadipa` 的能力是 2.06/T-Watch 的 |

---

## 9. 下一步

如果只做一件事：**批次 0 的 TE/band 测量 + 抬腕/AOD**。前者决定地图页能不能更顺，后者是成本最低、最直接把 1.75C 从“导航屏”变成“手表”的改动。

如果做一套：**批次 A + B**（抬腕/AOD/设置 schema + 通知中心 + 手机扩展数据 + 骑行摘要），预计只增加几 KB 级 Flash 和少量 RAM，不动地图/电源主链路。

如果要差异化：**批次 C 的 FIT/断点续记 + OBD/TPMS 手机转发**。这是摩托车用户愿意为之买单的功能，也比“再加一个游戏”更有价值。

如果要提升开发效率：**先做 SDL2 模拟器**（`MiniMotoNav` 路线），再改 UI；它会在后续每个批次里回本。

如果要生态：**先合 iOS 0.16**（`Koo0527/New-MotoNav`），再做通知/OTA；iOS 0.16 的 26 个新文件和 pages.5 楼块连续性，仍是当前仓库最直接的功能缺口。

---

## 10. 附录：本轮仓库账目

- 扫描：62 组查询，去重 1127 个仓库。
- 新增克隆：42 个（1.75C/1.75 12 个、M5StopWatch 12 个、同代板型 9 个、摩托/OBD/骑行/模拟器 9 个）。
- 本地总量：`doc/github-refs` 84 个仓库、约 5.2 GB（浅克隆，保留抓取日分支 tip）。
- 未克隆但值得留意的 README 级项目：
  - `Liu-zhongxian/zongzhu-bot`（265★）：1.75C Grok 语音终端 + 后端，语音交互/字幕/朗读可参考；
  - `Tsixom0/Tsixom-Watch-Buddy`（64★）：2.06 Claude 桌面伴侣；
  - `moomdate/redline-gauge`（43★）：CYD 赛车仪表主题/设置；
  - `justalittleking/stopwatch-flightradar`：StopWatch ADS-B 极坐标雷达（我们已有类似 PlaneRadar）；
  - `nuwaa/qzss-disaster-watch-stopwatch`：QZSS 灾害信息接收；
  - `yuiseki/m5-stopwatch-earth`：StopWatch 正交地球渲染。

更新本地快照：

```bash
cd doc/github-refs
for d in */ ; do
  [ -d "$d/.git" ] || continue
  git -C "$d" fetch --depth 1 origin || echo "拉取失败: $d"
done
```

新增仓库继续沿用 `owner__repo` 命名，并在 `doc/github-refs/README.md` 的增量表里登记 commit/日期/许可/大小。
