#!/usr/bin/env python

import asyncio
import websockets
import sys

# --- 配置信息 ---
HOST = "0.0.0.0"  # 监听所有网络接口
PORT = 8765        # 与C代码中相同的端口


CONSUME_INTERVAL = 0.05 # 消费帧的间隔，0.05秒 = 20 FPS

async def handler(websocket: websockets.WebSocketServerProtocol):
    """
    处理单个WebSocket连接。
    采用生产者-消费者模式，解决高帧率输入导致的延迟累积问题。
    """
    print(f"客户端已连接: {websocket.remote_address}", file=sys.stderr)
    
    # 用于在生产者和消费者之间传递最新帧的共享变量
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
            # 等待一个固定的时间间隔
            await asyncio.sleep(CONSUME_INTERVAL)
            
            # 从共享变量中取走（消费）最新的一帧
            frame_to_process = latest_frame
            latest_frame = None # 取走后清空，避免重复处理

            if frame_to_process:
                try:
                    # 将取到的最新帧写入标准输出
                    sys.stdout.buffer.write(frame_to_process)
                    sys.stdout.flush()
                    # print(f"已转发一帧 ({len(frame_to_process)}字节)", file=sys.stderr)
                except Exception as e:
                    print(f"写入stdout失败: {e}", file=sys.stderr)
                    # 如果写入失败（比如C程序关闭了），消费者也应该停止
                    break
    
    try:
        # 当客户端连接时，并发启动生产者和消费者任务
        consumer_task = asyncio.create_task(consumer())
        # producer()会一直运行，直到客户端断开连接
        await producer()
    except websockets.exceptions.ConnectionClosed as e:
        print(f"连接已关闭: {e}", file=sys.stderr)
    except Exception as e:
        print(f"发生未知错误: {e}", file=sys.stderr)
    finally:
        # 当连接结束时，确保消费者任务也被取消和清理
        if consumer_task:
            consumer_task.cancel()
        print("客户端断开连接，处理任务已停止。", file=sys.stderr)

async def main():
    """
    启动WebSocket服务器
    """
    # 获取本机IP用于显示，实际监听的是0.0.0.0
    import socket
    hostname = socket.gethostname()
    ip_address = socket.gethostbyname(hostname)
    
    print(f"Python WebSocket中继服务器已启动", file=sys.stderr)
    print(f" - 监听地址: ws://0.0.0.0:{PORT} (所有网络接口)", file=sys.stderr)
    print(f" - 可尝试连接: ws://{ip_address}:{PORT} 或 ws://localhost:{PORT}", file=sys.stderr)
    print("等待网页端连接...", file=sys.stderr)
    
    async with websockets.serve(handler, HOST, PORT):
        await asyncio.Future()  # 永远运行

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n服务器被手动关闭。", file=sys.stderr)