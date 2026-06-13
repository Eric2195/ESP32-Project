# Hello ESP32-C6

ESP-IDF 入门验证项目。等 ESP32-C6 开发板到手后，直接用这个项目点亮测试。

## 编译/烧录/监控 (等 ESP-IDF 配置完成后)

在 VSCode 底部状态栏点击：
- 🔨 编译 (Build)
- ➡️ 烧录 (Flash)
- 🔍 串口监控 (Monitor)

或使用快捷键：
- `Ctrl+E B` - Build
- `Ctrl+E F` - Flash
- `Ctrl+E M` - Monitor

## ESP32-C6 关键信息

- **架构**: RISC-V (不是 Xtensa!)
- **无线**: Wi-Fi 6 + 802.15.4 (Zigbee / Thread / Matter)
- **最低 IDF 版本**: v5.1+
- **目标名**: `esp32c6`
