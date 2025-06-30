@echo off
REM Windows GGML Visualizer Injection Script
REM Usage: inject_windows.bat <command> [args...]

setlocal enabledelayedexpansion

REM Get the directory of this script
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "BUILD_DIR=%PROJECT_ROOT%\build"

REM Check if the shared library exists
set "SHARED_LIB=%BUILD_DIR%\src\ggml_viz_hook.dll"
if not exist "%SHARED_LIB%" (
    echo Error: GGML Visualizer shared library not found at %SHARED_LIB%
    echo Please build the project first:
    echo   cd "%PROJECT_ROOT%" ^&^& mkdir build ^&^& cd build
    echo   cmake .. -DCMAKE_BUILD_TYPE=Release ^&^& cmake --build . --config Release
    exit /b 1
)

REM Check if output file is specified
if "%GGML_VIZ_OUTPUT%"=="" (
    REM Default output filename with timestamp
    for /f "tokens=2 delims==" %%i in ('wmic OS Get localdatetime /value') do set datetime=%%i
    set "TIMESTAMP=!datetime:~0,8!_!datetime:~8,6!"
    set "GGML_VIZ_OUTPUT=ggml_trace_!TIMESTAMP!.ggmlviz"
    echo GGML_VIZ_OUTPUT not set, using: !GGML_VIZ_OUTPUT!
)

REM Check if we have a command to run
if "%1"=="" (
    echo Usage: %0 ^<command^> [args...]
    echo.
    echo Examples:
    echo   %0 llama.cpp\main.exe -m model.gguf -p "Hello, world!"
    echo   %0 whisper.cpp\main.exe -m model.bin -f audio.wav
    echo.
    echo Environment variables:
    echo   GGML_VIZ_OUTPUT          - Output trace file ^(default: auto-generated^)
    echo   GGML_VIZ_VERBOSE         - Enable verbose logging ^(1/0^)
    echo   GGML_VIZ_OP_TIMING       - Enable operation timing ^(default: 1^)
    echo   GGML_VIZ_MEMORY_TRACKING - Enable memory tracking ^(default: 0^)
    echo   GGML_VIZ_TENSOR_NAMES    - Enable tensor names ^(default: 1^)
    echo   GGML_VIZ_MAX_EVENTS      - Maximum events to record ^(default: 10000000^)
    echo   GGML_VIZ_DISABLE         - Disable instrumentation ^(1/0^)
    exit /b 1
)

REM Set default environment variables if not specified
if "%GGML_VIZ_VERBOSE%"=="" set "GGML_VIZ_VERBOSE=0"
if "%GGML_VIZ_OP_TIMING%"=="" set "GGML_VIZ_OP_TIMING=1"
if "%GGML_VIZ_MEMORY_TRACKING%"=="" set "GGML_VIZ_MEMORY_TRACKING=0"
if "%GGML_VIZ_TENSOR_NAMES%"=="" set "GGML_VIZ_TENSOR_NAMES=1"
if "%GGML_VIZ_MAX_EVENTS%"=="" set "GGML_VIZ_MAX_EVENTS=10000000"

echo ==============================================
echo 🚀 GGML Visualizer Injection (Windows)
echo ==============================================
echo Library: %SHARED_LIB%
echo Output:  %GGML_VIZ_OUTPUT%
echo Command: %*
echo.

REM Show configuration
if "%GGML_VIZ_VERBOSE%"=="1" (
    echo Configuration:
    echo   Op timing: %GGML_VIZ_OP_TIMING%
    echo   Memory tracking: %GGML_VIZ_MEMORY_TRACKING%
    echo   Tensor names: %GGML_VIZ_TENSOR_NAMES%
    echo   Max events: %GGML_VIZ_MAX_EVENTS%
    echo.
)

REM Run the command with library injection
echo Starting application with GGML instrumentation...
echo.

REM Note: On Windows, DLL injection is more complex and typically requires
REM either modifying the executable or using specialized injection tools.
REM For now, we'll just set the environment and run the command.
REM The DLL should be in the same directory as the executable or in PATH.

REM Add the library directory to PATH so the DLL can be found
set "PATH=%BUILD_DIR%\src;%PATH%"

REM Execute the command
%*