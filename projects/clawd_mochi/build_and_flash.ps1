# build_and_flash.ps1 — 一键编译 + 烧录脚本
#
# 用法：
#   1. 在 PowerShell 中执行（推荐）：
#        .\build_and_flash.ps1
#
#   2. 如果提示执行策略限制，可先运行：
#        Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
#
# 说明：
#   - 此脚本直接调用 build.py，避免在 Git Bash/MSYS 中调用 export.ps1 失败的问题。
#   - 默认端口为 COM10。如果你的 ESP32 不是 COM10，请修改下面 $port 变量。
#   - build.py 会自动设置 ESP-IDF 环境变量、工具链路径，并调用 idf.py。
#
# 常用端口查看命令：
#   Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description

$venvPython = "C:\Users\zr\.espressif\python_env\idf6.0_py3.13_env\Scripts\python.exe"
$projectDir = "C:\Users\zr\Desktop\ESP32 Project\ESP32-Project\projects\clawd_mochi"
$port = "COM10"   # 根据设备管理器中的实际 COM 口修改

Set-Location $projectDir
Write-Host "Building project in $projectDir ..." -ForegroundColor Cyan

& $venvPython build.py build

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful! Flashing to $port ..." -ForegroundColor Green
    & $venvPython build.py -p $port flash
} else {
    Write-Host "Build failed. Flash aborted." -ForegroundColor Red
}
