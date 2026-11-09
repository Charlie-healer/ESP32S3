# StopWatch 功能对照与未移植清单

## 1. “图片”是不是原本的逻辑

分两层：

- **文字转图片的通道是原版逻辑**：App 的 `TextImage` 把中文渲染成 4 位灰度图，固件的 `drawTextImg()` 负责显示。原版 MotoNav 用它显示来电人名字、音乐歌名/歌手、天气地名等，因为圆屏没有中文字库。
- **目的地抽屉使用图片是我们的移植扩展**：StopWatch 原版 `WaitingScreen.h` 用 `drawAaString()` 画抽屉名称和地址，这是一个 ASCII 矢量字体，没有中文字库；App 发来的中文地名在 StopWatch 原版上同样会乱码/缺字。
- 原版图片槽是 0–12；我们新增了 13–16 给目的地名称，并把 `IMG_SLOTS` 从 13 扩到 17。

所以：中文名称现在能正常显示，是复用了原版的图片通道，不是 StopWatch 原本的抽屉逻辑。

## 2. 1.75C 已经有的功能

这些 175 固件本来就有，不需要从 StopWatch 移植：

- 导航、地图、路线、转向、车速、行程环；
- 和风天气（App 侧配置 Host/Key）；
- 音乐数据、HID/AMS 媒体键；
- DragonRadar 龙珠雷达；
- TrafficLightLab 红绿灯实验；
- HomeClock 待机时钟；
- ScreenRotationCommand 屏幕旋转；
- IdlePowerOff 空闲关机（AXP2101）；
- 开机动画、行程进度、转向环；
- 目的地抽屉（本次移植的最小版本）；
- 中文名称图片、触摸 Y 轴修正。

## 3. 还没移植的 StopWatch 功能

### 3.1 UI / 交互类（软件可实现，工作量中等）

| 功能 | StopWatch 原版 | 1.75C 当前 |
|---|---|---|
| 完整待机主页 | `WaitingScreen`：VICE 霓虹背景、矢量时钟、R 星抽屉、动效 | 已移植基础版；设置页 `HOME` 可切换 RIDER/VICE，当前骑手主页保留 |
| 圆屏设置页 | `SettingsPage`/`UiModern`：亮度、地图风格、旋转、3/5/10 分钟/常亮 | 已实现圆屏设置：BRIGHT/ROTATE/THEME/SLEEP/HOME，存 Preferences |
| 音乐播放器页 | `MusicPage`：封面滑动切歌、双击点赞、9 点钟方向音量 HUD | 有音乐页、封面缩略图/暗化背景、点赞、音量；滑动切歌/进度未移植 |
| 完整目的地抽屉 | 8 项、最多显示 4 项、滚动、星/钉图标、动效、震动 | 最多 4 项、无滚动、无震动；名称+地址用 4 位灰度图显示 |
| 毛玻璃/矢量 UI 框架 | `UiModern`、`PageRouter`、`PortScale` | 175 用另一套页面和点阵字 |
| 地图主题细节 | `MapThemes` 的 VICE 风格 | 175 有地图风格/雷达，但视觉细节不完全相同 |
| 伴侣模式视觉/触摸 | `CompanionMode` + `AvatarEngine` | 未移植；且 1.75C 无马达，触感反馈缺失 |

### 3.2 驱动 / 硬件类（需要替换芯片驱动）

| 功能 | StopWatch 原版 | 1.75C 现状 |
|---|---|---|
| 倾角 / G 值仪表 | `LeanAngleMeter`/`PageLeanMeter` + BMI270 | 已移植基础版；1.75C 用 QMI8658 新驱动，倾角/G、MAX L/R、ZERO 已做 |
| RTC 时钟 | `RtcRX8130`（RX8130） | 已移植；1.75C 用 ESP32-S3 内部 RTC，`RTC_NOINIT_ATTR` + `esp_clk_rtc_time()` 跨复位恢复 |
| 真关机 | `PowerManager` + M5PM1 | 未原样移植；1.75C 用 AXP2101，可做等效关机/休眠 |
| 触摸手势 | `TouchInput` + CST820B | 1.75C 是 CST9217；触摸已适配，Y 轴已修正，手势细节需实测 |
| 屏幕/IO 扩展 | `BoardStopWatchInit` + M5IOE1 | 1.75C 用 GPIO1/2 直接复位，M5IOE1 已替代 |

### 3.3 硬件限制，无法原样支持

| 功能 | 原因 |
|---|---|
| 双实体键 A/B 全局操作 | 1.75C 只有 PWR/BOOT，不是两个用户键；只能映射触摸或电源键，体验不同 |
| 马达震动/触感反馈 | 1.75C 文档没有列出马达；伴侣模式和抽屉震动无法还原 |
| M5PM1/M5IOE1/BMI270/RX8130/CST820B 的原芯片行为 | 1.75C 使用 AXP2101/QMI8658/ESP32-S3 内部 RTC/CST9217 等替代方案，只能做等效实现 |

## 4. 优先级建议

如果要继续按路线 B 移植，建议顺序：

1. **圆屏设置页**：亮度、旋转、待机休眠；使用 AXP2101，不依赖新硬件；
2. **音乐页/手势**：显示和触摸都有，音乐数据走手机；
3. **完整目的地抽屉**：地址第二行、滚动、图标放大、动效；
4. **倾角/G 值**：QMI8658 驱动和校准；
5. **伴侣模式视觉/触摸**：AvatarEngine 是 AGPL，且没有马达；
6. **双实体键/马达**：1.75C 硬件不支持，保持不支持。

## 5. 说明

- “未移植”指没有进入 `ports/175c/overlay` 的 175C 固件；上游 `MotoNav_stopwatch/firmware/boards/stopwatch` 代码仍在。
- 部分功能在 175 固件里已有不同实现（例如待机时钟、地图风格、空闲关机），不是完全缺失，只是 UI/细节和 StopWatch 不一致。
- `AvatarEngine` 为 AGPL-3.0；移植和分发要遵守其许可。
