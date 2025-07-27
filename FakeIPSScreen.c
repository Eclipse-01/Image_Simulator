/*
 * FakeIPSScreen.c - 模拟IPS屏幕显示
 * 
 * 提供ips200_show函数，使用Windows GDI API在窗口中显示灰度图像
 * 支持显示任意尺寸的uint8格式图像数据
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
                
                // 计算显示尺寸（放大2倍显示）
                int display_width = g_current_width * 2;
                int display_height = g_current_height * 2;
                
                // 将图像拉伸显示到窗口
                StretchBlt(hdc, 0, 0, display_width, display_height,
                          memDC, 0, 0, g_current_width, g_current_height, SRCCOPY);
                
                // 在窗口标题栏显示图像信息
                char title[100];
                sprintf(title, "IPS Simulator - %dx%d", g_current_width, g_current_height);
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
            // 不关闭窗口，只是隐藏
            ShowWindow(hwnd, SW_HIDE);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 初始化显示窗口
static int init_display_window(int width, int height) {
    if (g_window_created && g_current_width == width && g_current_height == height) {
        return 1; // 窗口已存在且尺寸匹配，无需重新创建
    }
    
    // 清理旧资源
    if (g_hBitmap) {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }
    if (g_bmi) {
        free(g_bmi);
        g_bmi = NULL;
    }
    
    // 如果窗口不存在，创建新窗口
    if (!g_window_created) {
        // 注册窗口类
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
        
        // 计算窗口尺寸
        int display_width = width * 2;
        int display_height = height * 2;
        
        // 创建窗口
        g_hwnd = CreateWindowExA(0, "FakeIPSScreen", "IPS Simulator",
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               display_width + 20, display_height + 60,
                               NULL, NULL, GetModuleHandle(NULL), NULL);
        
        if (!g_hwnd) {
            return 0;
        }
        
        ShowWindow(g_hwnd, SW_SHOW);
        UpdateWindow(g_hwnd);
        
        // 获取设备上下文
        g_hdc = GetDC(g_hwnd);
        g_window_created = 1;
    }
    
    // 创建新的位图信息结构
    g_bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (!g_bmi) {
        return 0;
    }
    
    // 设置位图信息
    memset(g_bmi, 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    g_bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_bmi->bmiHeader.biWidth = width;
    g_bmi->bmiHeader.biHeight = -height; // 负值表示自顶向下
    g_bmi->bmiHeader.biPlanes = 1;
    g_bmi->bmiHeader.biBitCount = 8; // 8位灰度
    g_bmi->bmiHeader.biCompression = BI_RGB;
    
    // 设置灰度调色板
    for (int i = 0; i < 256; i++) {
        g_bmi->bmiColors[i].rgbRed = i;
        g_bmi->bmiColors[i].rgbGreen = i;
        g_bmi->bmiColors[i].rgbBlue = i;
        g_bmi->bmiColors[i].rgbReserved = 0;
    }
    
    // 创建DIB位图
    g_hBitmap = CreateDIBSection(g_hdc, g_bmi, DIB_RGB_COLORS, 
                                (void**)&g_display_buffer, NULL, 0);
    
    if (!g_hBitmap) {
        return 0;
    }
    
    g_current_width = width;
    g_current_height = height;
    
    return 1;
}

// 主要的显示函数
void ips200_show(uint8_t* image_data, int width, int height) {
    if (!image_data || width <= 0 || height <= 0) {
        return; // 参数无效
    }
    
    // 初始化或更新显示窗口
    if (!init_display_window(width, height)) {
        fprintf(stderr, "初始化显示窗口失败\n");
        return;
    }
    
    // 复制图像数据到显示缓冲区
    if (g_display_buffer) {
        memcpy(g_display_buffer, image_data, width * height);
        
        // 强制重绘窗口
        InvalidateRect(g_hwnd, NULL, FALSE);
        UpdateWindow(g_hwnd);
        
        // 处理Windows消息（非阻塞）
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// 清理资源函数
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
    g_window_created = 0;
    g_current_width = 0;
    g_current_height = 0;
    g_display_buffer = NULL;
}

// 检查窗口是否仍然存在
int ips200_is_window_open(void) {
    return g_window_created && IsWindow(g_hwnd) && IsWindowVisible(g_hwnd);
}