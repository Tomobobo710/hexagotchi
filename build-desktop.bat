@echo off
REM Build the desktop target (build\desktop\game.exe).
REM
REM Runs build-alt.sh via MSYS2 bash instead of `make`. The Makefile runs the
REM native MinGW compiler through msys-bash recipes, which fail with "Cannot
REM create temporary file in C:\WINDOWS"; the shell script builds directly and
REM works. Same MSYS2 UCRT64 toolchain, same output. `make` still exists for
REM anyone whose environment handles it.
REM
REM   build-desktop.bat          build
REM   build-desktop.bat clean    remove build\desktop
C:\msys64\usr\bin\bash.exe -lc "cd \"$(cygpath '%CD%')\" && bash build-alt.sh %~1"
