/*
 * 图像数据流读取器 (从标准输入)
 *
 * 这个C程序不再需要任何网络库。它只是一个简单的工具，
 * 从标准输入流(stdin)读取固定大小的二进制数据块，
 * 这些数据由一个外部程序（如Python脚本）通过管道传入。
 */
#include "zf_common_headfile.h" // 确保类型定义可用


int main() {
    // 重要：在Windows上，必须将stdin设置为二进制模式，
    // 否则系统可能会错误地处理某些字节（如换行符）。
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        perror("无法将stdin设置为二进制模式");
        return 1;
    }

    // 分配一个缓冲区来存放一帧图像的数据
    unsigned char* buffer = (unsigned char*)malloc(FRAME_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }

    myprint("C程序已启动，准备从标准输入读取数据...\n");

    unsigned long long frame_counter = 0;
    size_t bytes_read;
/*----------------------------------在此处添加需要初始化的代码--------------------------------------------------------------*/


/*----------------------------------在此处添加需要初始化的代码--------------------------------------------------------------*/

    // 无限循环，持续从标准输入读取数据
    while ((bytes_read = fread(buffer, 1, FRAME_SIZE, stdin)) > 0) {
        if (bytes_read == FRAME_SIZE) {
            frame_counter++;

            // 将一维缓冲区数据复制到二维图像数组
            for (int i = 0; i < IMAGE_HEIGHT; i++) {
                for (int j = 0; j < IMAGE_WIDTH; j++) {
                    mt9v03x_image[i][j] = buffer[i * IMAGE_WIDTH + j];
                }
            }
/*----------------------------------在此处添加需要循环运行的代码--------------------------------------------------------------*/


/*----------------------------------在此处添加需要循环运行的代码--------------------------------------------------------------*/

            
            // 打印下一帧前，清除上一帧在标准错误输出的内容
            fflush(stderr);
        }
    }
    free(buffer);
    return 0;
}