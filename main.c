/*
 * This file is encoded in GB 2312. If you encounter garbled characters,
 * please set your text editor to use GB 2312 encoding to view it correctly.
 * For VS Code users, add "files.autoGuessEncoding": true" in your .vscode/settings.json
 * ͼ����������ȡ�� (�ӱ�׼����)
 *
 * ���C��������Ҫ�κ�����⡣��ֻ��һ���򵥵Ĺ��ߣ�
 * �ӱ�׼������(stdin)��ȡ�̶���С�Ķ��������ݿ飬
 * ��Щ������һ���ⲿ������Python�ű���ͨ���ܵ����롣
 */
#include "zf_common_headfile.h" // ȷ�����Ͷ������


int main() {
    // ��Ҫ����Windows�ϣ����뽫stdin����Ϊ������ģʽ��
    // ����ϵͳ���ܻ����ش���ĳЩ�ֽڣ��绻�з�����
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        perror("�޷���stdin����Ϊ������ģʽ");
        return 1;
    }

    // ����һ�������������һ֡ͼ�������
    unsigned char* buffer = (unsigned char*)malloc(FRAME_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "�ڴ����ʧ��\n");
        return 1;
    }

    myprint("C������������׼���ӱ�׼�����ȡ����...\n");

    unsigned long long frame_counter = 0;
    size_t bytes_read;
/*----------------------------------�ڴ˴������Ҫ��ʼ���Ĵ���--------------------------------------------------------------*/


/*----------------------------------�ڴ˴������Ҫ��ʼ���Ĵ���--------------------------------------------------------------*/

    // ����ѭ���������ӱ�׼�����ȡ����
    while ((bytes_read = fread(buffer, 1, FRAME_SIZE, stdin)) > 0) {
        if (bytes_read == FRAME_SIZE) {
            frame_counter++;

            // ��һά���������ݸ��Ƶ���άͼ������
            for (int i = 0; i < IMAGE_HEIGHT; i++) {
                for (int j = 0; j < IMAGE_WIDTH; j++) {
                    mt9v03x_image[i][j] = buffer[i * IMAGE_WIDTH + j];
                }
            }
            
            ips200_show(&mt9v03x_image[0][0], IMAGE_WIDTH, IMAGE_HEIGHT); // ��ʾͼ��
            
            // ��ʾһ��5���ؿ�Ĳʺ磬���ϵ��¹���
            for (int i = 0; i < 5; i++) {
                int row = (frame_counter + i) % IMAGE_HEIGHT;
                for (int j = 0; j < IMAGE_WIDTH; j++) {
                    // ������ɫֵ���γɲʺ�Ч��
                    uint8_t color = (uint8_t)((j * 256 / IMAGE_WIDTH + i * 50) % 256);
                    // ʹ�� ips200_draw_point ���Ʋ�ɫ��
                    ips200_draw_point(j, row, (color << 11) | (color << 5) | color);
                }
            }
/*----------------------------------�ڴ˴������Ҫѭ�����еĴ���--------------------------------------------------------------*/

/*----------------------------------�ڴ˴������Ҫѭ�����еĴ���--------------------------------------------------------------*/

            
            // ��ӡ��һ֡ǰ�������һ֡�ڱ�׼�������������
            ips200_refresh();
            fflush(stderr);
        }
    }
    free(buffer);
    return 0;
}