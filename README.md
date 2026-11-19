# MotoNav ESP32-S3 · Waveshare 1.75C 完整工程

这个仓库现在是可独立构建的完整 MotoNav 源码工程，不再只是 1.75C 的 overlay：

- 固件：MotoNav StopWatch 分支 + `ports/175c` 1.75C 移植层；
- Android：普通版 `8.3.26-ui.1`（`com.friday.motonav`）和读秒版源码（single.30）；
- iOS：主线 `0.16.0` 源码，已含手机通过 BLE `#TIME` 给圆屏校时；
- 文档：`doc/` 是 1.75C 移植记录，`docs/` 是上游主线文档。

> 本项目仅供个人非商业学习和使用。上游及第三方组件的许可见 `LICENSE.md`、`licenses/` 和各子目录说明。仓库不包含高德/和风 Key、keystore、描述文件或签名密码。

## 目录结构

```text
firmware/                 MotoNav 固件完整源码
  boards/175/             原版 1.75 板型
  boards/175c/            1.75C 板型（由 ports/175c 构建时生成）
  boards/185b/            1.85B 实验板型
  boards/stopwatch/       M5Stack StopWatch 板型
  libraries/              固件依赖库
ports/175c/               1.75C 引脚、overlay、构建脚本和说明
android/standard/         普通版 Android 源码
android/countdown/        读秒版 Android 内部组件源码
ios/                      iOS 0.16.0 源码（Bundle ID com.friday.motonav）
doc/                      1.75C 移植、构建、问题记录
docs/                     上游主线发布说明和验证记录
licenses/                 第三方许可
scripts/                  构建/测试脚本
```

## 1.75C 固件

构建：

```bash
python3 firmware/build.py --board 175c
```

脚本直接编译 `firmware/boards/175c/MotoNav`（由 `ports/175c/sync_board.py` 从 175 源码 + `ports/175c/overlay` 生成）。修改 overlay 后先运行 `python3 ports/175c/sync_board.py`。产物在：

```text
firmware/dist/175c/panel0/
  MotoNav_universal_1.75C_v7.12.11-175c.1_panel0_app_0x10000.bin
  MotoNav_universal_1.75C_v7.12.11-175c.1_panel0_full_0x0.bin
```

烧录 app 分区（不覆盖 NVS/配对信息）：

```bash
"$ESPTOOL" --chip esp32s3 -p /dev/cu.usbmodem2101 \
  write_flash 0x10000 firmware/dist/175c/panel0/MotoNav_universal_1.75C_v7.12.11-175c.1_panel0_app_0x10000.bin
```

首次安装或救砖才写 `0x0` 的完整镜像；不要手动覆盖镜像头的 flash mode。

## 1.75C 当前功能

- RIDER / VICE 双待机页，设置页 `HOME` 切换并持久化；
- 目的地抽屉 8 项/滚动、音乐封面/滑切/双击点赞/进度/循环/音量、满屏倾角/G 仪表、设置页 SOUND 音量、RTC 离线时钟；
- 主页 `TOY` 按钮：Bricks / Pinball / Marble / Pop 四个游戏、水波纹、ADS-B 飞机雷达（手机取数、BLE 转发，圆屏不开 WiFi）、REC 录音回放、PET 宠物；
- 12 款地图配色：原版 / 罪恶都市 / GTA5 / 赛博霓虹 / 复古纸 / 极简黑白 / 蓝图 / 掌机像素 / 雷达 / 寻宝罗盘 / 卫星图 / 龙珠雷达，昼夜两套，手机 App 切换；
- 地图自动 heading-up 旋转（手机 bearing 无效时用前后定位点算 course 兜底）；双击拖动缩放 0.30–3.00x；**单指长按 450 ms 后上下拖缩放、左右拖手动旋转**，手动旋转时右下角 `AUTO` 可恢复自动跟随；倾角/G 随屏幕任意角度旋转；待机背景跟所选地图主题联动；设置页 ROTATE 支持 0/90/180/270/GRAV 重力随动；
- 伴侣模式 AvatarEngine，震动反馈改为 ES8311 声音；
- PWR 短按熄屏/亮屏，长按保持 AXP2101 硬件关机；熄屏同时把 CST9217 触摸芯片置深睡，亮屏用 RST 唤醒；
- QMI8658 航向融合参与地图转向；AXP2101 未用电源轨已关闭，充电终止/预充 25 mA、截止 3.0 V；
- 亮屏固定 240MHz、QSPI 80MHz、fast flush 16KB 分块、主循环 1ms，熄屏降至 80MHz；
- 设置页亮度（默认 50%）、旋转、主题、睡眠、HOME、AOD 开关；
- 抬腕/落腕/触摸唤醒 + 假 AOD：面板低亮常显、1 Hz 时钟、每 2 分钟 2 px 烧屏偏移；设置页 `AOD` 可关；
- 通知中心：手机转发 `#N|app|title|body`，主页横幅 + 第 4 页列表（20 条、清除、翻页）；
- 手机扩展数据：`#PB` 手机电量、`#RAIN` 降雨、`#FUEL` 加油站、`#BUMP` 减速带；骑行结束显示 `#RIDE` 摘要（距离/时间/均速/最高/倾角/G）；
- 第 5 页 BIKE：手机 ELM327 转发 `#OBD`（转速/车速/水温/电压/负荷/油耗）和 `#TP` 胎压；低胎压/高温/低电压红色提示；
- 页面顺序：音乐 / 主页 / 天气 / 指南针 / 通知 / BIKE，6 个圆点。

## Android

- 普通版：`android/standard/`，版本 `8.3.26-ui.1`，默认包名 `com.friday.motonav`；
- 读秒版：`android/countdown/`，内部包名 `com.friday.motonav.countdown`，最终合并包仍是 `com.autonavi.amapauto`；
- 高德 SDK 使用 `11.2.100`：native 库已按 16 KB page-size 对齐，实机 `TextureMapView` 取图正常；`11.3.100` 取图回归见 `doc/04` §19，不要升级回去；`useLegacyPackaging=false`；
- 需要 JDK 17、Android SDK 35；高德 Key 和签名必须用自己的包名 + SHA1；
- 本仓库不包含 `local.properties`、keystore 或 API Key，按 `doc/03-Android-App-构建与配置.md` 配置。

## iOS

`ios/` 已更新为 MotoNav 主线 0.16.0（Build15 本地整合版），Bundle ID 保持：

- App：`com.friday.motonav`
- 分享扩展：`com.friday.motonav.share`

0.16 带来统一规划入口、途经点/加油站/收藏/历史、隧道调光、`RideBridge`/`GRecorder`、12 种地图风格和新的连接/设备卡界面。`PhoneClockSync.swift` 连接后向圆屏发送 `#TIME,<UTC秒>,<时区偏移>`；1.75C 的 ADS-B radar 已重新接回 `RadarFeeder.swift`。

> 当前 iOS 对齐的是主线导航/音乐/天气/骑行功能；1.75C 专属的目的地抽屉、专辑封面等 BLE 扩展仍以 Android 端为主，iOS 尚未实现同等 UI。完整 Xcode 构建仍需在本机接受 Xcode 许可并安装 Pods。

iOS 构建需要 macOS、Xcode 和 CocoaPods：

```bash
bash scripts/build-ios-local.sh
```

当前没有在 iPhone 真机上编译/安装验证；签名和描述文件需要使用者自己的 Apple 账号配置。

## 文档

- 1.75C 移植、构建、踩坑：优先看 `doc/README.md`；
- 上游功能、发布说明、iOS/Android 说明：看 `docs/`；
- 1.75C 模块和 overlay 入口：`ports/175c/README.md`。

## 参考上游

- https://github.com/zyn950929/MotoNav
- https://github.com/loniper/MotoNav
- https://github.com/Lubywu/MotoNav
- https://github.com/Charlie-healer/MotoNav_stopwatch
