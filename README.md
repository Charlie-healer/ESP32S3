# MotoNav ESP32-S3 · Waveshare 1.75C 完整工程

这个仓库现在是可独立构建的完整 MotoNav 源码工程，不再只是 1.75C 的 overlay：

- 固件：MotoNav StopWatch 分支 + `ports/175c` 1.75C 移植层；
- Android：普通版 `8.3.26-ui.1`（`com.friday.motonav`）和读秒版源码（single.30）；
- iOS：主线 `0.15.1` 源码，已含手机通过 BLE `#TIME` 给圆屏校时；
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
ios/                      iOS 0.15.1 源码（Bundle ID com.friday.motonav）
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
- 目的地抽屉 8 项/滚动、音乐封面/滑切/双击点赞/进度/循环/音量、倾角/G 仪表、RTC 离线时钟；
- 伴侣模式 AvatarEngine，震动反馈改为 ES8311 声音；
- PWR 短按熄屏/亮屏，长按保持 AXP2101 硬件关机；
- 熄屏和空闲待机 CPU 降至 80MHz，触摸/BLE/翻页时回到 240MHz；
- 设置页亮度（默认 50%）、旋转、主题、睡眠、HOME。

## Android

- 普通版：`android/standard/`，版本 `8.3.26-ui.1`，默认包名 `com.friday.motonav`；
- 读秒版：`android/countdown/`，内部包名 `com.friday.motonav.countdown`，最终合并包仍是 `com.autonavi.amapauto`；
- 高德 SDK 已升级到 11.3.100，native 库按 16KB page-size 对齐，`useLegacyPackaging=false`；
- 需要 JDK 17、Android SDK 35；高德 Key 和签名必须用自己的包名 + SHA1；
- 本仓库不包含 `local.properties`、keystore 或 API Key，按 `doc/03-Android-App-构建与配置.md` 配置。

## iOS

`ios/` 为 MotoNav 主线 0.15.1 源码，已把 Bundle ID 改为：

- App：`com.friday.motonav`
- 分享扩展：`com.friday.motonav.share`

包含 `PhoneClockWire.swift`，连接后向圆屏发送 `#TIME,<UTC秒>,<时区偏移>`，与 1.75C 的 `HomeClock`/RTC 离线时钟配合。

> 当前 iOS 对齐的是主线导航/音乐/天气功能；1.75C 专属的目的地抽屉、专辑封面等 BLE 扩展仍以 Android 端为主，iOS 尚未实现同等 UI。

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
