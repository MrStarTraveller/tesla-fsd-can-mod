# 中文说明

[English README](README.en.md)

# 社区 Discord

https://discord.gg/ZTQKAUTd2F

# CanFeather - Tesla FSD CAN 总线启用器

> **为什么要公开这个项目？** 有些卖家会为类似方案收取高达 500 欧元的费用。我们认为这严重高估了成本。板卡本身大约只要 20 欧元，即使算上人工，合理价格也不应超过 50 欧元。这个项目存在的目的，就是让大家不必为此付出过高代价。

原始仓库致谢请见：https://gitlab.com/Starmixcraft/tesla-fsd-can-mod

## 前置条件

**你的车辆必须已经拥有有效的 FSD 套餐**，无论是买断还是订阅都可以。本板卡是在 CAN 总线层面启用相关 FSD 功能，但车辆本身仍然需要 Tesla 提供的有效 FSD 授权。

如果你所在地区无法开通 FSD 订阅，可以通过下面的方法绕开：

1. 注册一个支持 FSD 订阅地区的 Tesla 账号，例如加拿大。
2. 将车辆转移到该账号下。
3. 使用该账号开通 FSD 订阅。

这样你就可以在全球任意地区激活 FSD 订阅。

## 它能做什么

这份固件运行在支持 CAN 总线的 Adafruit Feather 板卡上，支持 RP2040 CAN + MCP2515、M4 CAN Express + ATSAME51 原生 CAN，以及任意带原生 TWAI 外设的 ESP32 板卡。它会拦截特定的 CAN 报文，以启用并配置 Full Self-Driving（FSD）。此外，ASS（Actually Smart Summon）也不再受到欧盟限制。

## 仓库结构说明

这个仓库同时包含两类内容：一类是最终会编译并写入板卡的 C++ 固件；另一类是仅在本地开发机上使用的构建、测试和辅助脚本。

### 会写入板卡的固件部分

- `RP2040CAN.ino`：Arduino IDE 入口
- `src/main.cpp`：PlatformIO 入口
- `include/app.h`：通用启动与主循环
- `include/handlers.h`：车辆逻辑，包含 `LEGACY`、`HW3`、`HW4`
- `include/can_helpers.h`、`include/can_frame_types.h`：报文辅助逻辑与帧结构
- `include/drivers/mcp2515_driver.h`
- `include/drivers/same51_driver.h`
- `include/drivers/twai_driver.h`

这些文件会参与固件编译，最终生成写入板卡的二进制。

### 仅用于本地构建、测试和开发辅助

- `platformio.ini`：PlatformIO 构建配置
- `test/`：宿主机单元测试，不会写入板卡
- `include/drivers/mock_driver.h`：仅供测试使用的 mock 驱动
- `scripts/pio-local.ps1`：本地 PlatformIO 启动脚本，用于把缓存和工具目录尽量放在工作区
- `scripts/native_toolchain.py`：Windows 下 `native` 测试环境的工具链适配脚本
- `guides/`：说明文档与图片

这些文件只在开发机使用，不会进入最终固件。

### 核心功能

- 拦截特定 CAN 总线报文
- 将修改后的报文重新发送回车辆总线

### FSD 激活逻辑

- 监听与 Autopilot 相关的 CAN 帧
- 检查 Autopilot 设置中是否启用了 `"Traffic Light and Stop Sign Control"`
- 将这个设置作为 FSD 的触发条件
- 修改 CAN 报文中所需的位，激活 FSD

### 额外行为

- 读取跟车距离拨杆或设置
- 动态映射为速度档位

### HW4 - FSD V14 特性

- 接近应急车辆检测

### 支持的硬件变体

在 `RP2040CAN.ino` 中通过 `#define` 选择车辆硬件类型：

| Define   | 目标类型        | 监听的 CAN ID    | 说明 |
|----------|-----------------|------------------|------|
| `LEGACY` | HW3 Retrofit    | 1006, 69         | 设置 FSD 启用位，并通过跟车距离控制速度档位 |
| `HW3`    | HW3 车辆        | 1016, 1021       | 与 legacy 提供相同功能 |
| `HW4`    | HW4 车辆        | 1016, 1021       | 扩展速度档位范围，共 5 档 |

> **注意：** 运行 **2026.2.9.X** 固件的 HW4 车辆属于 **FSD v14**。但 **2026.8.X** 分支上的版本仍然是 **FSD v13**。如果你的车辆运行的是 FSD v13（包括 2026.8.X 分支或任何早于 2026.2.9 的版本），即使硬件是 HW4，也请按 `HW3` 编译。

### 如何判断你的硬件类型

- **Legacy**：车辆是 **竖屏中控**，并且是 **HW3**。通常适用于较早的（Palladium 之前）Model S / Model X，这些车原生搭载 HW3 或后续升级为 HW3。
- **HW3**：车辆是 **横屏中控**，并且是 **HW3**。你可以在车机的 **Controls → Software → Additional Vehicle Information** 中查看硬件版本。
- **HW4**：同上，但附加车辆信息页显示为 **HW4**。

### 关键行为

- 当车辆 Autopilot 设置中启用了 **"Traffic Light and Stop Sign Control"** 时，会设置 **FSD enable bit**。
- **速度档位** 由滚轮偏移量或跟车距离设置推导。
- **Nag suppression**：清除手离方向盘提醒位。
- 当 `enablePrint` 为 `true` 时，会通过 `115200` 波特率输出串口调试信息。

### LED 状态说明

- **常亮**：当前没有正在处理的 CAN 帧，固件处于空闲等待状态
- **短暂熄灭**：正在处理接收到的 CAN 帧
- **持续快闪（约 250 ms 节奏）**：初始化失败，固件没有进入正常 CAN 工作状态
- **持续慢闪（约 500 ms 节奏）**：运行过程中检测到新的驱动故障或恢复事件，固件进入运行期故障提示状态

LED 目前表示的是“活动状态 + 初始化故障状态 + 运行期故障提示”，仍然不是完整的总线健康状态指示灯。

### CAN 报文细节

下表列出了不同硬件类型会监听哪些 CAN 报文，以及具体会做哪些修改。

#### Legacy（HW3 Retrofit）

| CAN ID | Hex | 名称 | 方向 | Mux | 动作 |
|---|---|---|---|---|---|
| 69 | 0x045 | STW_ACTN_RQ | 仅读取 | — | 读取跟车距离拨杆位置，并映射为速度档位 |
| 1006 | 0x3EE | — | 读取 + 修改 | 0 | 从 UI 读取 FSD 状态；设置 bit 46（FSD enable）；把速度档位写入 byte 6 的 bit 1-2 |
| 1006 | 0x3EE | — | 读取 + 修改 | 1 | 清除 bit 19（nag suppression） |

#### HW3

| CAN ID | Hex | 名称 | 方向 | Mux | 动作 |
|---|---|---|---|---|---|
| 1016 | 0x3F8 | UI_driverAssistControl | 仅读取 | — | 读取跟车距离设置，并映射为速度档位 |
| 1021 | 0x3FD | UI_autopilotControl | 读取 + 修改 | 0 | 从 UI 读取 FSD 状态；计算速度偏移；设置 bit 46（FSD enable）；把速度档位写入 byte 6 的 bit 1-2 |
| 1021 | 0x3FD | UI_autopilotControl | 读取 + 修改 | 1 | 清除 bit 19（nag suppression） |
| 1021 | 0x3FD | UI_autopilotControl | 读取 + 修改 | 2 | 将速度偏移写入 byte 0 的 bit 6-7 和 byte 1 的 bit 0-5 |

#### HW4

| CAN ID | Hex | 名称 | 方向 | Mux | 动作 |
|---|---|---|---|---|---|
| 921 | 0x399 | DAS_status | 读取 + 修改 | — | ISA 超速提示音抑制（可选，默认关闭） |
| 1016 | 0x3F8 | UI_driverAssistControl | 仅读取 | — | 读取跟车距离设置，并映射为速度档位（5 档） |
| 1021 | 0x3FD | UI_autopilotControl | 读取 + 修改 | 0 | 从 UI 读取 FSD 状态；设置 bit 46（FSD enable）；设置 bit 60（FSD V14）；设置 bit 59（应急车辆检测） |
| 1021 | 0x3FD | UI_autopilotControl | 读取 + 修改 | 1 | 清除 bit 19（nag suppression）；设置 bit 47 |
| 1021 | 0x3FD | UI_autopilotControl | 读取 + 修改 | 2 | 将速度档位写入 byte 7 的 bit 4-6 |

> 信号名称来源于 [tesla-can-explorer](https://github.com/mikegapinski/tesla-can-explorer)，作者 [@mikegapinski](https://x.com/mikegapinski)。

## 支持的板卡

| 板卡 | CAN 接口 | 使用库 | 状态 |
|------|----------|--------|------|
| Adafruit Feather RP2040 CAN | MCP2515 over SPI | `mcp2515.h`（autowp） | 本地已编译验证，待实板测试 |
| Adafruit Feather M4 CAN Express（ATSAME51） | 原生 MCAN 外设 | `Adafruit CAN`（`CANSAME5x`） | 本地已编译验证，待实板测试 |
| ESP32 + CAN transceiver（例如 ESP32-DevKitC + SN65HVD230） | 原生 TWAI 外设 | ESP-IDF `driver/twai.h` | 本地已编译验证，待实板测试 |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | 基于 ESP32 TWAI 的 CA-IS3050G | ESP32 TWAI | 本地已编译验证，待实板测试 |

> 这里的“本地已编译验证”表示这些 PlatformIO 环境已经在当前开发机上完成构建验证；不表示我已经把固件刷入对应实物板卡，也不表示已经完成上车联调。

## 硬件要求

- 上面列出的任意一种支持板卡
- 与车辆 CAN 总线的连接能力（500 kbit/s）

**Feather RP2040 CAN** 需要板卡暴露以下引脚（由 earlephilhower 的板卡定义提供）：
- `PIN_CAN_CS`：MCP2515 的 SPI 片选
- `PIN_CAN_INTERRUPT`：中断引脚（当前未使用，代码采用轮询）
- `PIN_CAN_STANDBY`：CAN 收发器待机控制
- `PIN_CAN_RESET`：MCP2515 硬件复位

**Feather M4 CAN Express** 使用 ATSAME51 原生 CAN 外设，需要：
- `PIN_CAN_STANDBY`：CAN 收发器待机控制
- `PIN_CAN_BOOSTEN`：将 3V 升到 5V 以满足 CAN 信号电平的升压开关

**ESP32 + CAN 收发器** 使用原生 TWAI 外设，需要：
- 一个外部 CAN 收发器模块，例如 SN65HVD230、TJA1050 或 MCP2551
- `TWAI_TX_PIN`：连接到收发器 TX 的 GPIO，默认 `GPIO_NUM_5`
- `TWAI_RX_PIN`：连接到收发器 RX 的 GPIO，默认 `GPIO_NUM_4`

**重要：** Feather CAN 板上的 120 Ω 终端电阻必须切断，对应 RP2040 上的 **TERM** 跳线，M4 上的 **Trm** 跳线。如果你使用的是带终端电阻的 ESP32 外置 CAN 收发器，也要移除或关闭它。车辆 CAN 总线本身已经有终端，额外并联一个终端电阻会导致通信错误。

## 安装

### 方案 A：Arduino IDE，仅烧录

*如果你只是想把固件刷进板卡，推荐这个方式，不需要命令行工具。*

#### 1. 安装 Arduino IDE

从 [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software) 下载。

#### 2. 添加板卡包

**Feather RP2040 CAN：**
1. 打开 **File → Preferences**。
2. 在 **Additional Board Manager URLs** 中添加：
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. 打开 **Tools → Board → Boards Manager**，搜索 **Raspberry PI Pico/RP2040** 并安装。
4. 选择 **Adafruit Feather RP2040 CAN** 作为板卡。

**Feather M4 CAN Express：**
1. 在 **Additional Board Manager URLs** 中添加：
   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```
2. 在 Boards Manager 中安装 **Adafruit SAMD Boards**。
3. 选择 **Feather M4 CAN (SAME51)** 作为板卡。
4. 通过 Library Manager 安装 **Adafruit CAN** 库。

**ESP32 板卡：**
1. 在 **Additional Board Manager URLs** 中添加：
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. 在 Boards Manager 中安装 **esp32 by Espressif Systems**。
3. 选择你的 ESP32 板型，例如 **ESP32 Dev Module**。

#### 3. 安装所需库

通过 **Sketch → Include Library → Manage Libraries...** 安装：

- **Feather RP2040 CAN**：`MCP2515 by autowp`
- **Feather M4 CAN Express**：`Adafruit CAN`
- **ESP32**：无需额外安装，TWAI 驱动已经内置在 ESP32 Arduino Core 中

#### 4. 选择板卡和车辆类型

在 `RP2040CAN.ino` 顶部附近，取消注释与你的**板卡**对应的那一行：

```cpp
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
//#define DRIVER_SAME51  // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
//#define DRIVER_TWAI    // ESP32 boards with built-in TWAI (CAN) peripheral
```

然后取消注释与你的**车辆**对应的那一行：

```cpp
#define LEGACY // HW4, HW3, or LEGACY
//#define HW3
//#define HW4
```

#### 5. 上传

1. 用 USB 连接板卡。
2. 在 **Tools** 中选择正确的板卡与端口。
3. 点击 **Upload**。

### 方案 B：PlatformIO，用于开发、测试与烧录

*适合需要运行单元测试、为多种板卡构建，或者集成到 CI 的开发者。也可以直接烧录固件。*

#### 前置工具（Windows）

| 工具 | 用途 | 安装方式 |
|------|------|----------|
| [Python 3](https://www.python.org/downloads/) | PlatformIO 运行时 | `winget install Python.Python.3.14` |
| [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) | 构建系统与测试运行器 | `pip install platformio` |
| [MinGW-w64 GCC](https://winlibs.com/) | 本机原生测试编译器 | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |

> 安装完 MinGW-w64 后，请重启终端，确保 `gcc` 和 `g++` 已在 PATH 中。GCC 仅用于 `pio test -e native` 这种宿主机单元测试，交叉编译到 Feather 板卡时仍使用 PlatformIO 自带的 ARM 工具链。

#### 构建

1. 通过编辑 `src/main.cpp` 顶部附近的宏来选择车辆类型：
   ```cpp
   #define LEGACY  // 改成 HW4、HW3 或 LEGACY
   ```
   板卡由 PlatformIO 环境自动决定，也就是 `-e feather_rp2040_can`、`-e feather_m4_can` 或 `-e esp32_twai`。

2. 为你的板卡执行构建：
   ```bash
   # Adafruit Feather RP2040 CAN
   powershell -ExecutionPolicy Bypass -File .\scripts\pio-local.ps1 run -e feather_rp2040_can

   # Adafruit Feather M4 CAN Express (ATSAME51)
   powershell -ExecutionPolicy Bypass -File .\scripts\pio-local.ps1 run -e feather_m4_can

   # ESP32 with TWAI (CAN) peripheral
   powershell -ExecutionPolicy Bypass -File .\scripts\pio-local.ps1 run -e esp32_twai
   ```

   如果你不介意把 PlatformIO 的缓存与工具链放到系统默认目录，也可以直接使用 `pio run ...`。

#### 烧录

用 USB 连接板卡后执行：

```bash
# Adafruit Feather RP2040 CAN
pio run -e feather_rp2040_can --target upload

# Adafruit Feather M4 CAN Express (ATSAME51)
pio run -e feather_m4_can --target upload

# ESP32
pio run -e esp32_twai --target upload
```

> **提示：** 对 Feather 板来说，如果无法识别设备，可以双击 **Reset** 键进入 UF2 bootloader，再重试上传。对 ESP32 来说，如果自动复位上传失败，可以在上传时按住 **BOOT** 键。

#### 运行测试

单元测试运行在你的本机上，不需要实际硬件：

```bash
powershell -ExecutionPolicy Bypass -File .\scripts\pio-local.ps1 test -e native
```

如果你已经自行配置好了 PlatformIO 和本机编译器，也可以继续直接使用 `pio test -e native`。

### 接线

推荐的接入点是 [**X179 接头**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/)：

| 引脚 | 信号 |
|------|------|
| 13 | CAN-H |
| 14 | CAN-L |

将 Feather 的 CAN-H 与 CAN-L 分别连接到 X179 的 13、14 脚。

对于 **2020 年及以前的 legacy Model 3**，如果车辆没有配备 X179 接口（是否存在取决于生产日期），推荐接到 [**X652 接头**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/)：

| 引脚 | 信号 |
|------|------|
| 1 | CAN-H |
| 2 | CAN-L |

将 Feather 的 CAN-H 与 CAN-L 分别连接到 X652 的 1、2 脚。

## 速度档位

速度档位决定车辆在 FSD 下的驾驶激进程度。不同硬件类型的配置方式略有不同：

### Legacy、HW3 与 HW4 档位映射

| Distance | Profile (HW3) | Profile (HW4) |
| :--- | :--- | :--- |
| 2 | ⚡ Hurry | 🔥 Max |
| 3 | 🟢 Normal | ⚡ Hurry |
| 4 | ❄️ Chill | 🟢 Normal |
| 5 | — | ❄️ Chill |
| 6 | — | 🐢 Sloth |

## 串口监视器

使用 **115200 baud** 打开串口监视器，可以查看实时调试输出，包括 FSD 状态和当前速度档位。如果要关闭日志，把 `enablePrint = false`。

## 板卡移植说明

项目使用抽象的 `CanDriver` 接口，因此所有与车辆逻辑相关的部分，例如 handler、bit 操作、速度档位，都是各板卡共用的。不同板卡只需要替换驱动实现。

**不同板卡会变化的部分：**
- **RP2040 CAN**：`mcp2515.h`（autowp），基于 SPI，使用 struct 读写，需要 `PIN_CAN_CS`
- **M4 CAN Express**：`Adafruit_CAN`（`CANSAME5x`），使用原生 MCAN 外设和 packet-stream API，需要 `PIN_CAN_BOOSTEN`
- **ESP32 TWAI**：ESP-IDF `driver/twai.h`，基于原生 TWAI 外设和 FreeRTOS 队列 RX，需要外部 CAN 收发器和两个 GPIO

**保持一致的部分：**
- 所有 handler 结构与 bit 操作逻辑
- 车辆相关行为，例如 FSD 启用、nag suppression、速度档位
- 串口调试输出

### 驱动诊断信息

驱动层现在维护了一组轻量诊断计数，用于帮助定位构建后在实车上的初始化或通信问题，包括：

- 初始化失败次数
- 过滤器配置失败次数
- 中断配置失败次数
- 无报文读取次数
- 读取失败次数
- 发送失败次数
- 总线恢复次数

这套诊断信息主要用于开发和后续扩展，目前默认不会自动打印到串口。

## 开发与测试

项目使用 [PlatformIO](https://platformio.org/) 和 [Unity](https://github.com/ThrowTheSwitch/Unity) 测试框架。

### 项目结构

```text
include/
  can_frame_types.h       # 可移植的 CanFrame 结构
  can_driver.h            # 抽象 CanDriver 接口
  can_helpers.h           # setBit、readMuxID、isFSDSelectedInUI、setSpeedProfileV12V13
  handlers.h              # CarManagerBase、LegacyHandler、HW3Handler、HW4Handler
  app.h                   # 所有入口共用的 setup/loop 逻辑
  drivers/
    mcp2515_driver.h      # MCP2515 驱动（Feather RP2040 CAN）
    same51_driver.h       # CANSAME5x 驱动（Feather M4 CAN Express）
    twai_driver.h         # ESP32 TWAI 驱动
    mock_driver.h         # 单元测试使用的 Mock 驱动
src/
  main.cpp                # PlatformIO 入口
test/
  test_native_driver_diagnostics/ # 驱动诊断接口测试
  test_native_helpers/    # bit 操作辅助函数测试
  test_native_legacy/     # LegacyHandler 测试
  test_native_hw3/        # HW3Handler 测试
  test_native_hw4/        # HW4Handler 测试
  test_native_twai/       # TWAI 过滤器计算测试
RP2040CAN.ino             # Arduino IDE 入口（复用同一套头文件）
```

### 运行测试

```bash
powershell -ExecutionPolicy Bypass -File .\scripts\pio-local.ps1 test -e native
```

测试运行在你的宿主机上，不需要实际硬件。覆盖内容包括 FSD 激活、nag suppression、速度档位映射，以及 bit 操作正确性等 handler 逻辑。

## 警告

**本项目仅用于测试和学习目的。** 向车辆发送错误的 CAN 报文可能导致不可预期的行为、禁用关键安全系统，甚至永久损坏电子部件。CAN 总线控制着刹车、转向、安全气囊等重要系统，一条格式错误的报文就可能造成严重后果。如果你不完全理解自己在做什么，**不要把它装到你的车上**。

## 免责声明

**使用本项目的风险由你自己承担。** 修改车辆 CAN 总线报文可能导致不可预期甚至危险的行为。作者不对任何车辆损坏、人身伤害或由使用本软件引发的法律后果承担责任。本项目还可能导致车辆质保失效，并且可能不符合你所在地区的道路安全法规。驾驶时请始终手握方向盘并保持注意力集中。

## 第三方库

本项目依赖以下开源库，完整许可证文本见 [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)。

| Library | License | Copyright |
|---------|---------|-----------|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT | (c) 2013 Seeed Technology Inc., (c) 2016 Dmitry |
| [adafruit/Adafruit_CAN](https://github.com/adafruit/Adafruit_CAN) | MIT | (c) 2017 Sandeep Mistry |
| [espressif/esp-idf](https://github.com/espressif/esp-idf)（TWAI driver） | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## 许可证

本项目采用 **GNU General Public License v3.0** 许可证，详见 [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html)。
