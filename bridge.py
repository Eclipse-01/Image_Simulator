# -*- coding: utf-8 -*-
import asyncio
import websockets
import sys
import socket

# --- 配置区 ---
HOST = "0.0.0.0"  # 监听所有网络接口
PORT = 8765       # 监听的端口
CONSUME_INTERVAL = 0.05 # 消费帧的间隔 (0.05秒 = 20 FPS)

async def handler(websocket: websockets.WebSocketServerProtocol):
    """
    处理单个WebSocket连接。
    采用生产者-消费者模式，解决高帧率输入导致的延迟累积问题。
    """
    sys.stderr.write(f"[bridge.py INFO] 客户端已连接: {websocket.remote_address}\n")
    
    latest_frame = None
    consumer_task = None

    async def producer():
        """生产者：以最快速度接收数据，并更新latest_frame变量。"""
        nonlocal latest_frame
        async for message in websocket:
            latest_frame = message  # 不断用最新帧覆盖旧帧

    async def consumer():
        """消费者：以固定频率处理latest_frame中的最新数据。"""
        nonlocal latest_frame
        while True:
            await asyncio.sleep(CONSUME_INTERVAL)
            
            frame_to_process = latest_frame
            latest_frame = None # 取走后清空，避免重复处理

            if frame_to_process:
                try:
                    sys.stdout.buffer.write(frame_to_process)
                    sys.stdout.flush()
                except (IOError, BrokenPipeError):
                    sys.stderr.write("[bridge.py INFO] C程序关闭了管道，停止转发数据。\n")
                    break
                except Exception as e:
                    sys.stderr.write(f"[bridge.py ERROR] 写入stdout失败: {e}\n")
                    break
    
    try:
        consumer_task = asyncio.create_task(consumer())
        await producer()
    except websockets.exceptions.ConnectionClosed as e:
        sys.stderr.write(f"[bridge.py INFO] 网页端连接已关闭: {e}\n")
    except Exception as e:
        sys.stderr.write(f"[bridge.py ERROR] 处理连接时发生未知错误: {e}\n")
    finally:
        if consumer_task:
            consumer_task.cancel()
            try:
                await consumer_task # 等待任务真正取消
            except asyncio.CancelledError:
                pass # 捕获取消错误是正常的
        sys.stderr.write("[bridge.py INFO] 客户端断开连接，处理任务已停止。\n")

async def main():
    """
    启动WebSocket服务器并处理常见的网络错误
    """
    try:
        async with websockets.serve(handler, HOST, PORT):
            hostname = socket.gethostname()
            try:
                ip_address = socket.gethostbyname(hostname)
            except socket.gaierror:
                ip_address = "127.0.0.1" # 如果获取失败，回退到localhost

            sys.stderr.write("--- Python WebSocket Bridge Started ---\n")
            sys.stderr.write(f"- 监听地址: ws://0.0.0.0:{PORT} (所有网络接口)\n")
            sys.stderr.write(f"- 可尝试连接: ws://{ip_address}:{PORT} 或 ws://localhost:{PORT}\n")
            sys.stderr.write("--- 等待网页端连接... ---\n")
            sys.stderr.write("[HINT] 如果网页端无法连接，请尝试暂时关闭系统代理（如Clash）。\n")
            sys.stderr.flush()
            
            await asyncio.Future()  # 永久运行

    except OSError as e:
        if hasattr(e, 'winerror'):
            if e.winerror == 10048:  # 端口已被占用
                sys.stderr.write("\n[bridge.py FATAL ERROR] 启动服务器失败：端口已被占用！\n")
                sys.stderr.write(f"  端口 {PORT} 正在被另一个程序使用。\n")
                sys.stderr.write("  请检查任务管理器中是否已有 'python.exe' 或 'bridge.py' 进程在运行，并将其关闭。\n")
            elif e.winerror == 10013: # 权限不足
                sys.stderr.write("\n[bridge.py FATAL ERROR] 启动服务器失败：权限不足！\n")
                sys.stderr.write("  请检查你的防火墙或杀毒软件，确保它们没有阻止此脚本进行网络通信。\n")
            else:
                sys.stderr.write(f"\n[bridge.py FATAL ERROR] 启动服务器时发生未知的网络错误: {e}\n")
        else:
            sys.stderr.write(f"\n[bridge.py FATAL ERROR] 启动服务器时发生未知的网络错误: {e}\n")
            
    except Exception as e:
        sys.stderr.write(f"\n[bridge.py FATAL ERROR] 发生未知错误: {e}\n")

if __name__ == "__main__":
    if sys.platform == "win32":
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        sys.stderr.write("\n--- WebSocket Bridge 被手动关闭 ---\n")

