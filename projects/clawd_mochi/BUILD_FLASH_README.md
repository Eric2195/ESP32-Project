# Clawd Mochi — 构建与烧录指南

本文档说明如何在 Windows 环境下编译、烧录 `clawd_mochi` 工程，以及常见显示问题的调试思路。

---

## 1. 环境要求

- **操作系统**：Windows 10/11
- **ESP-IDF**：v6.0.1，安装在 `C:\esp\v6.0.1\esp-idf`
- **Python 虚拟环境**：`C:\Users\zr\.espressif\python_env\idf6.0_py3.13_env`
- **芯片目标**：ESP32-C6（由 `sdkconfig` 中的 `CONFIG_IDF_TARGET="esp32c6"` 决定）
- **串口**：默认 `COM10`，可在脚本中修改

> 如果你的安装路径不同，请编辑 `build.py` 和 `build_and_flash.ps1` 中的对应路径。

---

## 2. 工程结构

```
projects/clawd_mochi/
├── main/
│   ├── main.c          # 主程序：动画、HTTP 服务、USB 串口交互
│   ├── display.c       # ST7789 显示屏驱动初始化与绘图 API
│   ├── display.h       # 显示屏接口与引脚定义
│   ├── http_server.c   # Web 控制端 HTTP 服务
│   ├── wifi_ap.c       # WiFi AP + STA 连接
│   └── ...
├── sdkconfig           # ESP-IDF 工程配置（目标芯片 esp32c6）
├── build.py            # 跨 Shell 的 idf.py 包装脚本
├── build_and_flash.ps1 # PowerShell 一键编译烧录脚本
└── BUILD_FLASH_README.md   # 本文件
```

---

## 3. 一键编译烧录

### 3.1 使用 PowerShell（推荐）

```powershell
cd "C:\Users\zr\Desktop\ESP32 Project\ESP32-Project\projects\clawd_mochi"
.\build_and_flash.ps1
```

如果提示执行策略限制：

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### 3.2 使用 build.py（任意终端）

需要先确认你的 ESP32 所在 COM 口。查看方法：

```powershell
Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description
```

编译：

```bash
C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py
```

编译并烧录（以 COM10 为例）：

```bash
C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py -p COM10 flash
```

编译 + 烧录 + 打开串口监控：

```bash
C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py -p COM10 flash monitor
```

完全清理后重新编译：

```bash
C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py fullclean
C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py build
```

> 提示：把 `C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe` 这一段做成系统环境变量或别名会更方便。

---

## 4. 为什么不直接用 `idf.py`

在 Windows Git Bash（MSYS）中直接运行 `idf.py` 会遇到两类问题：

1. `export.ps1` 会检测 MSys/Mingw 并拒绝激活环境。
2. 系统默认 Python 缺少 `click` 等 ESP-IDF 依赖。

`build.py` 做了以下兼容处理：

- 移除 `MSYSTEM` 环境变量，避免激活脚本冲突。
- 显式设置 `IDF_PATH`、`IDF_PYTHON_ENV_PATH`、`PYTHON`。
- 把 `ninja`、`cmake`、`riscv32-esp-elf` 工具链加入 `PATH`。
- 用 `sys.path.insert` 直接导入 `idf.py` 的 `main()` 函数。

因此只要用 ESP-IDF 虚拟环境的 Python 运行 `build.py`，就能在 Git Bash、CMD、PowerShell 中一致地工作。

---

## 5. 显示屏问题调试速查

所有显示相关配置都在 `main/display.c` 的 `display_init()` 中。

### 5.1 颜色偏绿 / 发暗

把颜色反转打开：

```c
esp_lcd_panel_invert_color(panel_handle, true);
```

### 5.2 颜色偏白 / 过曝 / 偏色

尝试切换 RGB/BGR 颜色顺序：

```c
.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,   // 或 RGB
```

同时降低 SPI 时钟：

```c
.pclk_hz = 20 * 1000 * 1000,   // 20 MHz，可继续降到 16 MHz
```

### 5.3 图像上下颠倒 / 左右镜像

调整 `swap_xy` 和 `mirror`：

```c
esp_lcd_panel_swap_xy(panel_handle, true);
esp_lcd_panel_mirror(panel_handle, true, false);
```

常见组合：

| 效果 | `swap_xy` | `mirror_x` | `mirror_y` |
|------|-----------|------------|------------|
| 默认横屏（Rotation 1） | true | false | true |
| 横屏 180° 翻转 | true | true | false |
| 竖屏 | false | false | false |
| 竖屏 180° | false | true | true |

### 5.4 显示偏移 / 边缘黑边 / 花屏

1.54" 240×240 ST7789 的驱动 IC 实际分辨率常为 240×320，可能需要行列偏移。取消下面某一行的注释并测试：

```c
// esp_lcd_panel_set_gap(panel_handle, 0, 80);   // 常见上偏移
// esp_lcd_panel_set_gap(panel_handle, 80, 0);   // 常见左偏移
```

如果左侧有花点，优先降低 SPI 时钟到 16–20 MHz。

---

## 6. 串口交互

烧录完成后，可用任意串口工具（波特率 115200）连接 COM10，或运行：

```bash
C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py -p COM10 monitor
```

串口命令：

| 按键 | 功能 |
|------|------|
| `w` | 普通眼睛动画 |
| `s` | 眯眼动画 |
| `d` | Claude Code 终端界面 |
| `a` | Logo 动画 |
| `q` | 退出终端 |
| `b` | 切换背光 |

---

## 7. 故障排查

### 7.1 `No module named 'click'`

说明你用的不是 ESP-IDF 虚拟环境的 Python。请使用：

```bash
C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py
```

### 7.2 `xtensa-esp32-elf-gcc not found`

- 检查 `sdkconfig` 是否存在。
- 检查 `sdkconfig` 中 `CONFIG_IDF_TARGET` 是否为 `esp32c6`。
- 检查 `build.py` 中的工具链路径是否匹配你的安装。

### 7.3 `Failed to connect to ESP32-C6`

- 确认 USB 线连接的是数据口。
- 确认设备管理器中显示的 COM 口号，并修改 `build_and_flash.ps1` 或命令行中的端口。
- 按住 BOOT 键再按 RESET 键，让芯片进入下载模式。

---

## 8. 文件变更记录

- 2026-06-14：修复 ST7789 初始化（颜色反转、BGR、SPI 时钟、镜像）；整理 `build.py` / `build_and_flash.ps1`；新增本文档。
