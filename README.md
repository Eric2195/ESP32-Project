# ESP32 Project Workspace

这是 ESP32/ESP32-C6 的统一工作区，所有相关项目和操作都在这里进行。

## 环境信息

| 项目 | 路径 |
|------|------|
| ESP-IDF | `C:\esp\v6.0.1\esp-idf` |
| 工具链 | `C:\Users\zr\.espressif` |
| Python 虚拟环境 | `C:\Users\zr\.espressif\python_env\idf6.0_py3.13_env` |
| 当前芯片目标 | **ESP32-C6** |
| 默认串口 | **COM6** |

## 项目目录

```
projects/
└── hello_c6/          # 基础验证项目（已跑通）
```

## 快速开始

1. 双击 `ESP32 Project.code-workspace` 用 VSCode 打开
2. 进入 `projects/hello_c6`
3. 底部状态栏点击 🔨 Build → ➡️ Flash → 🔍 Monitor

## 新建项目

在 `projects/` 下创建新文件夹，按 ESP-IDF 项目结构：
```
projects/
└── your_project/
    ├── CMakeLists.txt
    ├── main/
    │   ├── CMakeLists.txt
    │   └── main.c
    └── sdkconfig
```

## 串口变了怎么办？

如果板子插到别的 USB 口，在 VSCode 设置里改：
```json
"idf.port": "COMx"
```

## 注意

- **不要在 MSYS/Git Bash 里直接跑 `idf.py`**，会报 "MSys/Mingw is no longer supported"
- 用 VSCode 的 PowerShell 终端，或者点状态栏按钮
