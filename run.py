# -*- coding: utf-8 -*-
import os
import glob
import subprocess
import sys
import shutil
import importlib

# --- 配置区 ---
REQUIRED_PACKAGES = ['websockets']
COMPILER = 'gcc'
OUTPUT_EXE = 'main.exe'
LINKER_FLAGS = ['-lgdi32', '-luser32']


def check_gcc():
    """检查系统中是否存在 GCC 编译器"""
    if shutil.which(COMPILER):
        print(f"[OK] 编译器 '{COMPILER}' 已找到。")
        return True
    else:
        print(f"[ERROR] 在系统的 PATH 环境变量中找不到编译器 '{COMPILER}'。")
        print("\n--- 安装指引 ---")
        print("这个项目需要 GCC 编译器来编译 C 语言源代码。")
        print("对于 Windows 用户，推荐安装 MinGW-w64：")
        print("1. 访问 w64devkit 发行版页面：https://github.com/skeeto/w64devkit/releases")
        print("2. 下载最新的 'w64devkit-x.x.x.zip' 文件。")
        print("3. 解压到一个你喜欢的位置（例如 C:\\w64devkit）。")
        print("4. 将解压后文件夹里的 'bin' 目录添加到系统的 PATH 环境变量中。")
        print("   (例如，将 'C:\\w64devkit\\bin' 添加到 PATH)")
        print("5. 重启你的命令行终端，然后重新运行此脚本。")
        return False

def check_and_install_packages():
    """检查并安装所需的 Python 库，并捕获特定的代理错误"""
    print("\n--- 正在检查所需的 Python 库 ---")
    all_good = True
    for package in REQUIRED_PACKAGES:
        try:
            importlib.import_module(package)
            print(f"[OK] 库 '{package}' 已安装。")
        except ImportError:
            print(f"[WARNING] 库 '{package}' 未找到，正在尝试自动安装...")
            try:
                pip_command = [
                    sys.executable, '-m', 'pip', 'install', package,
                    '--trusted-host', 'pypi.org',
                    '--trusted-host', 'files.pythonhosted.org'
                ]
                subprocess.check_call(pip_command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                print(f"[OK] 成功安装库 '{package}'。")
            except subprocess.CalledProcessError:
                result = subprocess.run(pip_command, capture_output=True, text=True, encoding='utf-8', errors='replace')
                stderr_text = result.stderr
                if "check_hostname requires server_hostname" in stderr_text:
                    print(f"[ERROR] 安装 '{package}' 失败，检测到网络代理问题。")
                    print("   请暂时关闭 Clash、V2Ray 或其他系统代理软件，然后重试。")
                else:
                    print(f"[ERROR] 自动安装 '{package}' 失败，pip 返回了以下错误：")
                    print("-------------------- pip error --------------------")
                    print(stderr_text)
                    print("-------------------------------------------------")
                all_good = False
    return all_good


def main():
    """主函数，执行检查、编译和运行的整个流程"""
    if not check_gcc() or not check_and_install_packages():
        return

    source_files = glob.glob('*.c')
    if not source_files:
        print("\n[ERROR] 在当前目录下没有找到任何 .c 源文件。")
        return

    print(f"\n[INFO] 找到了以下源文件: {', '.join(source_files)}")

    compile_command = [COMPILER] + source_files + ['-o', OUTPUT_EXE] + LINKER_FLAGS
    print("\n--- 准备执行编译命令 ---")
    print(' '.join(compile_command))
    print("--------------------------\n")
    try:
        result = subprocess.run(
            compile_command, capture_output=True, text=True, check=True, encoding='utf-8', errors='replace'
        )
        if result.stdout:
            print("[INFO] 编译器信息:\n" + result.stdout)
        print(f"[OK] 编译成功！已生成可执行文件: {OUTPUT_EXE}")
    except subprocess.CalledProcessError as e:
        print("[ERROR] !!! 编译失败 !!!")
        print("编译器返回了以下错误信息:")
        print(e.stderr)
        return

    print(f"\n--- 准备运行程序 ---")
    try:
        # !!! 关键修复 !!!
        # 使用 sys.executable 来确保调用的是当前正在运行的这个Python解释器
        python_executable = sys.executable
        
        print(f"执行: {python_executable} bridge.py | .\\{OUTPUT_EXE}")
        print("----------------------\n")
        
        # 使用这个明确的路径来启动 bridge.py
        python_process = subprocess.Popen([python_executable, 'bridge.py'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        main_process = subprocess.Popen([f'.\\{OUTPUT_EXE}'], stdin=python_process.stdout)
        python_process.stdout.close()
        
        stderr_output = python_process.stderr.read().decode('utf-8', errors='replace')
        if stderr_output:
            print("[bridge.py 的信息]:\n", stderr_output)

        main_process.wait()
        print("\n--- 程序运行结束 ---")
    except Exception as e:
        print(f"[ERROR] 程序运行时发生未知错误: {e}")


if __name__ == "__main__":
    main()
