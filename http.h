#ifndef HTTP_H
#define HTTP_H
//根据请求头进行处理
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "timer.h"
#include "util.h"
#include "rio.h"
#include "epoll.h"
#include "http_parse.h"
#include "http_request.h"

#define MAXLINE 8192
#define SHORTLINE 512

#define wk_str3_cmp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define wk_str3Ocmp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define wk_str4cmp(m, c0, c1, c2, c3)                                        \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

// 用key-value表示mime_type_t
/*
浏览器通常使用MIME类型（而不是文件扩展名）来确定如何处理文档；
因此服务器设置正确以将正确的MIME类型附加到响应对象的头部是非常重要的。
*/
typedef struct mime_type{
    const char *type;
    const char *value;
}mime_type_t;

void do_request(void *ptr);

#endif