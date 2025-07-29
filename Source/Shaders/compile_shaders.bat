@echo off

setlocal enabledelayedexpansion

set GLSLC=glslc.exe

set SRC_DIR=src
set OUT_DIR=spv

if not exist %OUT_DIR% (
    mkdir %OUT_DIR%
)

echo === Compiling shaders ===
echo.

for %%f in (%SRC_DIR%\*.vert %SRC_DIR%\*.frag) do (
    set "SRC=%%f"
    set "FILENAME=%%~nxf"
    set "OUT=%OUT_DIR%\%%~nxf.spv"

    if exist "!OUT!" del /f /q "!OUT!"

    echo [COMPILE] !FILENAME! "---->" !OUT!
    %GLSLC% "!SRC!" -o "!OUT!"
    if errorlevel 1 (
        echo [ERR] Failed to compile !FILENAME!
    ) else (
        echo [OK ] Compiled !FILENAME!
    )
    echo.
)

echo === Done ===
pause