@echo off
REM Build the web target (build\web\index.html).
REM
REM Runs build-web.sh via MSYS2 bash instead of `make web`. The Makefile packs
REM assets with the native compiler through msys-bash, which fails with "Cannot
REM create temporary file in C:\WINDOWS"; the shell script builds directly and
REM works. Same emscripten toolchain, same output.
REM
REM   build-web.bat          build
REM   build-web.bat clean    remove build\web
C:\msys64\usr\bin\bash.exe -lc "cd \"$(cygpath '%CD%')\" && bash build-web.sh %~1"
