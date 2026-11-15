# 10-GitHub 同类项目与 StopWatch 生态扩展调研（2026-11-13）

本轮不是重复 `doc/github-refs/README.md` 里已有的 26 个仓库，而是回答两个问题：

1. 除了已知的 1.75C / M5StopWatch 项目，GitHub 上还有哪些同板、同代、同用途的开源工程？
2. 其中哪些功能值得搬进当前 MotoNav 1.75C，哪些只是看起来热闹、实际不适合？

## 0. 结论

本次新增 **16 个仓库**，`doc/github-refs` 现在共 **42 个仓库、约 3.2 GB**（浅克隆，保留分支 tip）。

对当前工程最有价值的 8 个方向，按性价比排序：

| # | 功能 | 最佳参考 | 为什么值得 |
|---|---|---|---|
| 1 | 抬腕/落腕/触摸唤醒 | `muki01__Smartwatch-ESP32` 的 `wrist_gesture.h` | 纯 C++、无硬件依赖，QMI8658 直接可用；现在只能按 PWR |
| 2 | 通用通知中心 | `muki01` 通知存储 + `AmoledOS` ANCS + `bikepod` ANCS/AMS | 当前只有来电，没有微信/短信/App 通知；Android 侧已有 NotificationListenerService 基础 |
| 3 | 车辆 OBD-II 遥测 | `Gnzlt__ESP32-S3-Touch-OBD2`、`Blueline-One` | 摩托车场景最强差异化：转速/水温/电压/档位/故障码/油耗 |
| 4 | TPMS 胎压 | `OBD2` 的 `tpms_*`、`go-marker`、`Blueline` | 低胎压红色环 + 声音提醒，安全价值高 |
| 5 | 矢量地图窗口 | `Pan-wh__esp32-car-dashboard` 的 `mw/mr/mb/mp/me`、`bikepod` 的 `MAPBEGIN/MAP/...` | 比手机截屏省带宽、可平滑旋转，是地图管线的下一代方案 |
| 6 | AOD / 表盘 + 烧屏防护 | `muki01` 的 `aod.cpp`、`Pan-wh` 的 `photo_rt.cpp`、`wararyoLauncher` | 466×466 AMOLED 适合做常显；现在 RIDER/VICE 还不是真正的 AOD |
| 7 | 骑行摘要/日志 | `OBD2` 的 `ride_summary.c`、`bikepod` 的 `ARRIVED`、`MotoNav-CYD` | 手机已有 `RideRecorder`，圆屏只缺到达后的摘要页 |
| 8 | UI 毛玻璃/AA 矢量 | `Gyro-Seek__MotoNav_Watchcopy` 的 `UiModern.h` | 只改待机/设置/音乐页，不动地图，观感提升明显 |

不建议现在做的：

- **整机换 Brookesia / soRound 架构**：会推翻现有地图、协议、电源和手机 App 适配，收益不等于重写成本。
- **动态 `.so` App / Lua**：1.75C 没有 SD 卡，虽然 32 MB Flash 有 16 MB 闲置，但分区、加载器、ABI 都是新工程。
- **light sleep**：`doc/09` 已分析 PSRAM/BLE/USB 风险，当前熄屏 80 MHz 的方案更稳。
- **LoRa / GNSS / 录音**：1.75C 没有对应硬件或分区，`doc/09` 已说明。
- **双实体键 / 马达相关交互**：1.75C 没有 StopWatch 的 A/B 键和马达，只能做触摸/声音等效。

许可证提醒（详见 §8）：

- MIT / Apache-2.0：可以移植，保留版权声明。
- `soRound_os`、`Pan-wh/esp32-car-dashboard`、`iamsachin/bikepod`：**未声明许可**，只能参考思路，不要直接复制代码。
- MotoNav 系（`New-MotoNav`、`Gyro`）是自定义非商业许可；`Avatar` 是 AGPL-3.0。

---

## 1. 搜索方法与新增仓库

### 1.1 搜索范围

用 GitHub Search API 跑了 30+ 组关键词，重点覆盖：

```text
m5stopwatch / M5StopWatch / stopwatch m5stack
ESP32-S3 AMOLED 1.75 / 1.75C / 1.43 / 1.8 / 2.06
CO5300 esp32 / CST9217 / AXP2101 / QMI8658
esp32s3 smartwatch / esp32 watch lvgl / esp32-s3 smartwatch esp-idf
motorcycle navigation esp32 / esp32 motorcycle hud
OBD-II esp32 display / esp32 motorcycle dashboard
waveshare amoled 1.75 / brookesia esp32
MotoNav / MotoNav in:name / org:waveshareteam AMOLED / org:m5stack stopwatch
```

筛选标准：

- 同板（1.75C）优先；
- 同代（1.75 / 1.8 / 1.85B / 2.06 / M5StopWatch C152）次之；
- 功能密度高、README 能看出完整交互的优先；
- 只有截图、没有源码的仓库不克隆，只在表里标注；
- 许可证不明的仓库只做参考，不进入移植建议的“可直接复制”名单。

### 1.2 本次新增的 16 个仓库

| 目录 | 仓库 | commit | 日期 | 许可 | 大小 | 定位 |
|---|---|---|---|---|---|---|
| `m5stack__M5StopWatch-PowerDemo` | [m5stack/M5StopWatch-PowerDemo](https://github.com/m5stack/M5StopWatch-PowerDemo) | `a68d030` | 2026-10-29 | MIT | 11M | 官方功耗实测：L0/L1/L2、IMU 唤醒、RTC 唤醒、Wi-Fi light sleep |
| `Gnzlt__ESP32-S3-Touch-OBD2` | [Gnzlt/ESP32-S3-Touch-OBD2](https://github.com/Gnzlt/ESP32-S3-Touch-OBD2) | `5496bea` | 2026-11-02 | MIT | 8.7M | 1.85B 摩托 OBD2 仪表：10 页、ELM327、TPMS、SD 日志、倾角/G、故障码 |
| `Pan-wh__esp32-car-dashboard` | [Pan-wh/esp32-car-dashboard](https://github.com/Pan-wh/esp32-car-dashboard) | `eaf16be` | 2026-11-04 | 未声明 | 16M | 466×466 六表盘、BLE 遥测、iPhone 导航、照片/动图表盘、烧屏防护 |
| `iamsachin__bikepod` | [iamsachin/bikepod](https://github.com/iamsachin/bikepod) | `a7aa4f8` | 2026-10-14 | 未声明 | 3.8M | iPhone + 圆屏导航：ANCS/AMS、地图窗口、雨/加油站/减速带、倾角/G |
| `g00gander__go-marker` | [g00gander/go-marker](https://github.com/g00gander/go-marker) | `ad6dd5f` | 2026-11-09 | Apache-2.0 | 3.0M | 1.85B 圆屏导航、TPMS、BLE 协议、桌面模拟器 |
| `deanthecoder__SteedPilot` | [deanthecoder/SteedPilot](https://github.com/deanthecoder/SteedPilot) | `cf52e77` | 2026-09-25 | MIT | 17M | iPhone + 1.85B 极简 HUD，导航状态机/路线/骑行信息 |
| `Edge3dSolutions__Blueline-One` | [Edge3dSolutions/Blueline-One](https://github.com/Edge3dSolutions/Blueline-One) | `a8c446c` | 2026-08-31 | MIT | 4.3M | 圆屏 OBD/CAN 仪表：四合一、换挡灯、TPMS、MPG、计时器 |
| `muki01__Smartwatch-ESP32` | [muki01/Smartwatch-ESP32](https://github.com/muki01/Smartwatch-ESP32) | `6d98ad4` | 2026-10-01 | MIT | 23M | 1.8 完整手表 OS：抬腕、步数/睡眠、通知、AOD、闹钟、OTA、PC 预览 |
| `fellow99__ESP32-S3-Touch-AMOLED-1.75C-brookesia` | [fellow99/…-1.75C-brookesia](https://github.com/fellow99/ESP32-S3-Touch-AMOLED-1.75C-brookesia) | `0d3790f` | 2026-06-30 | MIT | 41M | 1.75C 类手机 UI：应用启动器、状态栏、手势、2048/Flappy/Snake、AI 聊天 |
| `charliejgallo__ESP32S3_AmoledOS` | [charliejgallo/ESP32S3_AmoledOS](https://github.com/charliejgallo/ESP32S3_AmoledOS) | `a8b2a3cf` | 2026-11-10 | MIT | 301M | 1.8 动态 App：20 内置 + 40 个 `.so`、Lua、ANCS、ESP-NOW、离线地图、模拟器 |
| `soBigRice__soRound_os` | [soBigRice/soRound_os](https://github.com/soBigRice/soRound_os) | `1d388b5` | 2026-11-04 | 未声明 | 185M | 1.75C 完整系统：26 个 App、62 款表盘、双 OTA、BLE HID/遥控、股票/宠物 |
| `Koo0527__New-MotoNav` | [Koo0527/New-MotoNav](https://github.com/Koo0527/New-MotoNav) | `d83f935` | 2026-11-11 | 非商业 | 214M | MotoNav 最新上游：iOS 0.16.0、固件 7.12.10-pages.5、楼块连续性 |
| `Gyro-Seek__MotoNav_Watchcopy` | [Gyro-Seek/MotoNav_Watchcopy](https://github.com/Gyro-Seek/MotoNav_Watchcopy) | `b9d36bf` | 2026-11-12 | 非商业 | 213M | StopWatch 专版 MotoNav：毛玻璃/AA 矢量 UI、矢量时钟、抽屉 |
| `NovikovVisuals__Pip-KeyPet` | [NovikovVisuals/Pip-KeyPet](https://github.com/NovikovVisuals/Pip-KeyPet) | `37bc39d` | 2026-11-06 | MIT | 13M | 1.75C 宠物：45 情绪、15 App、ANCS、音乐、相机快门、游戏、网页刷机 |
| `Trentct__m5stack-stopwatch-avatar` | [Trentct/m5stack-stopwatch-avatar](https://github.com/Trentct/m5stack-stopwatch-avatar) | `2049632` | 2026-08-25 | AGPL-3.0 | 380K | StopWatch 程序化头像：12 表情、60fps、dirty rect、手势 |
| `uezo__M5StarScope` | [uezo/M5StarScope](https://github.com/uezo/M5StarScope) | `b89fb6c` | 2026-09-21 | MIT | 8.4M | StopWatch 星空望远镜：8,921 颗星、IMU 指向、语音讲解 |

---

## 2. 硬件谱系：哪些代码能直接抄

| 板型 | 屏幕/触控 | IMU / 电源 / RTC | 音频 | 存储 | 按键/马达 | 与 1.75C 的复用度 |
|---|---|---|---|---|---|---|
| **1.75C（当前）** | 466×466 CO5300 QSPI / CST9217 | QMI8658 / AXP2101 / ESP32 内部 RTC + 32k | ES8311 + ES7210 双麦 | 32 MB Flash，无 SD | PWR/BOOT，无马达 | 基准 |
| **1.75（工程样机）** | 466×466 CO5300 / CST9217 | QMI8658 / AXP2101 / PCF85063 | ES8311 | 16 MB Flash + SD | 以官方为准 | 高：同显示/触控/IMU/PMIC，引脚和 Flash 不同 |
| **1.85B** | 360×360 ST77916 | IMU + RTC + 电量计 | 扬声器 | 16 MB Flash + SD | 以官方为准 | 中：同代 ESP32-S3，屏幕尺寸/驱动不同；OBD2 仓库可直接跑 |
| **1.8** | 368×448 SH8601 QSPI / FT3168 | QMI8658 / AXP2101 / PCF85063 | ES8311 + 麦克风 | 16 MB Flash + SD | 侧键 | **高**：Arduino + NimBLE + XPowersLib + SensorLib，和当前工具链最接近 |
| **2.06** | 410×502 CO5300 QSPI / FT3168 | QMI8658 / AXP2101 / PCF85063 | ES8311 | 32 MB Flash + SD | PWR/BOOT | 中高：同驱动，UI 尺寸不同 |
| **1.43C** | 466×466 CO5300 QSPI / CST820 | 未在官方 README 列出 | ES8311 + ES7210 | 8 MB Flash / 8 MB PSRAM | 以官方为准 | 中：同显示，Flash/触摸不同，不适合直接当 1.75C |
| **M5StopWatch C152** | 466×466 AMOLED / CST820 | BMI270 / M5PM1 / RX8130 | ES8311 | 16 MB Flash / 8 MB PSRAM | A/B 双键 + 马达 | 低-中：屏幕同尺寸，PMIC/IMU/RTC/按键/马达都不同 |

几个直接结论：

- **1.8 的 `muki01/Smartwatch-ESP32` 是移植成本最低的“完整手表功能包”**：同一套 Arduino + NimBLE + QMI8658 + AXP2101 + ES8311，抬腕、步数、睡眠、通知、AOD、闹钟、OTA 都是现成的。
- **1.85B 的 OBD2 仓库是摩托车遥测的最佳起点**：当前仓库本来就有 `firmware/boards/185b` 实验板型，可以先把 OBD2 跑在 1.85B 上，再把逻辑抽到 1.75C。
- **M5StopWatch 的代码不要按驱动抄**：PMIC、IMU、RTC、按键、马达全部不同，只能抄 UI/算法/交互。
- **1.75C 的 32 MB Flash 目前只用了前 16 MB**（`ports/175c/README.md` 已记录），做 OTA/日志/多固件之前要先解决分区。

选板建议（如果考虑再加一块板）：

- **继续用 1.75C**：屏幕最大、现有工程最完整；缺点是没 SD、没马达、没双键。
- **想要完整手表体验**：1.8（368×448、SD、PCF85063、FT3168、AXP2101、QMI8658、ES8311），muki01 的整套服务几乎可以直接跑。
- **想要摩托 OBD/独立 GPS**：1.85B（360×360、SD、RTC、电量计、扬声器），OBD2 仓库就是为它写的。
- **想要更大屏/更好观感**：2.06（410×502、SD、32MB），但 UI 要重新排。
- **想要双键+马达+做工**：M5StopWatch C152，但生态和 1.75C 的驱动差异最大。
- **1.43C 不建议作为 MotoNav 主力**：Flash 只有 8MB、触摸是 CST820，和当前 1.75C 的 32MB/CST9217 不是一条线。

---

## 3. 新增仓库分类与功能清单

### 3.1 1.75C 同板

#### `soBigRice__soRound_os`

- 1.75C 上最完整的“非导航 OS”：径向启动器、锁屏表盘、快捷面板、26 个内置 App、62 款表盘。
- App 覆盖：Wi-Fi、I2C、系统信息、天气、日历、倒计时、秒表、设置、OTA、音频可视化、水平仪、迷宫、流体、骰子、BLE 遥控台（鼠标/演示/音乐）、数字孪生、答案之书、星座、木鱼、方块、生态箱、专注、声音胶囊、股票、小咪宠物。
- 工程亮点：双 OTA 分区 + 启动回滚、Cloudflare R2 分发、React + Three.js 的 Web 校准器、i18n、BLE HID。
- 许可：未声明，**只能参考思路**。

#### `fellow99__ESP32-S3-Touch-AMOLED-1.75C-brookesia`

- 官方 1.75C + ESP-Brookesia Phone：应用启动器、状态栏（电池/Wi-Fi/时钟）、底部导航栏、边缘滑动返回、上滑回主页/最近任务。
- 插件化 App 生命周期（init → run → pause → resume → back → close），Kconfig 开关 Wi-Fi/Audio/AI/Video。
- 内置 2048、Flappy Bird、Snake、AI Chatbot（XiaoZhi/Coze/OpenAI）。
- 许可 MIT；和当前 MotoNav 的页面模型冲突较大，适合借“启动器/手势/状态栏”而不是整机替换。

#### `NovikovVisuals__Pip-KeyPet`

- 1.75C 上的“宠物 + 工具”固件：约 45 种情绪，触摸/摇晃/翻转/吹气/介绍新人都会影响心情。
- 15 个 App：时钟、宠物、Quick QR、噪声计、徽章、计时器、秒表、手电筒、风格、蓝牙、通知、音乐、相机快门、演示遥控、游戏（球、井字棋、8 字、硬币、骰子、迷宫）。
- iPhone ANCS 通知、音乐控制、网页刷机（Web Serial）、浏览器模拟器。
- 许可 MIT；功能组织方式很适合做“MotoNav 的娱乐/工具层”。

#### `waveshareteam__ESP32-S3-Touch-AMOLED-1.75C`

- 官方 BSP/例程：AXP2101、LVGL 9、ESP-Brookesia、Immersive_block 游戏、ES7210 频谱分析。
- 调驱动、看原理图、确认引脚的第一手参考，Apache-2.0。
- `examples/esp-idf/05_Spec_Analyzer` 是 ES7210 双麦的现成用法，做语音/声纹/噪声计可直接参考。

### 3.2 同代板型（1.75 / 1.8 / 1.85B / 2.06）

#### `muki01__Smartwatch-ESP32`（1.8，最推荐）

- 4 表盘 + AOD（烧屏保护）、11 个 App、通知中心、快捷设置、闹钟、计时器、秒表、日历、录音、天气、音乐、录音机、WLED、车控。
- Gadgetbridge 协议：通知、来电、音乐、天气、日历、校时，不需要自己写手机 App。
- 抬腕唤醒/落腕息屏/触摸唤醒、步数（抗抖动）、睡眠检测、7 天历史。
- 代码分层干净：`src/algorithms`、`src/services`、`src/ui`，驱动不碰 LVGL。
- 许可 MIT；**抬腕、步数、睡眠、通知存储、AOD、OTA 都可以按算法/服务层移植**，UI 需要按 Arduino_GFX 重写。

#### `charliejgallo__ESP32S3_AmoledOS`（1.8）

- 16 表盘（9 个从 SD 安装）、20 个内置 App、40 个从 SD 加载的 `.so` App、Lua 解释器、离线街区地图、网页门户、iPhone ANCS、双表 ESP-NOW、桌面模拟器。
- 工程亮点：动态 App 加载、Lua 脚本、同代码桌面模拟器、ANCS。
- 许可 MIT；**ANCS 和“桌面模拟器跑同一套 UI”值得学**，动态 `.so` 对 1.75C（无 SD）暂时不适用。

#### `Pan-wh__esp32-car-dashboard`（1.75 466×466）

- 6 个表盘：模拟钟、数字钟、速度表（驾驶员三态表情）、彩色数字钟、导航卡片、照片/动图表盘。
- BLE 遥测 + iPhone Core Location 导航投屏；照片表盘有完整烧屏防护（亮度封顶、每 2 分钟像素偏移、自动息屏）。
- `docs/ble.md` 的**地图窗口协议**非常接近 MotoNav 需要的下一代方案：局部米坐标、道路等级、建筑轮廓、路线折线、`mw…me` 原子切换、自车偏移 `ox/oy`。
- 许可未声明 + 使用范围限制（不装车、不商业化），**只能参考设计**。

#### `Gnzlt__ESP32-S3-Touch-OBD2`（1.85B，摩托遥测最佳参考）

- 10 个页面：Ride、Engine、Tyres、Performance、Lean & G、Trip、Fuel、Status、Rides、Diagnostics。
- BLE ELM327（Vgate iCar Pro / OBDLink CX / FFF0 / FFE0 / 18F0 等布局），标准 OBD2 PID、故障码、冻结帧。
- 倾角估计不是简单加速度角，而是 `tan(lean) = v * yaw_rate / g`，带 IMU 安装坐标系学习、陀螺零偏学习、抖动重学习；见 `main/sensors/motion.h`。
- TPMS：BLE 胎压传感器、配对、前后轮、报警。
- SD 日志、按车型的 `bike/profiles/*.h`、燃油估算、档位估算、骑行摘要。
- 许可 MIT；**代码结构是本次调研里最适合“抽成模块”的**：`main/obd/`、`main/elm/`、`main/tpms/`、`main/sensors/`、`main/ui/page_*.c`。

#### `g00gander__go-marker`（1.85B）

- 圆屏导航：地图、天气、行程、指南针、时钟、TPMS 低胎压红色脉冲环。
- `proto/nav_protocol.h` 是完整的 BLE 包定义；`ui/nav_areas_rx.c`、`ui/nav_roads_rx.c` 是分块地图接收；`sim/` 是同一套 UI 的桌面模拟器。
- 许可 Apache-2.0；适合参考“手机算路 + 设备只画”的边界和协议。

#### `deanthecoder__SteedPilot`（1.85B）

- iPhone 负责算路/GPS/限速/航向，ESP32 只做圆屏 HUD；模式：Navigation、Destination、RideInfo、NoPhone、Calibration。
- `include/SteedPilot/NavState.h` 的转向枚举（含环岛出口角度）、`App` 状态机、iOS `NavigationDecisionCore` 都很干净。
- 许可 MIT；适合参考“极简 HUD”而不是“把手机 UI 搬上圆屏”。

#### `Edge3dSolutions__Blueline-One`（圆屏 AMOLED）

- 四合一表盘（RPM 换挡灯弧、速度、油门、进气温度）+ RPM/速度/油门/IAT/水温/电压/TPMS/MAF/MPG/计时器。
- BLE OBD-II、触摸滑动、长按、MPH/KPH、摄氏/华氏、换挡灯起点、亮度、Preferences 保存。
- 许可 MIT；单文件 `Blueline_One.ino`，适合快速看“OBD 仪表 UI 怎么排”。

### 3.3 导航 / 摩托车项目

> 本节前三个仓库（`New-MotoNav`、`Gyro`、`bikepod`）已克隆；后面的 `MotoNav-CYD`、`esp32-oled-maps-hud`、`tripper` 只做了 README 级调研，**没有克隆**，仍列在这里是因为它们的字段/交互值得参考。

#### `iamsachin__bikepod`（2.8C，架构最像 MotoNav）

- iPhone App 负责搜索/算路/Ferrostar 导航/GPS/地图瓦片；Pod 只接收短文本行并绘制。
- 协议：`L|400|MG Rd`、`O|250|4|271|Hosur Rd`、`TRIP|18:42|6.0|7.3|1440`、`MAPBEGIN/MAP/MAPEND`、`POSE`、`SPD`、`LIMIT`、`NIGHT`、`THEN`、`PHONEBAT`、`FUELNEXT`、`RAIN`、`BUMP`、`ARRIVED`、`CRUISE`。
- 功能：转向箭头/环岛出口角度、Then 提示、实时街道图、行程页、倾角/G、来电（ANCS）、音乐（AMS + 封面）、到达摘要、夜间自动调光、断链恢复。
- 许可未声明，**只能参考协议和交互**；它的 `Phone_Notify.cpp`（ANCS/AMS）是本次最完整的 iOS 通知实现。

#### `Koo0527__New-MotoNav`（当前 MotoNav 的最新上游）

- iOS 0.16.0 Build12/Build15：四栏目、途经点/加油站/收藏/历史、自行车官方骑行引擎、统一路线规划、隧道调光、12 种地图风格、楼块连续性。
- 固件 7.12.10-pages.5：固定页面顺序、主页大号 24 小时时钟、旋转记忆、音乐/天气、空闲关机、蓝牙启动任务栈修正。
- 相比当前仓库的 iOS 0.15.1，新增了 26 个 Swift 文件：`NavigationCoordinator`、`RouteWaypointEditor`、`TunnelDimming`、`RideBridge`、`GRecorder`、`CoverLookup/Encoder`、`BleIdlePolicy`、`MotorPlanHistory`、`MotorReroutePolicy`、`ScreenSettings` 等。
- 许可：MotoNav 自定义非商业；当前工程同源，合入时保留署名和条款。

#### `Gyro-Seek__MotoNav_Watchcopy`（M5StopWatch 专版）

- 基于 MotoNav 的 StopWatch 适配：毛玻璃圆角/圆形底座、AA 矢量数字/字母、矢量时钟、R 星抽屉、音乐页、双按键映射、倾角/G、伴侣模式。
- `firmware/boards/stopwatch/MotoNav/UiModern.h` 是本次最直接的 UI 升级参考：`drawTranslucentRoundRect()`、`drawTranslucentCircle()`、5×7 AA 字模。
- 许可：MotoNav 非商业；1.75C 可以直接借 UI 绘制函数，不涉及 StopWatch 的 PMIC/IMU 驱动。

#### `markseder/MotoNav-CYD`

- CYD 上的 GPS 摩托车速度表/行程电脑/GPX 记录器：GNSS 诊断、20 条历史、速度尖峰过滤、断电后 GPX 修复、日夜主题。
- 许可 MIT；适合参考“没有手机时”的独立 GPS 形态，以及 GPX/行程数据的字段设计。

#### `retweakeverything/esp32-oled-maps-hud`

- 用 Chronos App 把 Google Maps 导航通知（含 48×48 原始转向图标）通过 BLE 发到 ESP32 OLED。
- 能显示任意 Google Maps 机动图标：轻微/急转、掉头、环岛第 N 出口、车道指引、匝道/合流/渡轮。
- 许可 MIT；如果 MotoNav 想减少“每种转向画一套图标”的维护量，可以借“手机发原始图标位图”的思路。

#### `kabulDasa/tripper`

- 离线优先的摩托导航 Pod 设计：规划器生成 `.trb` 路线包，Pod 端按路口快照/箭头/距离提示；带浏览器模拟器和错误路线/隧道/掉电测试。
- 目前是设计阶段，固件只有骨架；适合参考“离线路线包 + 模拟器”的产品形态。

### 3.4 M5StopWatch C152 生态

| 仓库 | 功能 | 对 1.75C 的价值 |
|---|---|---|
| `m5stack/M5StopWatch-PowerDemo` | 官方功耗实测：L0 8.9 µA、L1 20.5 µA、L2 50-90 µA、IMU 唤醒 37-74 µA、正常 45 mA、显示 144-154 mA、Wi-Fi 98-105 mA、Wi-Fi light sleep 3.4 mA | 功耗预算和唤醒模式的方法论；具体寄存器是 M5PM1，不能直接抄 |
| `m5stack/M5StopWatch-UserDemo` | 官方例程 + FluidBox + 显示常亮时关闭绿色电源灯 | FluidBox/低功耗小技巧 |
| `wararyo/M5StopWatch-wararyoLauncher` | 熄屏 20 mW、4 表盘、抬腕/按键/触摸唤醒、秒表/计时/步数、MultiFirm 宿主 | **抬腕、步数、表盘、低功耗待机的直接参考** |
| `wararyo/M5StopWatch-MultiFirm` | `ota_0` 宿主 + `ota_1..3` 三个 guest，PC 工具写入/备份/恢复 | 多固件共存；1.75C 32 MB Flash 有空间，但要重做分区 |
| `wararyo/M5StopWatch-MuteHid` | BLE HID Telephony Device，控制/显示会议静音 | BLE HID 扩展；和导航无关 |
| `ajaxjiang96/M5StopWatch-SmartKnob` | IMU 陀螺仪虚拟旋钮 + 马达 detent，9 种模式 | 触摸之外的 IMU 交互；1.75C 无马达，只能屏幕反馈 |
| `timit3-github/even-r1-m5stopwatch` | 把 StopWatch 变成 Even G2 的 R1 兼容遥控 | BLE HID/遥控协议 |
| `GOROman/vibewatch` | 六个 AI Agent 状态、批准/拒绝、Plan、按住说话 | AI 编程伴侣形态；不是导航核心 |
| `digitsisyph/codex-micro-stopwatch` | Codex 用量仪表盘、BLE HID、按住说话、USB 麦克风 | AI 编程伴侣；BLE HID + 语音交互参考 |
| `yanxuceo/knoedel-watch` | 德语单词卡、动画角色、间隔重复、语音笔记 | 教育/语音笔记；1.75C 有双麦可做 |
| `uezo/M5StarScope` | 8,921 颗星 + 行星、IMU 指向、语音讲解、离线天文 | 星空/指南针功能；QMI8658 + 手机 GPS 可做 |
| `Trentct/m5stack-stopwatch-avatar` | 12 表情、60fps、dirty rect、触摸/倾斜/摇晃反应 | 增强现有 AvatarEngine |
| `nananauno/M5-guruguru` | 触摸 9 方向头部跟随 | 伴侣模式交互 |
| `six-ddc/M5StopWatch-Kids` | 识字、算术、英语 | 儿童模式；非核心 |
| `tallboyzt/fmo-stopwatch` | 业余无线电通联伴侣 | 垂直场景；非核心 |
| `zqqf16/M5StopWatch-QEMU` / `ochyai/m5stack-stopwatch-simulator` | QEMU / macOS 模拟器，生产 C++ 直接在电脑跑，golden frame | 改 UI 不刷机；当前 1.75C 还没有同等模拟器 |
| `robustonian/M5StopWatch-Flux` | 直接写 framebuffer + dirty-rect + IMU 后台任务，466×466 稳定 33fps | 渲染管线；当前已有 fast flush，可对照 |

> 2026 年 M5StopWatch 生态里大量项目是“AI 编程伴侣”（Codex/Claude 用量、按住说话、审批），它们证明了 BLE HID + 语音 + 小屏状态机的组合，但对 MotoNav 的导航主线不是优先项。

---

## 4. 当前 MotoNav 1.75C 的功能基线

已有：

- 导航：地图/路线/转向环/车道/车速/限速/电子眼/行程进度/到达动画；
- 待机：RIDER / VICE 双主页、目的地抽屉、RTC 离线时钟；
- 音乐：封面、滑动切歌、双击点赞、进度、循环、音量、AMS/HID、歌词；
- 天气：和风天气、详情、预警；
- 来电：整屏来电、触摸接听；
- 传感：QMI8658 倾角/G、航向融合；
- 电源：PWR 熄屏、AXP2101 电源轨/充电、CST9217 深睡、80/240 MHz；
- 玩具：Bricks / Pinball / Marble / Pop / 水波纹 / ADS-B 雷达；
- 伴侣：AvatarEngine（震动改声音）；
- 手机：Android 8.3.26（标准/读秒）、iOS 0.15.1；Android 已有 `RideRecorder` 骑行轨迹。

明确没有：

- 通用通知中心（只有来电；`AmapNotifyListener` 只处理高德通知）；
- 抬腕/落腕唤醒（只有 PWR）；
- OBD-II / TPMS / 车辆遥测；
- 矢量地图窗口（当前是手机截屏栅格 `TYPE_RASTER=3`）；
- 真正的 AOD/表盘（RIDER/VICE 是待机页，不是 1 Hz 常显）；
- 闹钟/计时器/秒表；
- 步数/睡眠；
- OTA/网页刷机（USB 烧录）；
- 语音助手/语音笔记（ES7210 双麦未使用）；
- App 启动器/动态 App；
- 多固件共存。

---

## 5. 功能缺口矩阵

工作量：S ≈ 1-2 天，M ≈ 3-5 天，L ≈ 1-2 周，XL ≈ 2 周以上（按熟悉代码后的净开发量估算）。
优先级：P0 = 现在最划算；P1 = 有明确差异化；P2 = 有余力再做。

| 功能 | 当前状态 | 最佳参考 | 1.75C 硬件 | 工作量 | 优先级 |
|---|---|---|---|---|---|
| 抬腕/落腕/触摸唤醒 | 只有 PWR | `muki01` `wrist_gesture.h`；`wararyoLauncher` | QMI8658 可用 | S | **P0** |
| 通用通知中心 | 只有来电 | `muki01` 通知存储；`AmoledOS` ANCS；`bikepod` ANCS/AMS | BLE + 手机 | M | **P0** |
| 手机侧扩展数据 | 天气/限速/电子眼 | `bikepod` 的 `PHONEBAT/RAIN/FUELNEXT/BUMP` | 手机 | S | **P0** |
| 骑行摘要 | 手机有轨迹；圆屏只有到达 | `OBD2` `ride_summary.c`；`bikepod` `ARRIVED`；`MotoNav-CYD` | 手机数据 | S/M | **P0** |
| UI 毛玻璃/AA 矢量 | 点阵/普通矢量 | `Gyro` `UiModern.h` | 466×466 | S/M | **P0** |
| OBD-II 车辆遥测 | 无 | `OBD2`、`Blueline`、`OBD-HUB-WATCH` | 1.75C 需手机转发/双角色 | L | P1 |
| TPMS 胎压 | 无 | `OBD2` `tpms_*`；`go-marker`；`Blueline` | 外置 BLE 传感器 | M/L | P1 |
| 矢量地图窗口 | 截屏栅格 | `Pan-wh` `mw/mr/mb/mp/me`；`bikepod` `MAPBEGIN/MAP` | 手机 + 设备光栅化 | L/XL | P1 |
| AOD/表盘 | RIDER/VICE 待机页 | `muki01` `aod.cpp`；`Pan-wh` `photo_rt.cpp`；`wararyoLauncher` | AMOLED 适合 | M | P1 |
| 闹钟/计时/秒表 | 无 | `muki01` alarms/timer/stopwatch；`soRound` | RTC/内部 RTC | M | P1 |
| OTA/网页刷机 | USB 烧录 | `soRound` 双 OTA + 回滚；`Pip` Web Serial；`muki01` 浏览器 OTA | 32 MB Flash | L | P1 |
| 步数/睡眠 | 无 | `muki01` `step_counter.h`/`sleep_tracker.cpp`；`soRound` | QMI8658 | M | P2 |
| 语音助手/语音笔记 | ES7210 未用 | `brookesia` AI Chatbot；`StarScope` 语音；`knoedel` 语音笔记 | 双麦 + ES8311 | XL | P2 |
| App 启动器/多应用 | 页面式 | `brookesia`、`soRound`、`AmoledOS` | 无 SD，但有 Flash | XL | P2 |
| 动态 App | 无 | `AmoledOS` `.so`/Lua | 无 SD | XL | P2 |
| 游戏扩展 | 4 + 水波纹 + 雷达 | `brookesia` 2048/Flappy/Snake；`soRound` 迷宫/流体；`Pip` 6 款 | 触摸/IMU | S/M | P2 |
| 星空/指南针 | 无 | `M5StarScope` | QMI8658 + 手机 GPS | M | P2 |
| 多固件共存 | 无 | `MultiFirm` | 32 MB Flash，需分区 | L | P2 |
| i18n | 中文图片 + 英文界面 | `soRound` `i18n.c`；`Stait-Watch` | 字体/Flash | S/M | P2 |
| BLE HID 扩展 | 已有媒体键 | `MuteHid`、`SmartKnob`、`Even R1` | BLE | S | P2 |

---

## 6. 推荐批次

### 批次 A（P0，1-2 天，低风险）

目标：让 1.75C 更像“完整手表”，不动地图主链路。

1. **抬腕/落腕/触摸唤醒**
   - 直接移植 `muki01` 的 `wrist_gesture.h`（纯 C++，无 Arduino 依赖），按 50 Hz 喂 QMI8658。
   - 注意坐标系：`x` 指向 3 点、`y` 指向 12 点、`z` 垂直屏幕向外；1.75C 的安装方向需要先用 `Serial` 打印三轴确认，再决定是否翻转。
   - 屏幕关闭时进入“抬腕检测”，抬起稳定 160 ms 才亮；亮屏后手臂放下 600 ms 息屏；触摸/按键始终可唤醒。
   - 和现有 `PwrKeyScreen.h`/`ScreenState.h` 的熄屏状态机合并，避免两套状态互相打架。

2. **手机侧扩展数据**
   - 在现有 `#` 文本协议里增加：`#PB,<电量>,<充电>`、`#RAIN,<分钟>`、`#FUEL,<米>`、`#BUMP,<米>`；字段缺失保留旧值，`-1` 表示无数据。
   - 圆屏在导航页状态栏/边缘显示小图标，不抢主信息。
   - Android `BatteryManager`/天气/路线数据都已有，iOS 侧需要补 `BatteryMonitor` 和 Open-Meteo。

3. **骑行摘要**
   - 到达时手机发 `#RIDE,<km>,<分钟>,<均速>,<最高>,<最大倾角L>,<最大倾角R>,<最大G>`；圆屏显示 5 秒摘要页。
   - Android 的 `RideRecorder` 已有距离/最高速/轨迹，补均速和倾角/G 即可；iOS 0.16 的 `RideBridge`/`GRecorder` 可直接参考。
   - 设备端暂不写日志，避免 16 MB 分区和 LittleFS 的新风险。

4. **UI 毛玻璃/AA 矢量**
   - 从 `Gyro` 的 `UiModern.h` 取 `drawTranslucentRoundRect()`、`drawTranslucentCircle()` 和 AA 字模，只用在待机页、设置页、音乐页。
   - 地图页保持现有 fast flush，避免半透明逐像素混合拖慢帧率。

验收：抬腕 10 次亮 10 次、放下 5 次灭 5 次；通知/手机数据不影响导航帧率；摘要页不阻塞到达动画。

### 批次 B（P1，3-5 天）

1. **通用通知中心**
   - Android：把 `AmapNotifyListener` 扩展成可配置白名单（微信/短信/QQ/邮件等），过滤 `isOngoing`、空标题、自己 App；发 `#N,<app>,<title>,<body>`，长文本截断到 200 字节。
   - iOS：参考 `AmoledOS/components/aos_ble/aos_ancs.c` 或 `bikepod/firmware/pod/Phone_Notify.cpp` 实现 ANCS；需要配对和“允许通知”权限。
   - 固件：20 条环形队列 + 横幅 + 通知列表页；免打扰/骑行中静默开关；不存储验证码/密码类通知。

2. **闹钟/计时/秒表**
   - `muki01` 的 `services/alarms.cpp`、`countdown.cpp`、`ui/apps/stopwatch_app.cpp` 可参考状态机和 UI。
   - 1.75C 用 ESP32 内部 RTC + AXP2101；先做“亮屏时闹钟”，再做“熄屏 RTC 唤醒”，后者要确认 AXP2101 中断和 `esp_sleep` 配置。

3. **AOD**
   - 1 Hz 刷新、亮度 5-10%、纯黑背景、每分钟像素偏移 1-2 px；每 2 分钟换位置，避免烧屏。
   - `muki01` `aod.cpp` + `Pan-wh` `photo_rt.cpp` 的亮度封顶/偏移/自动息屏三段式。

验收：通知 20 条不丢、不卡导航；AOD 1 小时无残影；闹钟亮屏/熄屏都能响。

### 批次 C（P1，1-2 周，差异化）

1. **OBD-II**
   - 先在 1.85B 上直接跑 `Gnzlt/ESP32-S3-Touch-OBD2`，验证 ELM327 适配器和车型 PID。
   - 1.75C 建议**手机转发**：Android 连 ELM327（BLE Central），把 RPM/速度/水温/电压/油门/负荷/故障码打包进现有 BLE 文本协议；避免 ESP32 同时做“手机外设 + ELM327 主机”的双角色风险。
   - 新页面：`BIKE`（RPM 弧、速度、档位、水温、电压、瞬时油耗）；故障码用列表；低油压/高温/高转速报警。
   - 明确“信息展示，不替代原车仪表”，骑行中不做复杂交互。

2. **TPMS**
   - 手机连 BLE 胎压传感器（或 OBD2 的 TPMS 服务），转发前/后轮压力和温度；圆屏低胎压红色环 + 声音。
   - `OBD2` 的 `tpms_pair.c`/`tpms_alerts.c`/`tpms_formats.c` 是最完整的参考；`go-marker` 的 TPMS 页面更简洁。

3. **矢量地图窗口**
   - 先实现 `Pan-wh` 的 `mw/mr/mb/mp/me`：手机把当前位置周围约 500 m 的道路/建筑/路线转成局部米坐标，圆屏用现有 `MapFrame` 相机旋转/缩放。
   - 保留现有栅格图作为 fallback；手机端先双发，确认新管线稳定后再切默认。
   - 收益：每 100 m 只传约 2 KB，地图可平滑旋转，不再受截屏分辨率和压缩限制。
   - 风险：手机端几何生成、建筑数据、坐标系（GCJ-02）和边界处理，工作量大。

4. **OTA**
   - 先确认 32 MB Flash 的分区方案（当前只用前 16 MB），再考虑双 OTA + 回滚。
   - `soRound` 的双 OTA/回滚/R2 分发是最完整的生产参考；`Pip` 的 Web Serial 刷机适合“USB 一键刷”；`muki01` 的浏览器 OTA + PIN 适合无 App 场景。
   - 优先级低于 OBD/TPMS，因为它不直接增加骑行功能，但会显著降低后续迭代成本。

### 批次 D（P2，长期）

- 步数/睡眠：`muki01` 算法可直接用；对骑行设备是“ off-bike 增值”，不是核心。
- 语音：先做“语音笔记/语音命令”（ES7210 + ES8311），再做云端 AI；Brookesia 的 OpenAI WebRTC 和 StarScope 的语音讲解是两条不同路线。
- App 启动器/动态 App：Brookesia/soRound/AmoledOS 都证明可行，但会重写现有页面模型；除非决定把 MotoNav 变成“骑行 OS”，否则不做。
- 游戏/星空/多固件：有余力再做；游戏可先加 2048/Flappy/Snake，星空用 M5StarScope 的星表 + QMI8658。

---

## 7. 代码索引（移植时直接看这里）

| 想做的事 | 文件 |
|---|---|
| 抬腕/落腕 | `doc/github-refs/muki01__Smartwatch-ESP32/Smartwatch/src/algorithms/wrist_gesture.h` |
| 步数 | `.../src/algorithms/step_counter.h` |
| 睡眠 | `.../src/services/sleep_tracker.cpp` |
| 通知存储/通知中心 | `.../src/services/notifications.h`、`.../src/ui/system/notification_center.cpp` |
| Gadgetbridge BLE | `.../src/services/bluetooth.cpp`、`.../src/services/phone.cpp` |
| AOD/烧屏 | `.../src/ui/system/aod.cpp`；`Pan-wh__esp32-car-dashboard/firmware/dashboard/photo_rt.cpp` |
| OTA | `.../src/services/ota.cpp`；`soBigRice__soRound_os/GeekTool-IDF/main/app_ota.c` |
| ANCS | `charliejgallo__ESP32S3_AmoledOS/components/aos_ble/aos_ancs.c`；`iamsachin__bikepod/firmware/pod/Phone_Notify.cpp` |
| AMS/封面 | `iamsachin__bikepod/firmware/pod/Phone_Notify.cpp`、`Phone_Screens.cpp` |
| OBD/ELM327 | `Gnzlt__ESP32-S3-Touch-OBD2/main/obd/`、`main/elm/`、`main/ble/obd_link.c` |
| OBD 倾角/G | `Gnzlt__ESP32-S3-Touch-OBD2/main/sensors/motion.h`、`main/ui/page_lean.c` |
| TPMS | `Gnzlt__ESP32-S3-Touch-OBD2/main/tpms/`、`main/tpms_service.c`；`g00gander__go-marker` 的 TPMS 页面 |
| 骑行摘要/日志 | `Gnzlt__ESP32-S3-Touch-OBD2/main/obd/ride_summary.c`、`main/sd_log.c`；`iamsachin__bikepod/firmware/pod/BLE_Link.h` 的 `ARRIVED` |
| 矢量地图窗口 | `Pan-wh__esp32-car-dashboard/docs/ble.md`、`firmware/dashboard/nav_map.c`；`iamsachin__bikepod/firmware/pod/Map_Layer.h`；`g00gander__go-marker/proto/nav_protocol.h` |
| 极简 HUD 状态机 | `deanthecoder__SteedPilot/include/SteedPilot/NavState.h`、`src/App.cpp` |
| 毛玻璃/AA UI | `Gyro-Seek__MotoNav_Watchcopy/firmware/boards/stopwatch/MotoNav/UiModern.h` |
| 1.75C 类手机 UI | `fellow99__ESP32-S3-Touch-AMOLED-1.75C-brookesia/components/brookesia_phone/` |
| 1.75C 多 App 系统 | `soBigRice__soRound_os/GeekTool-IDF/main/launcher.c`、`app_*.c`、`i18n.c` |
| 1.75C 宠物/通知/游戏 | `NovikovVisuals__Pip-KeyPet/ui/`、`firmware/main/ble.c` |
| 动态 App/Lua | `charliejgallo__ESP32S3_AmoledOS/components/aos_ble/aos_ancs.c`、`apps/lua/main/lua_app.c`、`sim/` |
| 程序化头像 | `Trentct__m5stack-stopwatch-avatar/` |
| 星空 | `uezo__M5StarScope/` |
| 功耗数据 | `m5stack__M5StopWatch-PowerDemo/README.md`、`tests/*/README.md` |
| iPhone 导航/0.16 | `Koo0527__New-MotoNav/ios/MotoNav/`（`NavigationCoordinator.swift`、`RouteWaypointEditor.swift`、`TunnelDimming.swift`、`RideBridge.swift`、`GRecorder.swift`） |
| 停屏/深睡 | `curisama__The-Badge/main/port_esp.c`（已有 `doc/09`） |

---

## 8. 许可证与风险

### 8.1 许可

| 类型 | 仓库 | 处理 |
|---|---|---|
| MIT | `muki01/Smartwatch-ESP32`、`charliejgallo/ESP32S3_AmoledOS`、`fellow99/…brookesia`、`Gnzlt/ESP32-S3-Touch-OBD2`、`deanthecoder/SteedPilot`、`Edge3dSolutions/Blueline-One`、`NovikovVisuals/Pip-KeyPet`、`uezo/M5StarScope`、`m5stack/M5StopWatch-PowerDemo`、`m5stack/M5StopWatch-UserDemo`、`markseder/MotoNav-CYD`、`retweakeverything/esp32-oled-maps-hud` | 可移植，保留版权/许可声明 |
| Apache-2.0 | `g00gander/go-marker`、`waveshareteam/*` | 可移植，保留 NOTICE/许可 |
| 未声明 | `soBigRice/soRound_os`、`Pan-wh/esp32-car-dashboard`、`iamsachin/bikepod`、`kabulDasa/tripper` | **只参考思路，不复制代码**；如要使用先联系作者 |
| 自定义非商业 | `Koo0527/New-MotoNav`、`Gyro-Seek/MotoNav_Watchcopy` | 与当前工程同源，合入保留署名/条款，禁止商用 |
| AGPL-3.0 | `Trentct/m5stack-stopwatch-avatar` | 合入/分发需遵守 AGPL；当前 `AvatarEngine` 已是 AGPL，注意边界 |
| GPL-3.0 | `joaquimorg/OLEDS3Watch`、`cerary/OBD-HUB-WATCH` 等 | 合入会传染，尽量只参考交互 |

### 8.2 硬件/工程风险

- **1.75C 无 SD、无马达、无双实体键**：任何依赖这些的功能都要做触摸/声音/Flash 替代。
- **32 MB Flash 只用 16 MB**：OTA/日志/多固件前先改分区；不要直接套用 1.85B/StopWatch 的分区表。
- **ES7210 双麦未初始化**：语音功能需要新增驱动、I2S 配置和可能的录音分区；`05_Spec_Analyzer` 是起点。
- **BLE 双角色**：手机外设 + ELM327 主机同时跑，NimBLE 可以但当前 `BLEDevice` 封装未必支持；优先手机转发。
- **AMOLED 烧屏**：AOD/照片表盘必须做亮度封顶、像素偏移、自动息屏；不要长时间纯白高亮。
- **light sleep**：`doc/09` 已分析 PSRAM/BLE/USB 风险；没有实测前不要替换现有熄屏 80 MHz 方案。
- **安全**：OBD/TPMS/导航信息只做展示，不替代原车仪表/现场标志；骑行中通知应默认静默或只显示来电。

---

## 9. 建议的下一步

如果只做一件事：**抬腕/落腕唤醒 + AOD**。这是最低成本、最直接把 1.75C 从“导航屏”变成“手表”的改动。

如果做一套：**批次 A（抬腕 + 手机数据 + 骑行摘要 + UI 毛玻璃）**，预计只增加几 KB 级 Flash 和少量 RAM，不动地图/电源主链路，风险最低。

如果要一个差异化卖点：**OBD-II + TPMS（手机转发）**。这是摩托车用户愿意为之买单的功能，也比“再加一个游戏”更有价值。

如果要做架构升级：**矢量地图窗口**。先按 `Pan-wh` 的 `mw/mr/mb/mp/me` 做增量协议，保留栅格 fallback，不要一次替换现有地图管线。

如果要做生态：**先合 iOS 0.16**（`Koo0527/New-MotoNav`），再考虑通知/OTA；iOS 0.16 的 26 个新文件和 pages.5 楼块连续性，是当前仓库最直接的功能缺口。

---

## 10. 附录：搜索与更新

### 10.1 本轮使用的主要查询

```text
m5stopwatch
M5StopWatch
stopwatch m5stack
ESP32-S3 AMOLED 1.75
CO5300 esp32
esp32s3 smartwatch
esp32 watch lvgl
esp32-s3 smartwatch esp-idf
amoled watch firmware esp32
brookesia esp32
1.75 esp32 round
esp32-s3 amoled 1.43
MotoNav
motorcycle navigation esp32
esp32 motorcycle hud
OBD-II esp32 display
esp32 motorcycle dashboard
esp32 gps hud oled
org:waveshareteam AMOLED
org:m5stack stopwatch
M5StopWatch in:name
```

### 10.2 更新本地快照

```bash
cd doc/github-refs
for d in */ ; do
  [ -d "$d/.git" ] || continue
  git -C "$d" fetch --depth 1 origin || echo "拉取失败: $d"
done
# 单个仓库更新到远端当前分支：
git -C muki01__Smartwatch-ESP32 reset --hard @{u}
```

新增仓库时建议沿用 `owner__repo` 命名，并在 `doc/github-refs/README.md` 的增量表里登记 commit/日期/许可/大小。
