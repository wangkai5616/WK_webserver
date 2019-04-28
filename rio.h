//
// Latest edit by TeeKee on 2017/7/23.
//

#ifndef RIO_H
#define RIO_H

#include <sys/types.h>
#define RIO_BUFSIZE 8192

typedef struct{
    int rio_fd;             /* descriptor for this internal buf *///描述符
    ssize_t rio_cnt;        /* unread bytes in internal buf *///buf中未读字节数
    char *rio_bufptr;       /* next unread byte in internal buf *///下一个未读字符指针
    char rio_buf[RIO_BUFSIZE]; /* internal buffer *///缓冲
}rio_t;

ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd); 
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

#endif
