#ifndef ZF_COMMON_HEADFILE_H
#define ZF_COMMON_HEADFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <io.h>      // for _setmode
#include <fcntl.h>   // for _O_BINARY
#include <stdint.h>
#include <stdarg.h> // for va_list, va_start, va_end
#include <string.h>
#include <errno.h>
#include <windows.h>           // Win32 GDI API

typedef unsigned char      uint8;
typedef signed char        int8;
typedef unsigned short     uint16;
typedef signed short       int16;
typedef unsigned int       uint32;
typedef signed int         int32;

// --- 配置信息 ---
#define IMAGE_WIDTH 188
#define IMAGE_HEIGHT 120
#define FRAME_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT)

// 全局图像缓冲区，替代原来的mt9v03x_image
uint8 mt9v03x_image[IMAGE_HEIGHT][IMAGE_WIDTH];

// --- myprint 宏定义 ---
#define myprint(fmt, ...) \
    do { \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)


/**
 * 显示图像到IPS屏幕模拟器窗口
 * @param image_data 图像数据指针，uint8_t格式的灰度图像
 * @param width 图像宽度
 * @param height 图像高度
 */
void ips200_show(uint8_t* image_data, int width, int height);

/**
 * 清理所有资源并关闭窗口
 */
void ips200_cleanup(void);

/**
 * 检查窗口是否仍然打开
 * @return 1 如果窗口打开，0 如果窗口关闭
 */
int ips200_is_window_open(void);

#endif // ZF_COMMON_HEADFILE_H