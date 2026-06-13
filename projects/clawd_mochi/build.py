#!/usr/bin/env python
import os
import sys

# Remove MSYSTEM to avoid MSys/Mingw check
os.environ.pop('MSYSTEM', None)

# Set required environment variables
os.environ['IDF_PATH'] = 'C:/esp/v6.0.1/esp-idf'
os.environ['ESP_IDF_VERSION'] = '6.0.1'
os.environ['IDF_PYTHON_ENV_PATH'] = 'C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env'
os.environ['PYTHON'] = 'C:/Users/zr/.espressif/python_env/idf6.0_py3.13_env/Scripts/python.exe'

# Add tools to PATH
ninja_path = 'C:/Users/zr/.espressif/tools/ninja/1.12.1'
toolchain_path = 'C:/Users/zr/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin'
cmake_path = 'C:/Users/zr/.espressif/tools/cmake/3.30.5/bin'
paths_to_add = [ninja_path, toolchain_path, cmake_path]
current_path = os.environ.get('PATH', '')
for p in paths_to_add:
    if p not in current_path:
        current_path = p + ';' + current_path
os.environ['PATH'] = current_path

# Change to project directory
os.chdir('C:/Users/zr/Desktop/ESP32 Project/projects/clawd_mochi')

# Add idf.py tools to path
sys.path.insert(0, 'C:/esp/v6.0.1/esp-idf/tools')

# Import and run main
from idf import main
main(sys.argv[1:] if len(sys.argv) > 1 else ['build'])
