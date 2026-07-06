@echo off
REM Build the desktop target. Uses `make` from your normal cmd PATH.
REM   build-desktop.bat          build build\desktop\game.exe
REM   build-desktop.bat clean    remove build\desktop
if /i "%~1"=="clean" (
    make clean
) else (
    make
)
