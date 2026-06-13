#!/usr/bin/env python3
"""
build.py — ESP-IDF 构建/烧录入口脚本

用途：
  在 Windows + Git Bash / CMD / PowerShell 下都能调用 idf.py，
  无需手动激活 ESP-IDF 的 PowerShell/CMD 环境。

依赖：
  - ESP-IDF v6.0.1 安装在 C:/esp/v6.0.1/esp-idf
  - Python 虚拟环境在 C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env
  - 工具链、cmake、ninja 路径见下方 PATH 配置

用法：
  1. 仅编译：
       python build.py
       python build.py build

  2. 编译并烧录（默认端口 COM10）：
       python build.py -p COM10 flash

  3. 清理后重新编译：
       python build.py fullclean
       python build.py build

  4. 串口监控：
       python build.py -p COM10 monitor

  5. 编译 + 烧录 + 监控：
       python build.py -p COM10 flash monitor

注意：
  - 请用 ESP-IDF 虚拟环境里的 python.exe 运行此脚本，例如：
      C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe build.py
    否则会出现 "No module named 'click'" 等依赖错误。
  - 如果你的 ESP-IDF 安装路径不同，请修改下面的 IDF_PATH / IDF_PYTHON_ENV_PATH。
  - 如果目标芯片不是 esp32c6，请确保 sdkconfig 里 CONFIG_IDF_TARGET 正确，
    或在命令前加环境变量 IDF_TARGET=esp32c6。
"""
import os
import sys

# ---------------------------------------------------------------------------
# 1. 清理会与 ESP-IDF export 脚本冲突的环境变量
# ---------------------------------------------------------------------------
# 在 MSYS/Git Bash 中运行 PowerShell export.ps1 会检测到 MSys/Mingw 并拒绝。
# 直接导入 idf.py 运行时，保留 MSYSTEM 不会影响 CMake/编译器检测，
# 但保险起见仍将其移除。
os.environ.pop('MSYSTEM', None)

# ---------------------------------------------------------------------------
# 2. 设置 ESP-IDF 所需环境变量
# ---------------------------------------------------------------------------
os.environ['IDF_PATH'] = 'C:/esp/v6.0.1/esp-idf'
os.environ['ESP_IDF_VERSION'] = '6.0.1'
os.environ['IDF_PYTHON_ENV_PATH'] = 'C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env'
os.environ['PYTHON'] = 'C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe'

# ---------------------------------------------------------------------------
# 3. 把构建工具链加入 PATH
# ---------------------------------------------------------------------------
ninja_path = 'C:/Users/zr/.espressif/tools/ninja/1.12.1'
toolchain_path = 'C:/Users/zr/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin'
cmake_path = 'C:/Users/zr/.espressif/tools/cmake/3.30.5/bin'
paths_to_add = [ninja_path, toolchain_path, cmake_path]
current_path = os.environ.get('PATH', '')
for p in paths_to_add:
    if p not in current_path:
        current_path = p + ';' + current_path
os.environ['PATH'] = current_path

# ---------------------------------------------------------------------------
# 4. 切换到工程目录（脚本放在工程根目录下）
# ---------------------------------------------------------------------------
# 获取脚本所在目录，确保无论从哪运行都能定位到工程
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

# ---------------------------------------------------------------------------
# 5. 导入并运行 idf.py 的主入口
# ---------------------------------------------------------------------------
sys.path.insert(0, os.path.join(os.environ['IDF_PATH'], 'tools'))
from idf import main

if __name__ == '__main__':
    # 如果没有传参数，默认执行 build
    main(sys.argv[1:] if len(sys.argv) > 1 else ['build'])
