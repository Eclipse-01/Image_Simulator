/*
 * FakeIPSScreen.c - 模拟IPS屏幕显示 (重写版)
 * This file is encoded in UTF-8
 * * 提供了使用Windows GDI API在窗口中显示图像的功能。
 * - ips200_show: 显示一张完整的uint8灰度图像。
 * - ips200_draw_point: 在当前显示的图像上绘制一个RGB565格式的彩色点。
 * - ips200_refresh: 手动刷新屏幕以显示更新。
 * * 内部使用一个32位的DIB（设备无关位图）来存储图像数据，从而支持彩色显示。
 * 传入的灰度图会被转换为32位格式进行显示。
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// 窗口相关全局变量
static HWND g_hwnd = NULL;
static HDC g_hdc = NULL;
static HBITMAP g_hBitmap = NULL;
static BITMAPINFO* g_bmi = NULL;
// g_display_buffer指向由CreateDIBSection创建的位图内存，格式为32-bit BGR
static uint8_t* g_display_buffer = NULL; 
static int g_window_created = 0;
static int g_current_width = 0;
static int g_current_height = 0;

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (g_display_buffer && g_hBitmap) {
                // 创建内存DC
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, g_hBitmap);
                
                // 计算显示尺寸（放大2倍显示以获得更好的视觉效果）
                int display_width = g_current_width * 2;
                int display_height = g_current_height * 2;
                
                // 将图像拉伸显示到窗口
                StretchBlt(hdc, 0, 0, display_width, display_height,
                           memDC, 0, 0, g_current_width, g_current_height, SRCCOPY);
                
                // 在窗口标题栏显示图像信息
                char title[100];
                sprintf(title, "IPS Simulator - %dx%d (32-bit Color)", g_current_width, g_current_height);
                SetWindowTextA(hwnd, title);
                
                SelectObject(memDC, oldBitmap);
                DeleteDC(memDC);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_CLOSE:
            // 不直接关闭窗口，而是隐藏，以便下次调用时可以快速显示
            ShowWindow(hwnd, SW_HIDE);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 初始化或调整显示窗口和位图的大小
static int init_display_window(int width, int height) {
    // 如果窗口已经创建且尺寸未变，则无需重新创建
    if (g_window_created && g_current_width == width && g_current_height == height) {
        ShowWindow(g_hwnd, SW_SHOW);
        return 1;
    }
    
    // 清理旧的GDI资源
    if (g_hBitmap) {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }
    if (g_bmi) {
        free(g_bmi);
        g_bmi = NULL;
    }
    
    // 如果窗口尚未创建，则创建新窗口
    if (!g_window_created) {
        WNDCLASSA wc = {0};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "FakeIPSScreen";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        
        if (!RegisterClassA(&wc)) {
            return 0;
        }
        
        // 根据图像尺寸计算窗口大小（放大2倍并增加边框）
        int display_width = width * 2;
        int display_height = height * 2;
        RECT window_rect = {0, 0, display_width, display_height};
        AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

        g_hwnd = CreateWindowExA(0, "FakeIPSScreen", "IPS Simulator",
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 window_rect.right - window_rect.left, 
                                 window_rect.bottom - window_rect.top,
                                 NULL, NULL, GetModuleHandle(NULL), NULL);
        
        if (!g_hwnd) {
            return 0;
        }
        
        g_hdc = GetDC(g_hwnd);
        g_window_created = 1;
    }

    // 调整现有窗口的大小
    int display_width = width * 2;
    int display_height = height * 2;
    RECT window_rect = {0, 0, display_width, display_height};
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(g_hwnd, NULL, 0, 0, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, SWP_NOMOVE | SWP_NOZORDER);

    // 创建32位色深的位图信息结构
    g_bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    if (!g_bmi) {
        return 0;
    }
    
    memset(g_bmi, 0, sizeof(BITMAPINFOHEADER));
    g_bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_bmi->bmiHeader.biWidth = width;
    g_bmi->bmiHeader.biHeight = -height; // 负值表示图像是自顶向下的（标准图像格式）
    g_bmi->bmiHeader.biPlanes = 1;
    g_bmi->bmiHeader.biBitCount = 32;    // 32位色深 (B, G, R, A)
    g_bmi->bmiHeader.biCompression = BI_RGB;
    
    // 创建一个DIB Section，这样我们可以直接访问其像素数据
    g_hBitmap = CreateDIBSection(g_hdc, g_bmi, DIB_RGB_COLORS, 
                                 (void**)&g_display_buffer, NULL, 0);
    
    if (!g_hBitmap) {
        free(g_bmi);
        g_bmi = NULL;
        return 0;
    }
    
    g_current_width = width;
    g_current_height = height;

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    
    return 1;
}

// 刷新窗口显示，并处理Windows消息
void ips200_refresh(void) {
    if (!g_hwnd || !g_window_created) return;

    // 强制重绘整个窗口
    InvalidateRect(g_hwnd, NULL, FALSE);
    UpdateWindow(g_hwnd);
    
    // 处理消息队列，保持窗口响应
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_window_created = 0; // 标记窗口已关闭
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


// 显示一张完整的灰度图
void ips200_show(uint8_t* image_data, int width, int height) {
    if (!image_data || width <= 0 || height <= 0) {
        return; // 无效参数
    }
    
    // 初始化或更新显示窗口
    if (!init_display_window(width, height)) {
        fprintf(stderr, "Error: Failed to initialize display window.\n");
        return;
    }
    
    // 将传入的8位灰度图像数据复制并转换为32位BGR格式
    if (g_display_buffer) {
        uint32_t* dest_pixel = (uint32_t*)g_display_buffer;
        int pixel_count = width * height;
        for (int i = 0; i < pixel_count; i++) {
            uint8_t gray_value = image_data[i];
            // 构造 0x00RRGGBB 格式的像素，对于灰度图 R=G=B
            dest_pixel[i] = (gray_value << 16) | (gray_value << 8) | gray_value;
        }
        
        // 刷新屏幕
        ips200_refresh();
    }
}

// 在指定坐标绘制一个RGB565格式的彩色点
void ips200_draw_point(int x, int y, uint16_t color_rgb565) {
    // 检查坐标是否在有效范围内
    if (!g_display_buffer || x < 0 || x >= g_current_width || y < 0 || y >= g_current_height) {
        return;
    }

    // 从RGB565格式中提取R, G, B分量 (5-6-5 bits)
    uint8_t r5 = (color_rgb565 >> 11) & 0x1F;
    uint8_t g6 = (color_rgb565 >> 5) & 0x3F;
    uint8_t b5 = color_rgb565 & 0x1F;

    // 将5位和6位的颜色分量扩展到8位 (0-255)
    // (val * 255) / max_val 是一种精确的扩展方式
    uint8_t r8 = (r5 * 255 + 15) / 31;
    uint8_t g8 = (g6 * 255 + 31) / 63;
    uint8_t b8 = (b5 * 255 + 15) / 31;

    // 将8位的RGB分量合成为一个32位的BGR像素值
    uint32_t pixel32 = (b8) | (g8 << 8) | (r8 << 16);

    // 计算目标像素在缓冲区中的位置并写入颜色值
    uint32_t* dest_pixel = (uint32_t*)g_display_buffer;
    dest_pixel[y * g_current_width + x] = pixel32;
}


// 清理所有资源
void ips200_cleanup(void) {
    if (g_hBitmap) {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }
    if (g_hdc && g_hwnd) {
        ReleaseDC(g_hwnd, g_hdc);
        g_hdc = NULL;
    }
    if (g_hwnd) {
        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }
    if (g_bmi) {
        free(g_bmi);
        g_bmi = NULL;
    }
    g_display_buffer = NULL; // 由DeleteObject(g_hBitmap)管理，只需置空
    g_window_created = 0;
    g_current_width = 0;
    g_current_height = 0;

    UnregisterClassA("FakeIPSScreen", GetModuleHandle(NULL));
}

// 检查窗口是否仍然打开并可见
int ips200_is_window_open(void) {
    // 处理一次消息循环，以更新窗口状态
    MSG msg;
    if (PeekMessage(&msg, g_hwnd, 0, 0, PM_NOREMOVE)) {
        ips200_refresh();
    }
    return g_window_created && IsWindow(g_hwnd) && IsWindowVisible(g_hwnd);
}
