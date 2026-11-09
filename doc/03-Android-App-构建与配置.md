# Android App 构建与配置

## 1. 两套 Android 产品

| 产品 | 包名 | 说明 |
|---|---|---|
| 标准版 | `com.friday.motonav` | 独立 App，配 1.75/1.75C 圆屏 |
| 读秒版内部组件 | `com.friday.motonav.countdown` | Gradle 中间产物，不直接作为最终 App |
| 读秒版最终包 | `com.autonavi.amapauto` | 合并进高德车机版的修改包 |

标准版和读秒版不是同一个产品：

- 标准版是独立 App，直接安装；
- 读秒版最终包是“高德车机版 + MotoNav 增量”，包名必须是 `com.autonavi.amapauto`；
- 把读秒版最终包改成 `com.friday.motonav` 会破坏高德 SDK、原生库、资源合并和读秒数据源，不可行。

## 2. 标准版构建

### 2.1 版本与代码

- 版本：8.3.26-ui.1（versionCode 131）
- 包名：`com.friday.motonav`
- 源码：`android/standard/app/src/main/java/com/friday/motonav`，约 67 个文件
- 主要模块：`MainActivity`、`NavService`、`BleLink`、`MapFeeder`、`Weather`、`QWeatherClient`、`MusicBridge`、`CallWatcher`、`Keys`

### 2.2 前置环境

- JDK 17
- Gradle 8.11.1（或项目 wrapper 8.10.2）
- Android SDK platform 35
- build-tools 35.0.1
- `local.properties` 中写入：

```properties
sdk.dir=/Users/charlie/Library/Android/sdk
```

### 2.3 构建命令

```bash
cd MotoNav_stopwatch/android/standard

export JAVA_HOME=/opt/homebrew/opt/openjdk@17
export PATH="$JAVA_HOME/bin:$PATH"
GRADLE=~/.gradle/wrapper/dists/gradle-8.11.1-bin/bpt9gzteqjrbo1mjrsomdt32c/gradle-8.11.1/bin/gradle

$GRADLE clean assembleDebug --no-daemon
# 或
$GRADLE clean assembleRelease --no-daemon
```

产物：

```
app/build/outputs/apk/debug/app-debug.apk
app/build/outputs/apk/release/app-release.apk
```

### 2.4 签名

| 类型 | keystore | SHA1 | 用途 |
|---|---|---|---|
| debug | `android/standard/.local/debug.keystore` | `86:DD:40:08:E1:EC:2A:AC:C3:12:D1:F9:99:66:96:B7:D9:46:DD:D5` | 调试 APK |
| release | `android/standard/keystore/release.jks` | `C8:A2:BB:6B:0A:5B:6B:C0:F5:B7:A6:00:4C:FE:A4:41:96:ED:F1:C1` | 发布 APK |

- release 密码在 `android/standard/keystore/keystore.properties`；
- 两个 keystore 都要备份，不要提交、不要外传；
- debug 和 release 签名不同，APK 不能互相覆盖安装；
- 换电脑重新生成 debug keystore，SHA1 会变，高德 Key 要重新绑定。

### 2.5 安装

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
# 或
adb install -r app/build/outputs/apk/release/app-release.apk
```

如果从 debug 切到 release，需要先卸载旧包：

```bash
adb uninstall com.friday.motonav
adb install app/build/outputs/apk/release/app-release.apk
```

卸载会清空 App 数据（地图 Key、和风配置、历史记录）。

## 3. 高德 Key

高德 Android Key 需要 **包名 + 签名证书 SHA1** 两项，缺一不可：

- 包名：`com.friday.motonav`
- 调试版 SHA1：`86:DD:40:08:E1:EC:2A:AC:C3:12:D1:F9:99:66:96:B7:D9:46:DD:D5`
- 发布版 SHA1：`C8:A2:BB:6B:0A:5B:6B:C0:F5:B7:A6:00:4C:FE:A4:41:96:ED:F1:C1`

申请步骤：

1. 登录高德开放平台控制台，完成实名认证；
2. 创建应用；
3. 添加 Key，服务平台选 Android；
4. 填入上面的包名和对应 SHA1；
5. 把生成的 Key 填入 App 的“地图 Key”页面。

为什么两项都要：包名标识 App，SHA1 标识签名证书。只有包名的话，别人可以把自己的 App 改成同样的包名来盗用 Key；加上 SHA1 后，高德 SDK 会校验签名证书指纹。

SHA1 是公开指纹，可以填给高德；keystore 文件和密码才是秘密。

## 4. 和风天气

### 4.1 为什么天气需要单独配置

天气不是圆屏自己联网获取的，而是：

```
圆屏滑到天气页
  → 发送 T,W
  → 手机 App 的 CallWatcher 触发 Weather.request()
  → App 用和风 API 查询
  → 通过 BLE 把天气数据发给圆屏
```

因此需要：

1. App 里配置和风 API Host + API Key；
2. 手机有网络和定位权限；
3. 圆屏滑到天气页触发查询。

### 4.2 配置入口

App 底部“地图 Key” → “和风天气” → “配置”。

需要填：

- **API Host**：和风控制台 → 设置 → API Host，形如 `xxxx.qweatherapi.com`；
- **API Key**：和风控制台 → 项目 → 凭据 → API Key。

和风账号至少启用四类服务：天气预报、分钟降水、空气质量、天气预警。

### 4.3 已修复的问题

当前 8.3.26 工程来自 StopWatch 分支，原代码删除了三处天气链路：

1. `CallWatcher.onScreenTouch` 缺少 `"W" -> Weather.request()`；
2. `NavService` 的 `Weather.init(this)` 被注释；
3. “地图 Key”页缺少和风配置入口，`showWeatherConfig()` 是死代码。

已在标准版中补回：

- `CallWatcher` 恢复 `"W" -> Weather.request()`；
- `NavService` 恢复 `Weather.init(this)`；
- `MainActivity.keySection` 增加“和风天气 → 配置”和天气详情入口。

重新编译并安装 debug APK 后，“地图 Key”页会显示：

```
和风天气    未配置：请填写 API Host 与 API Key    配置
```

配置保存后，滑到圆屏天气页即可查询。

### 4.4 排查

```bash
adb logcat -s MotoNavWeather
```

App 的“查看天气详情与数据来源”也会显示 `Weather.info` 的具体错误。常见原因：

- Host 填成了 `api.qweather.com`（App 要求 `xxxx.qweatherapi.com`）；
- Key 没有对应服务权限；
- 手机没有定位权限；
- 高德 Key 与签名不匹配，定位失败后回退手机定位也失败。

## 5. 读秒版

### 5.1 包名层次

| 层 | 包名 |
|---|---|
| 内部组件 | `com.friday.motonav.countdown` |
| 最终合并包 | `com.autonavi.amapauto` |

内部组件已改名，并同步：

- `app/build.gradle.kts` 的 `namespace` / `applicationId`；
- 源码目录和 package/import；
- Manifest 的 signature 权限；
- `prepare-merge.py` 的 `appComponentFactory`；
- `native-route/route_bridge.cpp` 的 JNI 符号：
  `Java_com_friday_motonav_countdown_NativeRouteBridge_*`；
- 测试和构建脚本路径。

### 5.2 构建限制

- 完整读秒版构建只支持 Windows；
- 需要 Python 3、JDK 17、Android SDK 35、build-tools 35.0.0、Gradle 8.10.2、apktool 2.12.1、Zig 0.13.0；
- 需要 `provider.apk`（约 148 MB，脚本自动下载并校验 SHA256）；
- macOS 只能编译中间组件，不能生成最终合并包。

中间组件 APK 不含 `libmotonav_route.so`，不建议当最终读秒版安装。

### 5.3 读秒版的 Key

最终包名是 `com.autonavi.amapauto`，高德 Key 要按这个包名和最终签名 SHA1 申请。

当前 countdown 工程的 debug keystore SHA1：

```
A2:6D:97:E5:8E:70:62:5A:6B:BD:DB:4C:E7:42:1C:36:69:52:A2:36
```

如果要求“发布版安全码 SHA1”，需要给 countdown 工程生成 release keystore，并修改 Windows 构建脚本改用该 keystore 签名。

## 6. iOS

`ios/` 是独立 Swift App，使用同一套 BLE 协议。当前 Android 构建不依赖 iOS；iOS 端的天气/导航功能以对应版本说明为准。

## 7. 权限

App 需要蓝牙扫描/连接、定位、通知等权限。部分手机 ROM 禁止 `adb shell pm grant`，会报：

```
SecurityException: grantRuntimePermission: Neither user 2000 nor current process has
android.permission.GRANT_RUNTIME_PERMISSIONS
```

这种情况下在 App 界面里按系统弹窗授权，或到系统设置里手动打开蓝牙/定位权限。
