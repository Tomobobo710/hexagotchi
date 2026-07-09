@echo off
REM Build the standalone scene layout editor (tools/scene_editor.cpp) to
REM build\devtools\scene_editor.exe. Not part of the game -- a design tool
REM for visually placing actors and exporting their positions to JSON.
REM   scene-editor.bat          build build\devtools\scene_editor.exe
make scene-editor
