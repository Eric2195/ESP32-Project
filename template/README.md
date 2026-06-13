# ESP32 项目模板

这是 ESP32-C6 的最小项目模板，复制后改个名就能直接开发。

## 文件说明

```
.
├── CMakeLists.txt              # 项目根配置，改 project() 里的名字
├── main/
│   ├── CMakeLists.txt          # main 组件配置，一般不用改
│   └── main.c                  # 你的代码入口 app_main()
└── .vscode/
    ├── settings.json           # ESP-IDF 扩展配置（串口、路径等）
    └── c_cpp_properties.json   # C/C++ IntelliSense 配置（消除红波浪线）
```

## 使用步骤

1. **复制模板**
   ```
   复制 template/ 文件夹 → 粘贴到 projects/ 下 → 改名（如 wifi_test）
   ```

2. **改项目名**
   打开 `CMakeLists.txt`，把最后一行改成：
   ```cmake
   project(wifi_test)
   ```

3. **写代码**
   在 `main/main.c` 里的 `app_main()` 函数中写你的逻辑。

4. **第一次编译**
   - VSCode 底部状态栏点 🔨 **Build**
   - 或者按 `Ctrl+E B`
   - 第一次会自动生成 `sdkconfig` 和 `build/`

5. **烧录 + 监控**
   - 点 ➡️ **Flash** 烧录
   - 点 🔍 **Monitor** 看串口输出
   - 或者直接点 🔧 **Build, Flash and Monitor** 一键三连

## 注意事项

- **不要删除 `build/` 里的 `compile_commands.json`**，否则 C/C++ 扩展的红波浪线会回来
- **串口变了**（如从 COM6 变成 COM5）：改 `.vscode/settings.json` 里的 `"idf.port": "COM5"`
- **换芯片**（如从 C6 换成 S3）：`Ctrl+Shift+P` → `ESP-IDF: Set Espressif Device Target`
- **头文件报错**：`Ctrl+Shift+P` → `C/C++: Rescan Workspace`

## 示例：创建一个叫 "blink" 的项目

```bash
# 1. 复制模板
cp -r template projects/blink

# 2. 改项目名
# CMakeLists.txt: project(blink)

# 3. 改代码
# main/main.c 里写 LED 闪烁逻辑

# 4. Build + Flash + Monitor
```
