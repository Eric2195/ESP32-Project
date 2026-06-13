Set-Location "C:\Users\zr\Desktop\ESP32 Project\projects\clawd_mochi"
& "C:\esp\v6.0.1\esp-idf\export.ps1"
idf.py build
if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful! Flashing..."
    idf.py -p COM10 flash
}
