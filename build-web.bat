@echo off
REM Build the web target. Uses `make` from your normal cmd PATH.
REM   build-web.bat          build build\web\index.html
REM   build-web.bat clean    remove build\web
if /i "%~1"=="clean" (
    make clean-web
) else (
    make web
)
