//
// Latest edit by TeeKee on 2017/7/23.
//

#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include "http.h"
#include "threadpool.h"

#define MAXEVENTS 1024

int wk_epoll_create(int flags);
int wk_epoll_add(int epoll_fd, int fd, wk_http_request_t* request, int events);
int wk_epoll_mod(int epoll_fd, int fd, wk_http_request_t* request, int events);
int wk_epoll_del(int epoll_fd, int fd, wk_http_request_t* request, int events);
int wk_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout);
void wk_handle_events(int epoll_fd, int listen_fd, struct epoll_event* events,
                      int events_num, char* path, wk_threadpool_t* tp);

#endif
