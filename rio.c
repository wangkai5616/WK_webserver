//
// Latest edit by TeeKee on 2017/7/23.
//

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "rio.h"

//此函数尝试从fd中读取n个字符到usrbuf中，与read函数相比，
//它被信号处理函数中断后会再次尝试读取。因此，在除了可读字符数小于n情况下，
//该函数可以保证读取n个字节。
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char *)usrbuf;
    while(nleft > 0){
        if((nread = read(fd, bufp, nleft)) < 0){
			//被信号处理函数中断
			if(errno == EINTR)
                nread = 0;//本次读到0个字符，再次读取
            else
                return -1;
        }
		//读取到EOF
        else if(nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
	//返回实际读取的字节数
    return (n - nleft);
}

/*
rio_writen(int fd, void *, size_t)代表文件描述符的参数是int类型，而不是rio_t类型。
因为rio_writen不需要写缓冲。这是为什么呢？按道理来说，既然我们为read封装的rio_readn提供了缓冲区，
为什么不也为write提供一个有缓冲的rio_writen函数呢？

试想一个场景，你正在写一个http的请求报文，然后将这个报文写入了对应socket的文件描述符的缓冲区，
假设缓冲区大小为8K，该请求报文大小为1K。那么，如果缓冲区被设置为被填满才会自动将其
真正写入文件（而且一般也是这样做的），那就是说如果没有提供一个刷新缓冲区的函数手动刷新，
我还需要额外发送7K的数据将缓冲区填满，这个请求报文才能真正被写入到socket当中。
所以，一般带有缓冲区的函数库都会一个刷新缓冲区的函数，用于将在缓冲区的数据真正写入文件当中，
即使缓冲区没有被填满，而这也是C标准库的做法。然而，如果一个程序员一不小心忘记在写入操作完成
后手动刷新，那么该数据（请求报文）便一直驻留在缓冲区，而你的进程还在傻傻地等待响应。
*/
//此函数同理，保证写出n字节。
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char *)usrbuf;

    while(nleft > 0){
        if((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)
                nwritten = 0;
            else{
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
//带有缓冲区的RIO函数。缓冲区存在的目的是为了减少因多次调用系统级IO函数，
//陷入内核态而带来的额外开销。
/*
底层读取函数。当缓冲区数据充足时，此函数直接拷贝缓
冲区的数据给上层读取函数；当缓冲区不足时，该函数通过系统调用
从文件中读取最大数量的字节到缓冲区，再拷贝缓冲区数据给上层函数
*/
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    size_t cnt;
	//缓冲区中没有数据时，才去执行系统调用
    while(rp->rio_cnt <= 0){
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0){
            if(errno == EAGAIN){
                return -EAGAIN;
            }
			//遇到中断类型的错误的话应该进行读取，否则返回错误
            if(errno != EINTR){
                return -1;
            }
        }
		//读取到了EOF
        else if(rp->rio_cnt == 0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }
    cnt = n;
    if(rp->rio_cnt < (ssize_t)n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

//此函数类似rio_readn,不过因为加入了缓冲区，所以减少了陷入内核态时的开销。
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char *)usrbuf;
    while (nleft > 0){
		//会多次调用static函数rio_read
        if((nread = rio_read(rp, bufp, nleft)) < 0){
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        else if(nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}

//读取一行的数据，遇到'\n'结尾代表一行
//参数：rio_t包、用户地址即目的地址、一行的最大长度
//返回字节数包括换行符。
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    size_t n;
    ssize_t rc;
    char c, *bufp = (char *)usrbuf;
    for(n = 1; n < maxlen; n++){
        if((rc = rio_read(rp, &c, 1)) == 1){
            *bufp++ = c;
            if(c == '\n')
            break;
        }
		//没有接收到数据
        else if(rc == 0){
			//如果第一次循环就没接收到数据，则代表无数据可接收
            if (n == 1){
                return 0;
            }
            else
                break;
        }
        else if(rc == -EAGAIN){
            return rc;
        }
        else{
            return -1;
        }
    }
    *bufp = 0;
    return n;
}
