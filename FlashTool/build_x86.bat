@echo off
setlocal
echo ==========================================================
echo EDwIC Flasher - Building 32-bit (x86) version
echo ==========================================================

:: Attempt to find 32-bit Python via the Windows Python Launcher
py -3-32 --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] 32-bit Python not found!
    echo Please install 32-bit Python from python.org
    echo During installation, make sure to check "Add Python to PATH".
    echo.
    pause
    exit /b 1
)

echo [1/3] Installing dependencies for 32-bit Python...
py -3-32 -m pip install --upgrade pip
py -3-32 -m pip install pyserial esptool pyinstaller

echo.
echo [2/3] Cleaning up old builds...
if exist build rmdir /S /Q build
if exist dist rmdir /S /Q dist

echo.
echo [3/3] Building 32-bit executable...
:: We use py -3-32 to ensure the 32-bit compiler/packager is used
py -3-32 -m PyInstaller --noconfirm --onefile --windowed ^
    --version-file "version.txt" ^
    --name "EDwIC_Flasher_x86" ^
    --add-data "..\driver;driver" ^
    --collect-data esptool ^
    --hidden-import esptool ^
    flasher.py

if %errorlevel% equ 0 (
    echo.
    echo ==========================================================
    echo SUCCESS! 32-bit executable created.
    echo Location: dist\EDwIC_Flasher_x86.exe
    echo This version will work on both 32-bit and 64-bit Windows.
    echo ==========================================================
) else (
    echo.
    echo [ERROR] Build failed. Check the logs above.
)

pause
endlocal
