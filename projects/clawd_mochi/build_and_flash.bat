@echo off
setlocal
set MSYSTEM=
cd /d "C:\Users\zr\Desktop\ESP32 Project\projects\clawd_mochi"
call C:\esp\v6.0.1\esp-idf\export.bat
idf.py build
if %errorlevel% equ 0 (
    echo Build successful! Flashing...
    idf.py -p COM10 flash
)
pause
