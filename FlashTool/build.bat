@echo off
echo Installing dependencies...
py -m pip install pyserial esptool pyinstaller

echo.
echo Removing old builds if they exist...
if exist build rmdir /S /Q build
if exist dist rmdir /S /Q dist

echo.
echo Building executable...
py -m PyInstaller --noconfirm --onefile --windowed --version-file "version.txt" --name "EDwIC_Flasher" --add-data "..\driver;driver" --collect-data esptool --hidden-import esptool flasher.py

echo.
echo ==========================================================
echo DONE! 
echo Your executable is in the folder: dist\EDwIC_Flasher.exe
echo.
echo For the program to work, copy:
echo 1) firmware.bin
echo 2) littlefs.bin
echo Into the same folder as EDwIC_Flasher.exe
echo ==========================================================
pause
