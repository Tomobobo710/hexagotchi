@echo off
REM Build the standalone scene layout editor (tools/scene_editor.cpp) to
REM build\scene_editor\scene_editor.exe. Not part of the game -- a design tool
REM for visually placing actors and exporting their positions to JSON.
REM
REM Runs build-scene-editor.sh via MSYS2 bash instead of `make scene-editor`
REM (which fails on Windows -- see build-desktop.bat).
REM   scene-editor.bat          build
REM   scene-editor.bat clean    remove build\scene_editor
C:\msys64\usr\bin\bash.exe -lc "cd \"$(cygpath '%CD%')\" && bash build-scene-editor.sh %~1"
