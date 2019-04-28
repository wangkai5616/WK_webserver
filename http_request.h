#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
//根据请求头进行处理
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "list.h"

#define wk_AGAIN EAGAIN

#define wk_HTTP_PARSE_INVALID_METHOD        10
#define wk_HTTP_PARSE_INVALID_REQUEST       11
#define wk_HTTP_PARSE_INVALID_HEADER        12

#define wk_HTTP_UNKNOWN                     0x0001
#define wk_HTTP_GET                         0x0002
#define wk_HTTP_HEAD                        0x0004
#define wk_HTTP_POST                        0x0008

#define wk_HTTP_OK                          200
#define wk_HTTP_NOT_MODIFIED                304
#define wk_HTTP_NOT_FOUND                   404
#define MAX_BUF 8124

//请求信息结构
typedef struct wk_http_request{
    char* root;//配置目录
    int fd;//描述符（监听、连接）
    int epoll_fd;//epoll描述符
    char buff[MAX_BUF];//用户缓冲
    size_t pos;//缓冲区的起始下标
    size_t last;//缓冲区的结束下标
    int state;//请求解析状态

    void* request_start;//请求行的开始位置
    void* method_end;//请求方法结束位置
    int method; //请求方法
    void* uri_start;//记录URL的起始位置
    void* uri_end;//记录URL的结束位置
    void* path_start;
    void* path_end;
    void* query_start;
    void* query_end;
    int http_major;//记录主版本号
    int http_minor;//记录次版本号
    void* request_end;//请求行的结束位置

    struct list_head list;    // 存储请求头，list.h中定义了此结构

    void* cur_header_key_start;
    void* cur_header_key_end;
    void* cur_header_value_start;
    void* cur_header_value_end;
    void* timer;
}wk_http_request_t;

//响应头结构
typedef struct wk_http_out{
    int fd;//连接描述符
    int keep_alive;//HTTP连接状态
    time_t mtime;//文件的最后修改时间
    int modified;//判断文件是否修改
    int status;//返回码
}wk_http_out_t;

//请求头结构体
typedef struct wk_http_header{
    void* key_start;
    void* key_end;
    void* value_start;
    void* value_end;
    struct list_head list;
}wk_http_header_t;

typedef int (*wk_http_header_handler_pt)(wk_http_request_t* request, wk_http_out_t* out, char* data, int len);

typedef struct wk_http_header_handle{
    char* name;
    wk_http_header_handler_pt handler;    // 函数指针
}wk_http_header_handle_t;

extern wk_http_header_handle_t wk_http_headers_in[];

void wk_http_handle_header(wk_http_request_t* request, wk_http_out_t* out);
int wk_http_close_conn(wk_http_request_t* request);
int wk_init_request_t(wk_http_request_t* request, int fd, int epoll_fd, char* path);
int wk_init_out_t(wk_http_out_t* out, int fd);
const char* get_shortmsg_from_status_code(int status_code);

#endif