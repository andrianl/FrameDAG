@echo off
REM
premake5.exe vs2022
if %errorlevel% neq 0 (
    echo Premake5 not found. Please download it from https://premake.github.io/
    pause
) else (
    echo Project files generated for Visual Studio 2022.
    pause
)