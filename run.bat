@echo off
REM 自动查找并编译所有 .c 文件，包含所有头文件
setlocal enabledelayedexpansion

REM 设置输出文件名
set OUT=main.exe

REM 查找所有 .c 文件
set SRC=
for %%f in (*.c) do set SRC=!SRC! %%f

REM 编译命令，自动包含所有头文件

gcc !SRC! -o %OUT% -lgdi32 -luser32 

if %errorlevel% neq 0 (
    echo Compilation failed with error code %errorlevel%.
    exit /b 1
) else (
    echo Compilation succeeded, generated %OUT%
)

REM 运行生成的可执行文件
python bridge.py | %OUT%