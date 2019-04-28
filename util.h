#ifndef UTIL_H
#define UTIL_H

#define PATHLEN 128
#define LISTENQ 1024
#define BUFLEN 8192
#define DELIM "="

#define wk_CONF_OK 0
#define wk_CONF_ERROR -1

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//�����ļ��ṹ��
typedef struct wk_conf{
    char root[PATHLEN];//·��
    int port;//�˿ں�
    int thread_num;//�߳���
}wk_conf_t;

int read_conf(char* filename, wk_conf_t* conf);
void handle_for_sigpipe();
int socket_bind_listen(int port);
int make_socket_non_blocking(int fd);
void accept_connection(int listen_fd, int epoll_fd, char* path);

#endif
